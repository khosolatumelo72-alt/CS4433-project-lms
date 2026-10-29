#include "domain/Ports.hpp"

namespace lms {

// BR01. Every started 24 hours after the deadline costs 10% of the MAXIMUM mark:
//   1 second to 24 hours late  -> 10%      24 hours + 1 second to 48 hours -> 20%
//   48 to 72 hours late        -> 30%      more than 72 hours late         -> the whole mark (final mark 0)
// "Started" means we round up: being 1 second late already counts as one full day.
double TenPercentPerDayPolicy::penaltyFor(double maxMark, Duration lateBy) {
    constexpr long long kDay = 24 * 60 * 60;
    const long long secs = lateBy.count();
    if (secs <= 0) return 0.0;
    const long long startedDays = (secs + kDay - 1) / kDay;  // divide and round up
    if (startedDays > 3) return maxMark;  // zero marks after three days
    return maxMark * 0.10 * static_cast<double>(startedDays);
}

}  // namespace lms
