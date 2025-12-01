#ifndef QUANT_FORGE_TIME_UTILS_HPP
#define QUANT_FORGE_TIME_UTILS_HPP

#include <cstdint>
#include <string>

namespace time_utils {
    constexpr int MARK_OPEN_HOUR = 9;
    constexpr int MARKET_CLOSE_HOUR = 16;

    [[nodiscard]] bool is_within_market_hours(const int64_t& timestamp_ns);
}  // namespace time_utils

#endif
