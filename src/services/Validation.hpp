#pragma once
#include <string>

namespace lms {
namespace validate {

/// Removes spaces at both ends.
std::string trim(const std::string& s);
/// Throws ValidationError if the text is empty or longer than maxLen after trimming. Returns the trimmed text.
std::string requireText(const std::string& s, const std::string& field, std::size_t maxLen);
/// 3-32 characters: letters, digits, dot, underscore, hyphen.
std::string requireUsername(const std::string& s);
/// At least 8 characters, at most 128, and not the same as the user name.
void requirePassword(const std::string& password, const std::string& username);
/// Number from 0 to max inclusive (throws for NaN too).
void requireMark(double value, double max, const std::string& field);

}  // namespace validate
}  // namespace lms
