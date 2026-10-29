#pragma once
#include <chrono>
#include <optional>
#include <string>

namespace lms {

using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::seconds;

enum class Role { Student, Lecturer, HeadOfDepartment, Administrator };
enum class SubmissionStatus { Submitted, Late, Marked, Moderated, Released };
enum class AuditAction { MarkSet, MarkChanged, Approved };

std::string toString(Role r);
std::string toString(SubmissionStatus s);
std::string toString(AuditAction a);
Role roleFromString(const std::string& s);
SubmissionStatus statusFromString(const std::string& s);
AuditAction actionFromString(const std::string& s);

/// ISO-8601 UTC text, e.g. 2026-11-01T12:00:00Z (the format stored in lms.db).
std::string formatTime(TimePoint t);
/// Reads the stored format. Throws ValidationError if the text is not valid.
TimePoint parseTime(const std::string& s);
/// Reads user input of the form "YYYY-MM-DD HH:MM" (UTC). Returns nullopt if invalid.
std::optional<TimePoint> parseUserTime(const std::string& s);
/// Shows a time as "YYYY-MM-DD HH:MM UTC".
std::string displayTime(TimePoint t);

}  // namespace lms
