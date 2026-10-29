#pragma once
#include "domain/Ports.hpp"
#include "infrastructure/Database.hpp"

namespace lms {
class SqliteUserRepository final : public IUserRepository {
public:
    explicit SqliteUserRepository(Database& db) : db_(db) {}
    UserPtr findById(int id) override;
    UserPtr findByUsername(const std::string& name) override;
    int save(User& user) override;
    std::vector<UserPtr> listByRole(Role role) override;
private:
    Database& db_;
};
}  // namespace lms
