#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"

namespace lms {
class SqliteAssignmentRepository final : public IAssignmentRepository {
public:
    explicit SqliteAssignmentRepository(Database& db) : db_(db) {}
    std::optional<Assignment> findById(int id) override;
    std::vector<Assignment> listByCourse(int courseId) override;
    int save(Assignment& a) override;
    int addExtension(Extension& e) override;
    std::optional<Extension> findExtension(int assignmentId, int studentId) override;
private:
    Database& db_;
};
}  // namespace lms
