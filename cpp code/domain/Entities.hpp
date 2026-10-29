#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "domain/Types.hpp"

namespace lms {

class Department {
public:
    Department(int id, std::string name) : id_(id), name_(std::move(name)) {}
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    void setId(int id) { id_ = id; }
private:
    int id_;
    std::string name_;
};

class Course {
public:
    Course(int id, std::string code, std::string title, int departmentId)
        : id_(id), code_(std::move(code)), title_(std::move(title)), departmentId_(departmentId) {}
    int getId() const { return id_; }
    const std::string& getCode() const { return code_; }
    const std::string& getTitle() const { return title_; }
    int getDepartmentId() const { return departmentId_; }
    void setId(int id) { id_ = id; }
private:
    int id_;
    std::string code_;
    std::string title_;
    int departmentId_;
};

class Assignment {
public:
    Assignment(int id, int courseId, std::string title, std::string description, TimePoint deadline,
               double maxMark)
        : id_(id), courseId_(courseId), title_(std::move(title)), description_(std::move(description)),
          deadline_(deadline), maxMark_(maxMark) {}
    int getId() const { return id_; }
    int getCourseId() const { return courseId_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getDescription() const { return description_; }
    TimePoint getDeadline() const { return deadline_; }
    double getMaxMark() const { return maxMark_; }
    void setId(int id) { id_ = id; }
private:
    int id_;
    int courseId_;
    std::string title_;
    std::string description_;
    TimePoint deadline_;
    double maxMark_;
};

class Extension {
public:
    Extension(int id, int assignmentId, int studentId, int grantedBy, TimePoint newDeadline,
              std::string reason, TimePoint grantedAt)
        : id_(id), assignmentId_(assignmentId), studentId_(studentId), grantedBy_(grantedBy),
          newDeadline_(newDeadline), reason_(std::move(reason)), grantedAt_(grantedAt) {}
    int getId() const { return id_; }
    int getAssignmentId() const { return assignmentId_; }
    int getStudentId() const { return studentId_; }
    int getGrantedBy() const { return grantedBy_; }
    TimePoint getNewDeadline() const { return newDeadline_; }
    const std::string& getReason() const { return reason_; }
    TimePoint getGrantedAt() const { return grantedAt_; }
    void setId(int id) { id_ = id; }
private:
    int id_;
    int assignmentId_;
    int studentId_;
    int grantedBy_;
    TimePoint newDeadline_;
    std::string reason_;
    TimePoint grantedAt_;
};

/// The deadline that applies to one student: the assignment deadline, or the later
/// extension deadline if the lecturer granted one (UC13).
TimePoint effectiveDeadline(const Assignment& a, const std::optional<Extension>& ext);

class SubmissionVersion {
public:
    SubmissionVersion(int id, int submissionId, int versionNo, std::string fileName, std::int64_t fileSize,
                      std::vector<std::uint8_t> content, TimePoint uploadedAt)
        : id_(id), submissionId_(submissionId), versionNo_(versionNo), fileName_(std::move(fileName)),
          fileSize_(fileSize), content_(std::move(content)), uploadedAt_(uploadedAt) {}
    int getId() const { return id_; }
    int getSubmissionId() const { return submissionId_; }
    int getVersionNo() const { return versionNo_; }
    const std::string& getFileName() const { return fileName_; }
    std::int64_t getFileSize() const { return fileSize_; }
    const std::vector<std::uint8_t>& getContent() const { return content_; }
    TimePoint getUploadedAt() const { return uploadedAt_; }
    void setId(int id) { id_ = id; }
    void setSubmissionId(int id) { submissionId_ = id; }
private:
    int id_;
    int submissionId_;
    int versionNo_;
    std::string fileName_;
    std::int64_t fileSize_;
    std::vector<std::uint8_t> content_;
    TimePoint uploadedAt_;
};

/// One student's work for one assignment, with its life cycle (State machine, Figure 5).
class Submission {
public:
    Submission(int id, int assignmentId, int studentId, SubmissionStatus status, TimePoint firstSubmittedAt)
        : id_(id), assignmentId_(assignmentId), studentId_(studentId), status_(status),
          firstSubmittedAt_(firstSubmittedAt) {}
    int getId() const { return id_; }
    int getAssignmentId() const { return assignmentId_; }
    int getStudentId() const { return studentId_; }
    SubmissionStatus getStatus() const { return status_; }
    TimePoint getFirstSubmittedAt() const { return firstSubmittedAt_; }
    void setId(int id) { id_ = id; }
    bool isReleased() const { return status_ == SubmissionStatus::Released; }
    /// Moves to the next status. Throws RuleError for a move that is not in the state diagram.
    void advanceTo(SubmissionStatus next);
    static bool isAllowed(SubmissionStatus from, SubmissionStatus to);
private:
    int id_;
    int assignmentId_;
    int studentId_;
    SubmissionStatus status_;
    TimePoint firstSubmittedAt_;
};

class Mark {
public:
    Mark(int submissionId, double awardedMark, double penalty, double finalMark, std::string feedback,
         int markedBy, TimePoint markedAt)
        : submissionId_(submissionId), awardedMark_(awardedMark), penalty_(penalty), finalMark_(finalMark),
          feedback_(std::move(feedback)), markedBy_(markedBy), markedAt_(markedAt) {}
    int getSubmissionId() const { return submissionId_; }
    double getAwardedMark() const { return awardedMark_; }
    double getPenalty() const { return penalty_; }
    double getFinalMark() const { return finalMark_; }
    const std::string& getFeedback() const { return feedback_; }
    int getMarkedBy() const { return markedBy_; }
    TimePoint getMarkedAt() const { return markedAt_; }
    bool hasFeedback() const;
    void setFinalMark(double m) { finalMark_ = m; }
private:
    int submissionId_;
    double awardedMark_;
    double penalty_;
    double finalMark_;
    std::string feedback_;
    int markedBy_;
    TimePoint markedAt_;
};

class AuditEntry {
public:
    AuditEntry(int id, int submissionId, int actorId, TimePoint at, AuditAction action,
               std::optional<double> oldValue, std::optional<double> newValue, std::string reason)
        : id_(id), submissionId_(submissionId), actorId_(actorId), at_(at), action_(action),
          oldValue_(oldValue), newValue_(newValue), reason_(std::move(reason)) {}
    int getId() const { return id_; }
    int getSubmissionId() const { return submissionId_; }
    int getActorId() const { return actorId_; }
    TimePoint getAt() const { return at_; }
    AuditAction getAction() const { return action_; }
    const std::optional<double>& getOldValue() const { return oldValue_; }
    const std::optional<double>& getNewValue() const { return newValue_; }
    const std::string& getReason() const { return reason_; }
    void setId(int id) { id_ = id; }
private:
    int id_;
    int submissionId_;
    int actorId_;
    TimePoint at_;
    AuditAction action_;
    std::optional<double> oldValue_;
    std::optional<double> newValue_;
    std::string reason_;
};

}  // namespace lms
