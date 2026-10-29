#pragma once
#include <memory>
#include <optional>
#include <string>

#include "domain/Types.hpp"

namespace lms {

/// Abstract base of the four roles (Class diagram, Figure A).
class User {
public:
    static constexpr int kMaxFailedAttempts = 5;
    static constexpr int kLockMinutes = 5;

    virtual ~User() = default;

    int getId() const { return id_; }
    const std::string& getUsername() const { return username_; }
    const std::string& getPasswordHash() const { return passwordHash_; }
    bool isActive() const { return active_; }
    int getFailedAttempts() const { return failedAttempts_; }
    const std::optional<TimePoint>& getLockedUntil() const { return lockedUntil_; }

    /// Each subclass says which role it is (polymorphism).
    virtual Role role() const = 0;

    bool isLocked(TimePoint now) const;
    /// Counts a wrong password; locks the account after kMaxFailedAttempts in a row.
    void registerFailedLogin(TimePoint now);
    void resetFailedLogins();

    void setId(int id) { id_ = id; }
    void setActive(bool active) { active_ = active; }
    void setPasswordHash(std::string hash) { passwordHash_ = std::move(hash); }

protected:
    User(int id, std::string username, std::string passwordHash, bool active, int failedAttempts,
         std::optional<TimePoint> lockedUntil);

private:
    int id_;
    std::string username_;
    std::string passwordHash_;
    bool active_;
    int failedAttempts_;
    std::optional<TimePoint> lockedUntil_;
};

using UserPtr = std::unique_ptr<User>;

class Administrator final : public User {
public:
    Administrator(int id, std::string username, std::string passwordHash, bool active, int failedAttempts,
        std::optional<TimePoint> lockedUntil)
        : User(id, std::move(username), std::move(passwordHash), active, failedAttempts, lockedUntil) {}
    Role role() const override { return Role::Administrator; }
};

class Lecturer final : public User {
public:
    Lecturer(int id, std::string username, std::string passwordHash, bool active, int failedAttempts,
        std::optional<TimePoint> lockedUntil)
        : User(id, std::move(username), std::move(passwordHash), active, failedAttempts, lockedUntil) {}
    Role role() const override { return Role::Lecturer; }
};

class Student final : public User {
public:
    Student(int id, std::string username, std::string passwordHash, bool active, int failedAttempts,
        std::optional<TimePoint> lockedUntil)
        : User(id, std::move(username), std::move(passwordHash), active, failedAttempts, lockedUntil) {}
    Role role() const override { return Role::Student; }
};

class HeadOfDepartment final : public User {
public:
    HeadOfDepartment(int id, std::string username, std::string passwordHash, bool active,
                     int failedAttempts, std::optional<TimePoint> lockedUntil, int departmentId)
        : User(id, std::move(username), std::move(passwordHash), active, failedAttempts, lockedUntil),
          departmentId_(departmentId) {}
    int getDepartmentId() const { return departmentId_; }
    Role role() const override { return Role::HeadOfDepartment; }

private:
    int departmentId_;
};

/// Everything UserFactory needs to build one user.
struct UserSpec {
    Role role = Role::Student;
    int id = 0;
    std::string username;
    std::string passwordHash;
    bool active = true;
    int failedAttempts = 0;
    std::optional<TimePoint> lockedUntil;
    int departmentId = 0;  // required for HeadOfDepartment only
};

/// Factory pattern: the only place that chooses the User subclass.
class UserFactory {
public:
    static UserPtr create(const UserSpec& spec);
};

}  // namespace lms
