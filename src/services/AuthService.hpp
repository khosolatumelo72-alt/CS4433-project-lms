#pragma once
#include "domain/Ports.hpp"
#include "services/Dtos.hpp"

namespace lms {
class AuthService {
public:
    AuthService(IUserRepository& users, IPasswordHasher& hasher, IClock& clock)
        : users_(users), hasher_(hasher), clock_(clock) {}
    /// FR02. Throws AuthError with one message for an unknown user and a wrong password.
    UserPtr login(const std::string& username, const std::string& password);
    /// FR03.
    void changePassword(int userId, const NewPassword& req);
    void logout() {}
private:
    IUserRepository& users_;
    IPasswordHasher& hasher_;
    IClock& clock_;
};
}  // namespace lms
