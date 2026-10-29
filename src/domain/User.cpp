#include "domain/User.hpp"

namespace lms {

User::User(int id, std::string username, std::string passwordHash, bool active, int failedAttempts,
           std::optional<TimePoint> lockedUntil)
    : id_(id), username_(std::move(username)), passwordHash_(std::move(passwordHash)), active_(active),
      failedAttempts_(failedAttempts), lockedUntil_(lockedUntil) {}

bool User::isLocked(TimePoint now) const { return lockedUntil_.has_value() && now < *lockedUntil_; }

void User::registerFailedLogin(TimePoint now) {
    ++failedAttempts_;
    if (failedAttempts_ >= kMaxFailedAttempts) {
        lockedUntil_ = now + std::chrono::minutes(kLockMinutes);
        failedAttempts_ = 0;
    }
}

void User::resetFailedLogins() {
    failedAttempts_ = 0;
    lockedUntil_.reset();
}

}  // namespace lms
