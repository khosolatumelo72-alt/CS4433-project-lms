#include "services/SubmissionService.hpp"

#include "domain/Errors.hpp"
#include "services/Validation.hpp"

namespace lms {

TimePoint SubmissionService::effectiveDeadlineFor(int studentId, int assignmentId) {
    auto asg = assignments_.findById(assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    return effectiveDeadline(*asg, assignments_.findExtension(assignmentId, studentId));
}

// UC10, UC11, UC12: a student submits a file. In short:
//   - check the file and that the student is enrolled
//   - first submission: save it as Submitted (on time) or Late (after the deadline, never refused)
//   - later submission: replace the file, but only while the work is still Submitted and the
//     effective deadline has not passed
SubmitResult SubmissionService::submit(const SubmitRequest& req) {
    // 1. Check the file itself.
    const FileData& file = req.file;
    std::string name = validate::requireText(file.fileName, "File name", 255);
    if (file.content.empty()) throw ValidationError("The file is empty");
    if (static_cast<std::int64_t>(file.content.size()) > kMaxFileBytes)
        throw ValidationError("The file is larger than 10 MB");

    // 2. Find the assignment and check the student is enrolled in its course.
    auto asg = assignments_.findById(req.assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    if (!courses_.isEnrolled(asg->getCourseId(), req.studentId))
        throw PermissionError("You are not enrolled in this course");

    // 3. Work out the times. The effective deadline is the assignment deadline, or the student's
    //    extension if it is later.
    const TimePoint now = clock_.now();
    const TimePoint deadline = effectiveDeadline(*asg, assignments_.findExtension(req.assignmentId, req.studentId));

    // 4. A student has at most one submission per assignment, so look for an earlier one.
    auto existing = submissions_.find(req.assignmentId, req.studentId);

    SubmitResult result;
    result.effectiveDeadline = deadline;
    result.submittedAt = now;
    if (existing)
        replaceFile(*existing, name, file.content, now, deadline, result);
    else
        saveFirstSubmission(req, name, file.content, now, deadline, result);
    return result;
}

// The student already has a submission, so this is a replacement (FR10). The old file is kept as an
// earlier version; the new file becomes the latest version.
void SubmissionService::replaceFile(const Submission& existing, const std::string& name,
                                    const std::vector<std::uint8_t>& content, TimePoint now, TimePoint deadline,
                                    SubmitResult& result) {
    if (existing.getStatus() == SubmissionStatus::Late) throw RuleError("Late work cannot be replaced");
    if (existing.getStatus() != SubmissionStatus::Submitted)
        throw RuleError("This work has already been marked and cannot be replaced");
    if (now > deadline) throw RuleError("The deadline has passed, so the file can no longer be replaced");

    auto latest = submissions_.latestVersion(existing.getId());
    int nextVersion = (latest ? latest->getVersionNo() : 0) + 1;

    Transaction tx(db_);
    SubmissionVersion version(0, existing.getId(), nextVersion, name, static_cast<std::int64_t>(content.size()),
                              content, now);
    submissions_.addVersion(version);
    tx.commit();

    result.submissionId = existing.getId();
    result.versionNo = nextVersion;
    result.status = existing.getStatus();
    result.isResubmission = true;
}

// The student's first file for this assignment. On time means Submitted, after the effective
// deadline means Late. Late work is still accepted (BR5).
void SubmissionService::saveFirstSubmission(const SubmitRequest& req, const std::string& name,
                                            const std::vector<std::uint8_t>& content, TimePoint now,
                                            TimePoint deadline, SubmitResult& result) {
    const SubmissionStatus status = (now <= deadline) ? SubmissionStatus::Submitted : SubmissionStatus::Late;

    // The submission and its first file are saved together, so we never keep one without the other (NFR6).
    Transaction tx(db_);
    Submission submission(0, req.assignmentId, req.studentId, status, now);
    submissions_.save(submission);  // this gives the submission its id
    SubmissionVersion version(0, submission.getId(), 1, name, static_cast<std::int64_t>(content.size()), content, now);
    submissions_.addVersion(version);
    tx.commit();

    result.submissionId = submission.getId();
    result.versionNo = 1;
    result.status = status;
}

SubmissionView SubmissionService::viewOwn(int studentId, int assignmentId) {
    auto asg = assignments_.findById(assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    if (!courses_.isEnrolled(asg->getCourseId(), studentId)) throw PermissionError("You are not enrolled in this course");
    SubmissionView v;
    v.assignmentTitle = asg->getTitle();
    v.effectiveDeadline = effectiveDeadline(*asg, assignments_.findExtension(assignmentId, studentId));
    if (auto s = submissions_.find(assignmentId, studentId)) {
        v.status = s->getStatus();
        if (s->isReleased()) v.mark = marks_.find(s->getId());  // BR2: only when Released
        if (auto latest = submissions_.latestVersion(s->getId())) v.versions = latest->getVersionNo();
    }
    return v;
}

std::vector<UserPtr> SubmissionService::listNotSubmitted(int lecturerId, int assignmentId) {
    auto asg = assignments_.findById(assignmentId);
    if (!asg) throw NotFoundError("Assignment not found");
    if (!courses_.isTeaching(asg->getCourseId(), lecturerId)) throw PermissionError("You do not teach this course");
    std::vector<UserPtr> out;
    for (int studentId : courses_.listStudentIds(asg->getCourseId()))
        if (!submissions_.find(assignmentId, studentId))
            if (UserPtr u = users_.findById(studentId)) out.push_back(std::move(u));
    return out;
}

}  // namespace lms
