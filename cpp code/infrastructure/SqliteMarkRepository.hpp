#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"

namespace lms {
class SqliteMarkRepository final : public IMarkRepository {
public:
    explicit SqliteMarkRepository(Database& db) : db_(db) {}
    std::optional<Mark> find(int submissionId) override;
    void save(const Mark& m) override;
    int addAudit(AuditEntry& e) override;
    std::vector<AuditEntry> listAudit(int submissionId) override;
private:
    Database& db_;
};
}  // namespace lms
