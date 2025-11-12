#include "report_store.hpp"

#include <mutex>
#include <string>
#include <vector>

#include "../../simulators/back_test/back_test_engine.hpp"
#include "../../simulators/monte_carlo/monte_carlo_engine.hpp"

namespace forge {
    void ReportStore::store_back_test_report(const std::string& plugin_name, const simulators::BackTestReport& report) {
        std::lock_guard<std::mutex> lock(mutex_);

        back_test_reports_[plugin_name] = report;
    }

    void ReportStore::store_monte_carlo_report(const std::string& plugin_name, const simulators::MonteCarloReport& report) {
        std::lock_guard<std::mutex> lock(mutex_);

        monte_carlo_reports_[plugin_name] = report;
    }

    std::vector<simulators::BackTestReport> ReportStore::get_back_test_reports() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<simulators::BackTestReport> reports;
        reports.reserve(back_test_reports_.size());
        for (const auto& [plugin_name, report] : back_test_reports_) {
            reports.emplace_back(report);
        }
        return reports;
    }

    std::vector<simulators::MonteCarloReport> ReportStore::get_monte_carlo_reports() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<simulators::MonteCarloReport> reports;
        reports.reserve(monte_carlo_reports_.size());
        for (const auto& [plugin_name, report] : monte_carlo_reports_) {
            reports.emplace_back(report);
        }
        return reports;
    }
}  // namespace forge
