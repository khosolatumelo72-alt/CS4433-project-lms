#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"
#include "services/Dtos.hpp"

namespace lms {
class SubmissionService {
public:
    static constexpr std::int64_t kMaxFileBytes = 10 * 1024 * 1024;

    SubmissionService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                      ICourseRepository& courses, IUserRepository& users, IMarkRepository& marks, IClock& clock,
                      Database& db)
        : submissions_(submissions), assignments_(assignments), courses_(courses), users_(users),
          marks_(marks), clock_(clock), db_(db) {}
    /// FR08, FR10, FR11. Throws RuleError when a replacement is not allowed (UC10 flows 2c, 2d).
    SubmitResult submit(const SubmitRequest& req);
    /// The deadline that applies to this student (UC13).
    TimePoint effectiveDeadlineFor(int studentId, int assignmentId);
    /// FR19. The mark is included only when the submission is Released (BR2).
    SubmissionView viewOwn(int studentId, int assignmentId);
    /// FR18. Students enrolled in the course who have not submitted.
    std::vector<UserPtr> listNotSubmitted(int lecturerId, int assignmentId);
private:
    void replaceFile(const Submission& existing, const std::string& name, const std::vector<std::uint8_t>& content,
                     TimePoint now, TimePoint deadline, SubmitResult& result);
    void saveFirstSubmission(const SubmitRequest& req, const std::string& name,
                             const std::vector<std::uint8_t>& content, TimePoint now, TimePoint deadline,
                             SubmitResult& result);
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IUserRepository& users_;
    IMarkRepository& marks_;
    IClock& clock_;
    Database& db_;
};
}  // namespace lms
