#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"

namespace lms {
class SqliteSubmissionRepository final : public ISubmissionRepository {
public:
    explicit SqliteSubmissionRepository(Database& db) : db_(db) {}
    std::optional<Submission> findById(int id) override;
    std::optional<Submission> find(int assignmentId, int studentId) override;
    std::vector<Submission> listByAssignment(int assignmentId) override;
    int save(Submission& s) override;
    int addVersion(SubmissionVersion& v) override;
    std::optional<SubmissionVersion> latestVersion(int submissionId) override;
private:
    Database& db_;
};
}  // namespace lms
