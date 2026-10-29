#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"
#include "services/Dtos.hpp"

namespace lms {
class MarkingService {
public:
    MarkingService(ISubmissionRepository& submissions, IMarkRepository& marks, IAssignmentRepository& assignments,
                   ICourseRepository& courses, IUserRepository& users, IPenaltyPolicy& penalty, IClock& clock,
                   Database& db)
        : submissions_(submissions), marks_(marks), assignments_(assignments), courses_(courses), users_(users),
          penalty_(penalty), clock_(clock), db_(db) {}
    /// FR09, FR12, FR17 (UC15). Applies the late penalty and writes the audit entry in one transaction.
    void recordMark(const MarkRequest& req);
    /// All submissions for an assignment the lecturer teaches, with their status.
    std::vector<SubmissionRow> listToMark(int lecturerId, int assignmentId);
    /// The newest file version, so the lecturer can open it.
    SubmissionVersion latestFile(int lecturerId, int submissionId);
private:
    ISubmissionRepository& submissions_;
    IMarkRepository& marks_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IUserRepository& users_;
    IPenaltyPolicy& penalty_;
    IClock& clock_;
    Database& db_;
};
}  // namespace lms
