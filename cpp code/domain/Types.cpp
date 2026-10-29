#include "domain/Types.hpp"

#include <cstdio>
#include <ctime>

#include "domain/Errors.hpp"

namespace lms {

std::string toString(Role r) {
    switch (r) {
        case Role::Student: return "Student";
        case Role::Lecturer: return "Lecturer";
        case Role::HeadOfDepartment: return "HeadOfDepartment";
        case Role::Administrator: return "Administrator";
    }
    throw ValidationError("unknown role");
}
std::string toString(SubmissionStatus s) {
    switch (s) {
        case SubmissionStatus::Submitted: return "Submitted";
        case SubmissionStatus::Late: return "Late";
        case SubmissionStatus::Marked: return "Marked";
        case SubmissionStatus::Moderated: return "Moderated";
        case SubmissionStatus::Released: return "Released";
    }
    throw ValidationError("unknown status");
}
std::string toString(AuditAction a) {
    switch (a) {
        case AuditAction::MarkSet: return "MarkSet";
        case AuditAction::MarkChanged: return "MarkChanged";
        case AuditAction::Approved: return "Approved";
    }
    throw ValidationError("unknown audit action");
}
Role roleFromString(const std::string& s) {
    for (Role r : {Role::Student, Role::Lecturer, Role::HeadOfDepartment, Role::Administrator})
        if (toString(r) == s) return r;
    throw ValidationError("unknown role: " + s);
}
SubmissionStatus statusFromString(const std::string& s) {
    for (auto v : {SubmissionStatus::Submitted, SubmissionStatus::Late, SubmissionStatus::Marked,
                   SubmissionStatus::Moderated, SubmissionStatus::Released})
        if (toString(v) == s) return v;
    throw ValidationError("unknown status: " + s);
}
AuditAction actionFromString(const std::string& s) {
    for (auto v : {AuditAction::MarkSet, AuditAction::MarkChanged, AuditAction::Approved})
        if (toString(v) == s) return v;
    throw ValidationError("unknown audit action: " + s);
}

// ---- Dates and times -------------------------------------------------------------------------
// All times are UTC. A TimePoint counts seconds since 1 January 1970. To turn that into a calendar
// date (and back) we count days with simple loops. The loops are easy to follow and fast enough,
// because years only go from 1970 to 9999. We avoid the C functions timegm and gmtime_r because
// they do not exist on every operating system (Windows lacks them).
namespace {

const int kSecondsPerDay = 24 * 60 * 60;

bool isLeapYear(int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}

// How many whole days lie between 1970-01-01 and the given date.
long long daysSince1970(int year, int month, int day) {
    long long days = 0;
    for (int y = 1970; y < year; ++y) days += isLeapYear(y) ? 366 : 365;
    for (int m = 1; m < month; ++m) days += daysInMonth(year, m);
    return days + (day - 1);
}

}  // namespace

// Writes a time as text such as 2026-11-01T12:00:00Z (the format stored in the database).
std::string formatTime(TimePoint t) {
    long long seconds = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
    if (seconds < 0) seconds = 0;  // times before 1970 are never stored

    long long days = seconds / kSecondsPerDay;
    int secondsToday = static_cast<int>(seconds % kSecondsPerDay);

    // Walk forward from 1970 one year, then one month, at a time until the days run out.
    int year = 1970;
    while (days >= (isLeapYear(year) ? 366 : 365)) {
        days -= isLeapYear(year) ? 366 : 365;
        ++year;
    }
    int month = 1;
    while (days >= daysInMonth(year, month)) {
        days -= daysInMonth(year, month);
        ++month;
    }
    int day = static_cast<int>(days) + 1;

    char buf[64];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02dZ", year, month, day, secondsToday / 3600,
                  secondsToday % 3600 / 60, secondsToday % 60);
    return buf;
}

namespace {

// Builds a TimePoint from calendar parts. Returns false if the date does not exist
// (for example 31 February, or month 13).
bool toTime(int year, int month, int day, int hour, int minute, int second, TimePoint& out) {
    if (year < 1970 || year > 9999 || month < 1 || month > 12) return false;
    if (day < 1 || day > daysInMonth(year, month)) return false;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;

    long long seconds = daysSince1970(year, month, day) * kSecondsPerDay + hour * 3600 + minute * 60 + second;
    out = TimePoint(std::chrono::seconds(seconds));
    return true;
}

}  // namespace

// Reads the database format 2026-11-01T12:00:00Z. Anything else means the data is damaged.
TimePoint parseTime(const std::string& s) {
    int y, mo, d, h, mi, sec;
    char tail = 0;
    if (std::sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%c", &y, &mo, &d, &h, &mi, &sec, &tail) != 7 ||
        tail != 'Z')
        throw ValidationError("invalid stored time: " + s);
    TimePoint out;
    if (!toTime(y, mo, d, h, mi, sec, out)) throw ValidationError("invalid stored time: " + s);
    return out;
}

std::optional<TimePoint> parseUserTime(const std::string& s) {
    int y, mo, d, h, mi;
    int consumed = 0;
    if (std::sscanf(s.c_str(), "%4d-%2d-%2d %2d:%2d%n", &y, &mo, &d, &h, &mi, &consumed) != 5)
        return std::nullopt;
    for (std::size_t i = static_cast<std::size_t>(consumed); i < s.size(); ++i)
        if (s[i] != ' ') return std::nullopt;
    TimePoint out;
    if (!toTime(y, mo, d, h, mi, 0, out)) return std::nullopt;
    return out;
}

std::string displayTime(TimePoint t) {
    std::string iso = formatTime(t);  // 2026-11-01T12:00:00Z
    return iso.substr(0, 10) + " " + iso.substr(11, 5) + " UTC";
}

}  // namespace lms
