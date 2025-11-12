#ifndef QUANT_FORGE_REPORT_STORE_HPP
#define QUANT_FORGE_REPORT_STORE_HPP

#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "../../simulators/back_test/back_test_engine.hpp"
#include "../../simulators/monte_carlo/monte_carlo_engine.hpp"

namespace forge {

    class ReportStore {
       public:
        void store_back_test_report(const std::string& plugin_name, const simulators::BackTestReport& report);
        void store_monte_carlo_report(const std::string& plugin_name, const simulators::MonteCarloReport& report);
        [[nodiscard]] std::vector<simulators::BackTestReport> get_back_test_reports() const;
        [[nodiscard]] std::vector<simulators::MonteCarloReport> get_monte_carlo_reports() const;

       private:
        std::unordered_map<std::string, simulators::BackTestReport> back_test_reports_;
        std::unordered_map<std::string, simulators::MonteCarloReport> monte_carlo_reports_;
        mutable std::mutex mutex_;
    };
}  // namespace forge

#endif
