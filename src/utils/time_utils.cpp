#include "time_utils.hpp"

#include <chrono>
#include <string>

namespace time_utils {
    bool is_within_market_hours(const int64_t& timestamp_ns) {
        auto tp = std::chrono::system_clock::time_point(duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(timestamp_ns)));
        auto dp = floor<std::chrono::days>(tp);

        std::chrono::weekday wd{dp};
        auto tod = std::chrono::hh_mm_ss(tp - dp);
        auto hour = static_cast<int>(tod.hours().count());

        if (wd == std::chrono::Sunday || wd == std::chrono::Saturday) {
            return false;
        }

        if (hour < MARK_OPEN_HOUR || hour >= MARKET_CLOSE_HOUR) {
            return false;
        }

        return true;
    }
}  // namespace time_utils
