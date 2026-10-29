#include "services/ModerationService.hpp"

#include "domain/Errors.hpp"
#include "services/Validation.hpp"

namespace lms {

// Every public method starts here. It checks that the caller is an active Head of Department.
// "holder" keeps the loaded user alive, so the reference we return stays valid for the caller.
// dynamic_cast asks "is this User really a HeadOfDepartment?" and gives null if it is not.
const HeadOfDepartment& ModerationService::requireHod(UserPtr& holder, int hodId) {
    holder = users_.findById(hodId);
    const auto* hod = dynamic_cast<const HeadOfDepartment*>(holder.get());
    if (!hod || !hod->isActive()) throw PermissionError("Only a Head of Department can do this");
    return *hod;
}

// Loads a submission, but only if its course belongs to the Head of Department's own department.
// Submission -> Assignment -> Course -> Department is the chain we follow to find out.
Submission ModerationService::loadInDepartment(const HeadOfDepartment& hod, int submissionId) {
    auto sub = submissions_.findById(submissionId);
    if (!sub) throw NotFoundError("Submission not found");
    auto asg = assignments_.findById(sub->getAssignmentId());
    auto course = asg ? courses_.findById(asg->getCourseId()) : std::nullopt;
    if (!course || course->getDepartmentId() != hod.getDepartmentId())
        throw PermissionError("This submission is not in your department");
    return *sub;
}

std::vector<SubmissionRow> ModerationService::queue(int hodId) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    std::vector<SubmissionRow> rows;
    for (const auto& c : courses_.listByDepartment(hod.getDepartmentId()))
        for (const auto& a : assignments_.listByCourse(c.getId()))
            for (auto& s : submissions_.listByAssignment(a.getId())) {
                if (s.getStatus() != SubmissionStatus::Marked && s.getStatus() != SubmissionStatus::Moderated) continue;
                SubmissionRow r{s, "", std::nullopt, false};
                if (UserPtr u = users_.findById(s.getStudentId())) r.studentName = u->getUsername();
                if (auto m = marks_.find(s.getId())) { r.finalMark = m->getFinalMark(); r.hasFeedback = m->hasFeedback(); }
                rows.push_back(std::move(r));
            }
    return rows;
}

std::optional<Mark> ModerationService::viewMark(int hodId, int submissionId) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    loadInDepartment(hod, submissionId);
    return marks_.find(submissionId);
}

// UC19: the Head of Department approves a Marked submission. The audit entry and the status change
// are saved together in one transaction: either both happen or neither does (NFR7).
void ModerationService::approve(int hodId, int submissionId) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    Submission sub = loadInDepartment(hod, submissionId);
    if (sub.getStatus() != SubmissionStatus::Marked) throw RuleError("Only a Marked submission can be approved");
    const TimePoint now = clock_.now();
    Transaction tx(db_);
    AuditEntry audit(0, submissionId, hodId, now, AuditAction::Approved, std::nullopt, std::nullopt, "approved");
    marks_.addAudit(audit);
    sub.advanceTo(SubmissionStatus::Moderated);
    submissions_.save(sub);
    tx.commit();
}

// UC20: the Head of Department changes the final mark. A reason is required, and the old and
// new marks are written to the audit log in the same transaction as the change (BR4, NFR7).
void ModerationService::changeMark(int hodId, const MarkChange& req) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    Submission sub = loadInDepartment(hod, req.submissionId);
    if (sub.getStatus() != SubmissionStatus::Marked && sub.getStatus() != SubmissionStatus::Moderated)
        throw RuleError("The mark can only be changed while the submission is Marked or Moderated");
    auto asg = assignments_.findById(sub.getAssignmentId());
    validate::requireMark(req.newFinalMark, asg->getMaxMark(), "Mark");
    std::string reason = validate::requireText(req.reason, "Reason", 500);
    auto mark = marks_.find(sub.getId());
    if (!mark) throw NotFoundError("The submission has no mark");
    const double old = mark->getFinalMark();
    if (old == req.newFinalMark) throw ValidationError("The new mark is the same as the current mark");
    const TimePoint now = clock_.now();
    Transaction tx(db_);
    mark->setFinalMark(req.newFinalMark);
    marks_.save(*mark);
    AuditEntry audit(0, sub.getId(), hodId, now, AuditAction::MarkChanged, old, req.newFinalMark, reason);
    marks_.addAudit(audit);
    tx.commit();
}

// Releases one submission so the student can see the mark (UC21, UC22). Rules checked in order:
//   1. the status must be Marked or Moderated (BR3)
//   2. written feedback must exist (BR6)
// A Marked submission is approved at the same time, because releasing it counts as approval.
void ModerationService::releaseOne(const HeadOfDepartment& hod, Submission& sub) {
    if (sub.getStatus() != SubmissionStatus::Marked && sub.getStatus() != SubmissionStatus::Moderated)
        throw RuleError("Only a Marked or Moderated submission can be released (status is " + toString(sub.getStatus()) + ")");
    auto mark = marks_.find(sub.getId());
    if (!mark || !mark->hasFeedback()) throw RuleError("Written feedback is required before release");
    const TimePoint now = clock_.now();
    Transaction tx(db_);
    if (sub.getStatus() == SubmissionStatus::Marked) {  // releasing counts as approval (BR3)
        AuditEntry audit(0, sub.getId(), hod.getId(), now, AuditAction::Approved, std::nullopt, std::nullopt,
                         "approved by release");
        marks_.addAudit(audit);
        sub.advanceTo(SubmissionStatus::Moderated);
    }
    sub.advanceTo(SubmissionStatus::Released);
    submissions_.save(sub);
    tx.commit();
}

void ModerationService::release(int hodId, int submissionId) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    Submission sub = loadInDepartment(hod, submissionId);
    releaseOne(hod, sub);
}

// Releases every Marked or Moderated submission of an assignment. A submission that cannot be
// released (for example it has no feedback) is skipped and listed in the report, and the loop goes on.
ReleaseReport ModerationService::releaseAll(int hodId, int assignmentId) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    auto asg = assignments_.findById(assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    auto course = courses_.findById(asg->getCourseId());
    if (!course || course->getDepartmentId() != hod.getDepartmentId())
        throw PermissionError("This assignment is not in your department");
    ReleaseReport report;
    for (Submission& s : submissions_.listByAssignment(assignmentId)) {
        if (s.getStatus() != SubmissionStatus::Marked && s.getStatus() != SubmissionStatus::Moderated) continue;
        try {
            releaseOne(hod, s);
            report.released.push_back(s.getId());
        } catch (const LmsError& e) {  // one failure must not stop the others (UC21 5a)
            report.skipped.emplace_back(s.getId(), e.what());
        }
    }
    return report;
}

std::vector<AuditEntry> ModerationService::viewAudit(int hodId, int submissionId) {
    UserPtr holder;
    const HeadOfDepartment& hod = requireHod(holder, hodId);
    loadInDepartment(hod, submissionId);
    return marks_.listAudit(submissionId);
}

}  // namespace lms
