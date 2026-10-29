#include "services/Validation.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "domain/Errors.hpp"

namespace lms {
namespace validate {

std::string trim(const std::string& s) {
    auto b = std::find_if_not(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
    auto e = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return b < e ? std::string(b, e) : std::string();
}

std::string requireText(const std::string& s, const std::string& field, std::size_t maxLen) {
    std::string t = trim(s);
    if (t.empty()) throw ValidationError(field + " must not be empty");
    if (t.size() > maxLen) throw ValidationError(field + " is too long (maximum " + std::to_string(maxLen) + " characters)");
    return t;
}

std::string requireUsername(const std::string& s) {
    std::string t = trim(s);
    if (t.size() < 3 || t.size() > 32) throw ValidationError("user name must be 3 to 32 characters");
    for (unsigned char c : t)
        if (!std::isalnum(c) && c != '.' && c != '_' && c != '-')
            throw ValidationError("user name may only contain letters, digits, '.', '_' and '-'");
    return t;
}

void requirePassword(const std::string& password, const std::string& username) {
    if (password.size() < 8) throw ValidationError("password must be at least 8 characters");
    if (password.size() > 128) throw ValidationError("password must be at most 128 characters");
    if (password == username) throw ValidationError("password must not be the same as the user name");
}

void requireMark(double value, double max, const std::string& field) {
    if (std::isnan(value) || value < 0 || value > max)
        throw ValidationError(field + " must be between 0 and " + std::to_string(max));
}

}  // namespace validate
}  // namespace lms
