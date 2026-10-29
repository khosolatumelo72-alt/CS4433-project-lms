#pragma once
#include <stdexcept>
#include <string>

namespace lms {

/// Base class of every error the program raises on purpose.
class LmsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
/// Input that is missing, malformed or out of range.
class ValidationError : public LmsError { public: using LmsError::LmsError; };
/// Wrong user name or password, locked or disabled account.
class AuthError : public LmsError { public: using LmsError::LmsError; };
/// The user's role or ownership does not allow the action.
class PermissionError : public LmsError { public: using LmsError::LmsError; };
/// The requested record does not exist.
class NotFoundError : public LmsError { public: using LmsError::LmsError; };
/// A business rule (BR01, BR2-BR6, resubmission rule, status order) forbids the action.
class RuleError : public LmsError { public: using LmsError::LmsError; };
/// SQLite reported an error.
class DatabaseError : public LmsError { public: using LmsError::LmsError; };

}  // namespace lms
