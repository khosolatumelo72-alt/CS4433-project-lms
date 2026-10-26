#include "infrastructure/SystemClock.hpp"
namespace lms {
TimePoint SystemClock::now() {
    // Whole seconds only, so a time written to the database and read back is identical.
    return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
}
}  // namespace lms
