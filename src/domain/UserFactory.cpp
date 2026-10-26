#include "domain/Errors.hpp"
#include "domain/User.hpp"

namespace lms {

UserPtr UserFactory::create(const UserSpec& s) {
    if (s.username.empty()) throw ValidationError("a user needs a user name");
    switch (s.role) {
        case Role::Student:
            return std::make_unique<Student>(s.id, s.username, s.passwordHash, s.active, s.failedAttempts, s.lockedUntil);
        case Role::Lecturer:
            return std::make_unique<Lecturer>(s.id, s.username, s.passwordHash, s.active, s.failedAttempts, s.lockedUntil);
        case Role::Administrator:
            return std::make_unique<Administrator>(s.id, s.username, s.passwordHash, s.active, s.failedAttempts, s.lockedUntil);
        case Role::HeadOfDepartment:
            if (s.departmentId <= 0) throw ValidationError("a Head of Department needs a department");
            return std::make_unique<HeadOfDepartment>(s.id, s.username, s.passwordHash, s.active, s.failedAttempts,
                                                      s.lockedUntil, s.departmentId);
    }
    throw ValidationError("unknown role");
}

}  // namespace lms
