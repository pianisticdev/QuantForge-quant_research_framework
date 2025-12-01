#ifndef QUANT_UTILS_MONEY_UTILS_HPP
#define QUANT_UTILS_MONEY_UTILS_HPP

#include <cstdint>
#include <string>

#include "constants.hpp"

namespace money_utils {
    class Money {
       private:
        int64_t microdollars_;

       public:
        explicit Money(int64_t microdollars) : microdollars_(microdollars) {}

        [[nodiscard]] static Money from_dollars(double dollars) { return Money(static_cast<int64_t>(std::round(dollars * constants::MONEY_SCALED_BASE))); }
        [[nodiscard]] static Money parse_microdollars(const std::string& money_string);

        // Conversion Methods for ABI
        [[nodiscard]] int64_t to_microdollars() const { return microdollars_; }
        [[nodiscard]] double to_dollars() const { return static_cast<double>(microdollars_) / constants::MONEY_SCALED_BASE; }

        // Conversion Methods for C ABI (context dependent)
        [[nodiscard]] int64_t to_abi_int64() const { return microdollars_; }
        [[nodiscard]] double to_abi_double() const { return to_dollars(); }

        // Arithmetic Operators
        [[nodiscard]] Money operator+(Money other) const { return Money(microdollars_ + other.microdollars_); }
        [[nodiscard]] Money operator-(Money other) const { return Money(microdollars_ - other.microdollars_); }
        Money operator+=(Money other) {
            microdollars_ += other.microdollars_;
            return *this;
        }
        Money operator-=(Money other) {
            microdollars_ -= other.microdollars_;
            return *this;
        }
        [[nodiscard]] Money operator*(double scalar) const { return Money(static_cast<int64_t>(std::round(static_cast<double>(microdollars_) * scalar))); }
        [[nodiscard]] Money operator/(double scalar) const { return Money(static_cast<int64_t>(std::round(static_cast<double>(microdollars_) / scalar))); }

        // Comparison operators
        bool operator<(Money other) const { return microdollars_ < other.microdollars_; }
        bool operator<=(Money other) const { return microdollars_ <= other.microdollars_; }
        bool operator>(Money other) const { return microdollars_ > other.microdollars_; }
        bool operator>=(Money other) const { return microdollars_ >= other.microdollars_; }
        bool operator==(Money other) const { return microdollars_ == other.microdollars_; }
    };
}  // namespace money_utils

#endif
