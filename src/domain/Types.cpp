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

namespace {
// Portable UTC calendar maths (Howard Hinnant's civil-date algorithms), so the build
// does not depend on the POSIX-only timegm and gmtime_r.
long long daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}
void civilFromDays(long long z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += m <= 2;
}
}  // namespace

std::string formatTime(TimePoint t) {
    const long long secs = std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
    long long days = secs / 86400, rem = secs % 86400;
    if (rem < 0) { rem += 86400; --days; }
    int y; unsigned mo, d;
    civilFromDays(days, y, mo, d);
    char buf[64];
    std::snprintf(buf, sizeof buf, "%04d-%02u-%02uT%02d:%02d:%02dZ", y, mo, d, static_cast<int>(rem / 3600),
                  static_cast<int>(rem % 3600 / 60), static_cast<int>(rem % 60));
    return buf;
}

namespace {
bool toTime(int y, int mo, int d, int h, int mi, int s, TimePoint& out) {
    if (y < 1970 || y > 9999 || mo < 1 || mo > 12 || d < 1 || d > 31 || h < 0 || h > 23 || mi < 0 ||
        mi > 59 || s < 0 || s > 59)
        return false;
    static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    if (d > kDays[mo - 1] + (mo == 2 && leap ? 1 : 0)) return false;  // e.g. 31 February
    const long long secs = daysFromCivil(y, static_cast<unsigned>(mo), static_cast<unsigned>(d)) * 86400 +
                           h * 3600 + mi * 60 + s;
    out = TimePoint(std::chrono::seconds(secs));
    return true;
}
}  // namespace

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
