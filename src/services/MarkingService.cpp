#include "services/MarkingService.hpp"

#include <algorithm>

#include "domain/Errors.hpp"
#include "services/Validation.hpp"

namespace lms {

// UC15, UC16: a lecturer marks a submission for the first time. Steps:
//   1. check the lecturer teaches the course, the work is not marked yet, and the mark is valid
//   2. work out how late the file was, and ask the penalty policy what the penalty is (BR01)
//   3. save the mark, the audit entry and the new status together in one transaction
void MarkingService::recordMark(const MarkRequest& req) {
    auto sub = submissions_.findById(req.submissionId);
    if (!sub) throw NotFoundError("Submission not found");
    auto asg = assignments_.findById(sub->getAssignmentId());
    if (!asg) throw NotFoundError("Assignment not found");
    if (!courses_.isTeaching(asg->getCourseId(), req.lecturerId)) throw PermissionError("You do not teach this course");
    if (sub->getStatus() != SubmissionStatus::Submitted && sub->getStatus() != SubmissionStatus::Late)
        throw RuleError("This submission is already marked. Only the Head of Department can change the mark.");
    validate::requireMark(req.awardedMark, asg->getMaxMark(), "Mark");
    std::string feedback = validate::trim(req.feedback);
    if (feedback.size() > 5000) throw ValidationError("Feedback is too long (maximum 5000 characters)");

    auto version = submissions_.latestVersion(sub->getId());
    if (!version) throw NotFoundError("The submission has no file");
    // How late was the file? A negative or zero answer means on time. The policy turns it into a penalty.
    const TimePoint eff = effectiveDeadline(*asg, assignments_.findExtension(asg->getId(), sub->getStudentId()));
    const Duration lateBy = std::chrono::duration_cast<Duration>(version->getUploadedAt() - eff);
    const double penalty = penalty_.penaltyFor(asg->getMaxMark(), lateBy);
    const double finalMark = std::max(0.0, req.awardedMark - penalty);  // never below zero
    const TimePoint now = clock_.now();

    Transaction tx(db_);
    marks_.save(Mark(sub->getId(), req.awardedMark, penalty, finalMark, feedback, req.lecturerId, now));
    AuditEntry audit(0, sub->getId(), req.lecturerId, now, AuditAction::MarkSet, std::nullopt, finalMark, "first marking");
    marks_.addAudit(audit);
    sub->advanceTo(SubmissionStatus::Marked);
    submissions_.save(*sub);
    tx.commit();
}

std::vector<SubmissionRow> MarkingService::listToMark(int lecturerId, int assignmentId) {
    auto asg = assignments_.findById(assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    if (!courses_.isTeaching(asg->getCourseId(), lecturerId)) throw PermissionError("You do not teach this course");
    std::vector<SubmissionRow> rows;
    for (auto& s : submissions_.listByAssignment(assignmentId)) {
        SubmissionRow r{s, "", std::nullopt, false};
        if (UserPtr u = users_.findById(s.getStudentId())) r.studentName = u->getUsername();
        if (auto m = marks_.find(s.getId())) { r.finalMark = m->getFinalMark(); r.hasFeedback = m->hasFeedback(); }
        rows.push_back(std::move(r));
    }
    return rows;
}

SubmissionVersion MarkingService::latestFile(int lecturerId, int submissionId) {
    auto sub = submissions_.findById(submissionId);
    if (!sub) throw NotFoundError("Submission not found");
    auto asg = assignments_.findById(sub->getAssignmentId());
    if (!asg || !courses_.isTeaching(asg->getCourseId(), lecturerId)) throw PermissionError("You do not teach this course");
    auto v = submissions_.latestVersion(submissionId);
    if (!v) throw NotFoundError("The submission has no file");
    return *v;
}

}  // namespace lms
