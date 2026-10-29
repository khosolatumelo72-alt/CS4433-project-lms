#include "services/AuthService.hpp"

#include "domain/Errors.hpp"
#include "services/Validation.hpp"

namespace lms {

UserPtr AuthService::login(const std::string& username, const std::string& password) {
    const char* kBad = "Invalid user name or password";
    UserPtr user = users_.findByUsername(validate::trim(username));
    if (!user) throw AuthError(kBad);
    const TimePoint now = clock_.now();
    if (!user->isActive()) throw AuthError("This account is disabled. Ask an administrator.");
    if (user->isLocked(now))
        throw AuthError("This account is locked until " + displayTime(*user->getLockedUntil()) + ".");
    if (!hasher_.verify(password, user->getPasswordHash())) {
        user->registerFailedLogin(now);
        users_.save(*user);
        throw AuthError(kBad);
    }
    user->resetFailedLogins();
    users_.save(*user);
    return user;
}

void AuthService::changePassword(int userId, const NewPassword& req) {
    UserPtr user = users_.findById(userId);
    if (!user) throw NotFoundError("user not found");
    if (!hasher_.verify(req.oldPassword, user->getPasswordHash())) throw AuthError("The current password is wrong");
    validate::requirePassword(req.newPassword, user->getUsername());
    user->setPasswordHash(hasher_.hash(req.newPassword));
    users_.save(*user);
}

}  // namespace lms
