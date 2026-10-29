#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"
#include "services/Dtos.hpp"

namespace lms {
class ModerationService {
public:
    ModerationService(ISubmissionRepository& submissions, IMarkRepository& marks, IUserRepository& users,
                      IAssignmentRepository& assignments, ICourseRepository& courses, IClock& clock, Database& db)
        : submissions_(submissions), marks_(marks), users_(users), assignments_(assignments), courses_(courses),
          clock_(clock), db_(db) {}
    /// FR14: Marked and Moderated submissions of the HoD's department.
    std::vector<SubmissionRow> queue(int hodId);
    /// FR15: approve a Marked submission (UC19).
    void approve(int hodId, int submissionId);
    /// FR15: change the final mark, with a reason; written to the audit log in the same transaction (UC20).
    void changeMark(int hodId, const MarkChange& req);
    /// FR16: release one submission (UC22). Throws RuleError if it has no feedback.
    void release(int hodId, int submissionId);
    /// FR16: release every eligible submission of an assignment; skipped ones are listed (UC23).
    ReleaseReport releaseAll(int hodId, int assignmentId);
    /// FR17.
    std::vector<AuditEntry> viewAudit(int hodId, int submissionId);
    /// The mark and feedback so the HoD can review it.
    std::optional<Mark> viewMark(int hodId, int submissionId);
private:
    const HeadOfDepartment& requireHod(UserPtr& holder, int hodId);
    Submission loadInDepartment(const HeadOfDepartment& hod, int submissionId);
    void releaseOne(const HeadOfDepartment& hod, Submission& sub);
    ISubmissionRepository& submissions_;
    IMarkRepository& marks_;
    IUserRepository& users_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IClock& clock_;
    Database& db_;
};
}  // namespace lms
