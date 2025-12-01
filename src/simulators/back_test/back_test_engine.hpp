#ifndef QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP
#define QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "../../utils/max_heap.hpp"
#include "../../utils/min_heap.hpp"
#include "./models.hpp"

using namespace money_utils;

namespace simulators {

    struct BackTestReport {
        std::string some_value_;
        std::string another_value_;
    };

    struct ExecutionResult {
        bool success_;
        std::string message_;
        std::optional<Money> cash_delta_;
        std::optional<models::Position> position_;
        std::optional<models::Trade> trade_;
        std::optional<models::Instruction> remaining_instruction_;

        [[nodiscard]] bool is_filled() const { return remaining_instruction_ == std::nullopt; }

        // Error Case Constructor
        ExecutionResult(std::string message) : success_(false), message_(std::move(message)) {}

        // Success Case Constructor
        ExecutionResult(bool success, std::string message, Money cash_delta, std::optional<models::Instruction> remaining_instruction,
                        models::Position position, models::Trade trade)
            : success_(success),
              message_(std::move(message)),
              cash_delta_(cash_delta),
              position_(std::move(position)),
              trade_(std::move(trade)),
              remaining_instruction_(std::move(remaining_instruction)) {}
    };

    class SlippageCalculator {
       public:
        [[nodiscard]] static int64_t calculate_slippage_time_ns(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                                const models::BackTestState& state);
    };

    class Exchange {
       public:
        [[nodiscard]] static bool is_within_market_hour_restrictions(int64_t timestamp_ns, const plugins::manifest::HostParams& host_params);
        [[nodiscard]] static Money calculate_commision(const models::Trade& trade, const plugins::manifest::HostParams& host_params);
    };

    class Executor {
       public:
        [[nodiscard]] static ExecutionResult execute_order(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                           const models::BackTestState& state);
        [[nodiscard]] static ExecutionResult execute_signal(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                            const models::BackTestState& state);
        [[nodiscard]] static ExecutionResult execute_sell(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                          const models::BackTestState& state);
        [[nodiscard]] static std::pair<double, double> get_fillable_and_remaining_quantities(const models::Instruction& instruction,
                                                                                             const plugins::manifest::HostParams& host_params,
                                                                                             const models::BackTestState& state);
    };

    class PositionCalculator {
       public:
        [[nodiscard]] static double calculate_signal_position_size(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                                   const models::BackTestState& state);
        [[nodiscard]] static std::optional<Money> calculate_signal_stop_loss_price(const models::Instruction& instruction,
                                                                                   const plugins::manifest::HostParams& host_params,
                                                                                   const models::BackTestState& state);
        [[nodiscard]] static std::optional<Money> calculate_signal_take_profit_price(const models::Instruction& instruction,
                                                                                     const plugins::manifest::HostParams& host_params,
                                                                                     const models::BackTestState& state);
    };

    class RiskManager {
       public:
        [[nodiscard]] static bool is_at_stop_loss(const models::Trade& trade, const models::BackTestState& state);
        [[nodiscard]] static bool is_at_take_profit(const models::Trade& trade, const models::BackTestState& state);
        [[nodiscard]] static bool has_stop_loss(const models::Trade& trade);
        [[nodiscard]] static bool has_take_profit(const models::Trade& trade);
    };

    class EquityCalculator {
       public:
        [[nodiscard]] static Money calculate_equity(const models::BackTestState& state);
        [[nodiscard]] static double calculate_return(const plugins::manifest::HostParams& host_params, Money equity);
        [[nodiscard]] static double calculate_max_drawdown(const models::BackTestState& state, Money equity);
    };

    class BackTestEngine {
       public:
        BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store);
        void run();
        void empty_instruction_heap(const http::stock_api::AggregateBarResult& bar, const plugins::manifest::HostParams& host_params);
        void process_plugin_instructions_to_heap(const PluginResult& result, const plugins::manifest::HostParams& host_params);
        void empty_stop_loss_heap(const plugins::manifest::HostParams& host_params);
        void empty_take_profit_heap(const plugins::manifest::HostParams& host_params);
        [[nodiscard]] const BackTestReport& get_report();
        void resolve_execution(const ExecutionResult& execution_result, const plugins::manifest::HostParams& host_params);

       private:
        const plugins::loaders::IPluginLoader* plugin_;
        const forge::DataStore* data_store_;
        BackTestReport report_;
        models::BackTestState state_ = {
            .cash_ = Money(0),
            .positions_ = {},
            .current_prices_ = {},
            .current_volumes_ = {},
            .current_timestamp_ns_ = 0,
            .trade_history_ = {},
            .equity_curve_ = {},
        };
        data_structures::MinHeap<models::Instruction> instruction_heap_;
        data_structures::MinHeap<models::Trade> stop_loss_heap_;
        data_structures::MaxHeap<models::Trade> take_profit_heap_;
    };
}  // namespace simulators

#endif
