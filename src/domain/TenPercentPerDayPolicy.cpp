#include "domain/Ports.hpp"

namespace lms {

double TenPercentPerDayPolicy::penaltyFor(double maxMark, Duration lateBy) {
    constexpr long long kDay = 24 * 60 * 60;
    const long long secs = lateBy.count();
    if (secs <= 0) return 0.0;
    const long long startedDays = (secs + kDay - 1) / kDay;
    if (startedDays > 3) return maxMark;  // zero marks after three days
    return maxMark * 0.10 * static_cast<double>(startedDays);
}

}  // namespace lms
