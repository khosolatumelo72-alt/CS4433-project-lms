#include "infrastructure/SqliteUserRepository.hpp"

namespace lms {
namespace {
const char* kCols = "id, username, password_hash, role, department_id, active, failed_attempts, locked_until";

UserPtr fromRow(Statement& s) {
    UserSpec spec;
    spec.id = s.getInt(0);
    spec.username = s.getText(1);
    spec.passwordHash = s.getText(2);
    spec.role = roleFromString(s.getText(3));
    spec.departmentId = s.isNull(4) ? 0 : s.getInt(4);
    spec.active = s.getInt(5) != 0;
    spec.failedAttempts = s.getInt(6);
    if (!s.isNull(7)) spec.lockedUntil = parseTime(s.getText(7));
    return UserFactory::create(spec);
}
}  // namespace

UserPtr SqliteUserRepository::findById(int id) {
    Statement s(db_, std::string("SELECT ") + kCols + " FROM user_account WHERE id = ?1;");
    s.bind(1, id);
    return s.step() ? fromRow(s) : nullptr;
}

UserPtr SqliteUserRepository::findByUsername(const std::string& name) {
    Statement s(db_, std::string("SELECT ") + kCols + " FROM user_account WHERE username = ?1;");
    s.bind(1, name);
    return s.step() ? fromRow(s) : nullptr;
}

int SqliteUserRepository::save(User& u) {
    const auto* hod = dynamic_cast<const HeadOfDepartment*>(&u);
    if (u.getId() == 0) {
        Statement s(db_,
                    "INSERT INTO user_account(username, password_hash, role, department_id, active, "
                    "failed_attempts, locked_until) VALUES (?1,?2,?3,?4,?5,?6,?7);");
        s.bind(1, u.getUsername()).bind(2, u.getPasswordHash()).bind(3, toString(u.role()));
        if (hod) s.bind(4, hod->getDepartmentId()); else s.bindNull(4);
        s.bind(5, u.isActive() ? 1 : 0).bind(6, u.getFailedAttempts());
        if (u.getLockedUntil()) s.bind(7, formatTime(*u.getLockedUntil())); else s.bindNull(7);
        s.step();
        u.setId(static_cast<int>(db_.lastInsertRowId()));
    } else {
        Statement s(db_,
                    "UPDATE user_account SET password_hash=?2, active=?3, failed_attempts=?4, "
                    "locked_until=?5 WHERE id=?1;");
        s.bind(1, u.getId()).bind(2, u.getPasswordHash()).bind(3, u.isActive() ? 1 : 0).bind(4, u.getFailedAttempts());
        if (u.getLockedUntil()) s.bind(5, formatTime(*u.getLockedUntil())); else s.bindNull(5);
        s.step();
    }
    return u.getId();
}

std::vector<UserPtr> SqliteUserRepository::listByRole(Role role) {
    Statement s(db_, std::string("SELECT ") + kCols + " FROM user_account WHERE role = ?1 ORDER BY username;");
    s.bind(1, toString(role));
    std::vector<UserPtr> out;
    while (s.step()) out.push_back(fromRow(s));
    return out;
}
}  // namespace lms
