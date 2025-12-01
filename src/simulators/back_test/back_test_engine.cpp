#include "back_test_engine.hpp"

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "../../utils/money_utils.hpp"
#include "../../utils/time_utils.hpp"
#include "./models.hpp"
#include "utils/constants.hpp"

using namespace money_utils;

namespace simulators {

    BackTestEngine::BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store)
        : plugin_(plugin), data_store_(data_store) {}

    void BackTestEngine::run() {
        const auto& host_params = plugin_->get_host_params();

        state_.cash_ = Money(host_params.initial_capital_);

        auto iterable_plugin_data = data_store_->get_iterable_plugin_data(plugin_->get_plugin_name());

        std::for_each(iterable_plugin_data.begin(), iterable_plugin_data.end(), [&, host_params](const auto& bar) {
            if (!Exchange::is_within_market_hour_restrictions(bar.unix_ts_ns_, host_params)) {
                return;
            }

            PluginResult result = plugin_->on_bar(bar, state_);

            if (result.code_ != 0) {
                throw std::runtime_error("Plugin on_bar failed: " + std::string(result.message_));
            }

            empty_instruction_heap(bar, host_params);

            process_plugin_instructions_to_heap(result, host_params);

            empty_stop_loss_heap(host_params);
            empty_take_profit_heap(host_params);
        });

        const char* json_out = nullptr;
        PluginResult result = plugin_->on_end(&json_out);

        if (result.code_ != 0) {
            throw std::runtime_error("Plugin on_end failed: " + std::string(result.message_));
        }

        plugin_->free_string(json_out);

        report_ = {
            .some_value_ = "some_value",
            .another_value_ = "another_value",
        };
    }

    void BackTestEngine::empty_instruction_heap(const http::stock_api::AggregateBarResult& bar, const plugins::manifest::HostParams& host_params) {
        while (!instruction_heap_.empty()) {
            auto top_instruction_optional = instruction_heap_.top();

            if (!top_instruction_optional.has_value()) {
                break;
            }

            auto instruction = top_instruction_optional.value();

            if (instruction.filled_at_ns_ > bar.unix_ts_ns_) {
                break;
            }

            instruction_heap_.pop();

            auto execution_result = instruction.is_signal() ? Executor::execute_signal(instruction, host_params, state_)
                                                            : Executor::execute_order(instruction, host_params, state_);

            if (execution_result.trade_.has_value()) {
                models::Trade trade = execution_result.trade_.value();
                if (trade.stop_loss_price_.has_value()) {
                    stop_loss_heap_.push(trade);
                }

                if (trade.take_profit_price_.has_value()) {
                    take_profit_heap_.push(trade);
                }
            }

            if (execution_result.remaining_instruction_.has_value()) {
                auto mutable_instruction = execution_result.remaining_instruction_.value();
                mutable_instruction.filled_at_ns_ =
                    SlippageCalculator::calculate_slippage_time_ns(execution_result.remaining_instruction_.value(), host_params, state_);
                instruction_heap_.push(mutable_instruction);
            }

            resolve_execution(execution_result, host_params);
        }
    }

    void BackTestEngine::process_plugin_instructions_to_heap(const PluginResult& result, const plugins::manifest::HostParams& host_params) {
        auto instructions = models::Instruction::to_instructions(result);
        for (const auto& instruction : instructions) {
            auto mutable_instruction = instruction;
            mutable_instruction.filled_at_ns_ = SlippageCalculator::calculate_slippage_time_ns(instruction, host_params, state_);
            instruction_heap_.push(mutable_instruction);
        }
    }

    void BackTestEngine::empty_stop_loss_heap(const plugins::manifest::HostParams& host_params) {
        while (!stop_loss_heap_.empty()) {
            auto top_trade_opt = stop_loss_heap_.top();

            if (!top_trade_opt.has_value()) {
                break;
            }

            auto stop_loss_trade = top_trade_opt.value();

            if (!stop_loss_trade.stop_loss_price_.has_value() ||
                stop_loss_trade.stop_loss_price_.value() < state_.current_prices_.at(stop_loss_trade.symbol_)) {
                break;
            }

            stop_loss_heap_.pop();

            auto sell_instruction = stop_loss_trade.to_sell_instruction();
            auto mutable_instruction = sell_instruction;
            mutable_instruction.filled_at_ns_ = SlippageCalculator::calculate_slippage_time_ns(sell_instruction, host_params, state_);
            instruction_heap_.push(mutable_instruction);
        }
    }

    void BackTestEngine::empty_take_profit_heap(const plugins::manifest::HostParams& host_params) {
        while (!take_profit_heap_.empty()) {
            auto top_trade_opt = take_profit_heap_.top();

            if (!top_trade_opt.has_value()) {
                break;
            }

            auto take_profit_trade = top_trade_opt.value();

            if (!take_profit_trade.stop_loss_price_.has_value() ||
                take_profit_trade.stop_loss_price_.value() > state_.current_prices_.at(take_profit_trade.symbol_)) {
                break;
            }

            take_profit_heap_.pop();

            auto sell_instruction = take_profit_trade.to_sell_instruction();
            auto mutable_instruction = sell_instruction;
            mutable_instruction.filled_at_ns_ = SlippageCalculator::calculate_slippage_time_ns(sell_instruction, host_params, state_);
            instruction_heap_.push(mutable_instruction);
        }
    }

    const BackTestReport& BackTestEngine::get_report() { return report_; }

    void BackTestEngine::resolve_execution(const ExecutionResult& execution_result, const plugins::manifest::HostParams& host_params) {
        if (!execution_result.success_) {
            throw std::runtime_error("Execution failed: " + execution_result.message_);
        }

        if (execution_result.cash_delta_.has_value() && execution_result.position_.has_value() && execution_result.trade_.has_value()) {
            state_.cash_ += execution_result.cash_delta_.value();

            state_.positions_[execution_result.position_.value().symbol_] = execution_result.position_.value();

            state_.trade_history_.emplace_back(execution_result.trade_.value());
            state_.current_timestamp_ns_ = execution_result.trade_.value().timestamp_ns_;
            state_.current_prices_[execution_result.trade_.value().symbol_] = execution_result.trade_.value().price_;

            auto equity = EquityCalculator::calculate_equity(state_);

            // We need a configurable rolling window, and a configurable risk free rate (0.02 default)
            state_.equity_curve_.emplace_back(models::EquitySnapshot{
                .timestamp_ns_ = execution_result.trade_.value().timestamp_ns_,
                .equity_ = equity,
                .return_ = EquityCalculator::calculate_return(host_params, equity),
                .max_drawdown_ = EquityCalculator::calculate_max_drawdown(state_, equity),
                .sharpe_ratio_ = 0,
                .sharpe_ratio_rolling_ = 0,
                .sortino_ratio_ = 0,
                .sortino_ratio_rolling_ = 0,
                .calmar_ratio_ = 0,
                .calmar_ratio_rolling_ = 0,
                .tail_ratio_ = 0,
                .tail_ratio_rolling_ = 0,
                .value_at_risk_ = 0,
                .value_at_risk_rolling_ = 0,
                .conditional_value_at_risk_ = 0,
                .conditional_value_at_risk_rolling_ = 0,
            });
        }
    }

    Money EquityCalculator::calculate_equity(const models::BackTestState& state) {
        auto total_assets = Money(0);
        for (const auto& [_, position] : state.positions_) {
            total_assets += state.current_prices_.at(position.symbol_) * position.quantity_;
        }
        return total_assets + state.cash_;
    }

    double EquityCalculator::calculate_return(const plugins::manifest::HostParams& host_params, Money equity) {
        auto initial_capital_money = Money(host_params.initial_capital_);
        auto net_return = equity - initial_capital_money;
        return net_return.to_dollars() / initial_capital_money.to_dollars();
    }

    double EquityCalculator::calculate_max_drawdown(const models::BackTestState& state, Money equity) {
        Money peak_equity = equity;
        for (const auto& equity_snapshot : state.equity_curve_) {
            if (equity_snapshot.equity_ > peak_equity) {
                peak_equity = equity_snapshot.equity_;
            }
        }
        return (peak_equity - equity).to_dollars() / peak_equity.to_dollars();
    }

    bool Exchange::is_within_market_hour_restrictions(int64_t timestamp_ns, const plugins::manifest::HostParams& host_params) {
        if (host_params.market_hours_only_ != true) {
            return true;
        }

        return time_utils::is_within_market_hours(timestamp_ns);
    }

    Money Exchange::calculate_commision(const models::Trade& trade, const plugins::manifest::HostParams& host_params) {
        const std::string commission_type = host_params.commission_type_.value_or(std::string(""));
        const double commission_value = host_params.commission_.value_or(0.0);

        if (commission_type.empty() || commission_value == 0.0) {
            return Money(0);
        }

        if (commission_type == "per_share") {
            return Money::from_dollars(commission_value) * trade.quantity_;
        }

        if (commission_type == "percentage") {
            Money trade_value = trade.price_ * trade.quantity_;
            return trade_value * commission_value;
        }

        if (commission_type == "flat") {
            return Money::from_dollars(commission_value);
        }

        return Money(0);
    }

    std::pair<double, double> Executor::get_fillable_and_remaining_quantities(const models::Instruction& instruction,
                                                                              const plugins::manifest::HostParams& host_params,
                                                                              const models::BackTestState& state) {
        if (host_params.fill_max_pct_of_volume_ != std::nullopt) {
            auto symbol_cash = state.current_prices_.at(instruction.symbol_) * instruction.quantity_;
            auto fill_pct = symbol_cash.to_dollars() / static_cast<double>(state.current_volumes_.at(instruction.symbol_));

            if (host_params.fill_max_pct_of_volume_.has_value() && fill_pct > host_params.fill_max_pct_of_volume_.value()) {
                auto max_fill_quantity = static_cast<double>(state.current_volumes_.at(instruction.symbol_)) * host_params.fill_max_pct_of_volume_.value();
                auto remaining_quantity = instruction.quantity_ - max_fill_quantity;
                return {max_fill_quantity, remaining_quantity};
            }
        }
        return {instruction.quantity_, 0};
    }

    ExecutionResult Executor::execute_order(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                            const models::BackTestState& state) {
        auto [fillable_quantity, remaining_quantity] = Executor::get_fillable_and_remaining_quantities(instruction, host_params, state);

        if (!host_params.allow_fractional_shares_.has_value()) {
            fillable_quantity = std::floor(fillable_quantity);
            if (fillable_quantity <= 0) {
                return {"Order quantity is too small to execute"};
            }
        }

        models::Trade trade(instruction.symbol_, fillable_quantity, state.current_prices_.at(instruction.symbol_), state.current_timestamp_ns_,
                            instruction.stop_loss_price_, instruction.take_profit_price_);

        auto commission = Exchange::calculate_commision(trade, host_params);

        auto trade_value = trade.price_ * fillable_quantity;
        auto cash_delta = [&]() {
            if (instruction.action_ == constants::BUY) {
                return (trade_value + commission) * -1;
            }

            if (instruction.action_ == constants::SELL) {
                return trade_value - commission;
            }

            return Money(0);
        }();

        auto post_trade_cash = state.cash_ + cash_delta;

        if (post_trade_cash.to_dollars() < 0) {
            return {"Insufficient funds for trade and commission"};
        }

        models::Position position = state.positions_.at(instruction.symbol_);
        position.symbol_ = instruction.symbol_;
        if (instruction.action_ == constants::BUY) {
            position.quantity_ += fillable_quantity;
        } else if (instruction.action_ == constants::SELL) {
            position.quantity_ -= fillable_quantity;
        }
        std::vector<models::Trade> symbol_trades;
        for (const auto& existing_trade : state.trade_history_) {
            if (existing_trade.symbol_ == instruction.symbol_) {
                symbol_trades.emplace_back(existing_trade);
            }
        }
        position.average_price_ =
            (trade.price_ + (position.average_price_ * static_cast<double>(symbol_trades.size()))) / (static_cast<double>(symbol_trades.size()) + 1);

        if (remaining_quantity > 0) {
            models::Instruction remaining_instruction = instruction;
            remaining_instruction.quantity_ = remaining_quantity;
            remaining_instruction.created_at_ns_ = state.current_timestamp_ns_;
            return {true, "Partial fill", cash_delta, std::make_optional(remaining_instruction), position, trade};
        }

        return {true, "Execution allowed", Money(0), std::nullopt, position, trade};
    }

    ExecutionResult Executor::execute_signal(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                             const models::BackTestState& state) {
        auto enriched_instruction = instruction;
        enriched_instruction.quantity_ = PositionCalculator::calculate_signal_position_size(instruction, host_params, state);
        enriched_instruction.order_type_ = constants::MARKET;
        enriched_instruction.stop_loss_price_ = PositionCalculator::calculate_signal_stop_loss_price(instruction, host_params, state);
        enriched_instruction.take_profit_price_ = PositionCalculator::calculate_signal_take_profit_price(instruction, host_params, state);

        return execute_order(enriched_instruction, host_params, state);
    }

    double PositionCalculator::calculate_signal_position_size(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                              const models::BackTestState& state) {
        Money current_price = state.current_prices_.at(instruction.symbol_);
        Money equity = EquityCalculator::calculate_equity(state);

        std::string sizing_method = host_params.position_sizing_method_.value_or("fixed_percentage");
        double position_size_value = host_params.position_size_value_.value_or(constants::DEFAULT_POSITION_SIZE_VALUE);

        double quantity = 0;

        if (sizing_method == "fixed_percentage") {
            Money dollar_amount = equity * position_size_value;
            quantity = dollar_amount.to_dollars() / current_price.to_dollars();
        }

        if (sizing_method == "fixed_dollar") {
            quantity = position_size_value / current_price.to_dollars();
        }

        if (sizing_method == "equal_weight") {
            size_t symbol_count = host_params.symbols_.size();
            Money dollar_per_symbol = equity / static_cast<double>(symbol_count);
            quantity = dollar_per_symbol.to_dollars() / current_price.to_dollars();
        }

        if (host_params.max_position_size_.has_value() && quantity > host_params.max_position_size_.value()) {
            quantity = host_params.max_position_size_.value();
        }

        return quantity;
    }

    std::optional<Money> PositionCalculator::calculate_signal_stop_loss_price(const models::Instruction& instruction,
                                                                              const plugins::manifest::HostParams& host_params,
                                                                              const models::BackTestState& state) {
        if (!host_params.use_stop_loss_.value_or(false)) {
            return std::nullopt;
        }

        Money current_price = state.current_prices_.at(instruction.symbol_);

        if (host_params.stop_loss_pct_ == std::nullopt) {
            return std::nullopt;
        }

        if (instruction.action_ == constants::BUY) {
            return current_price * (1.0 - host_params.stop_loss_pct_.value());
        }

        if (instruction.action_ == constants::SELL) {
            return current_price * (1.0 + host_params.stop_loss_pct_.value());
        }

        return std::nullopt;
    }

    std::optional<Money> PositionCalculator::calculate_signal_take_profit_price(const models::Instruction& instruction,
                                                                                const plugins::manifest::HostParams& host_params,
                                                                                const models::BackTestState& state) {
        if (!host_params.use_take_profit_.value_or(false)) {
            return std::nullopt;
        }

        Money current_price = state.current_prices_.at(instruction.symbol_);

        if (host_params.take_profit_pct_ == std::nullopt) {
            return std::nullopt;
        }

        if (instruction.action_ == constants::BUY) {
            return current_price * (1.0 + host_params.take_profit_pct_.value());
        }

        if (instruction.action_ == constants::SELL) {
            return current_price * (1.0 - host_params.take_profit_pct_.value());
        }

        return std::nullopt;
    }

    int64_t SlippageCalculator::calculate_slippage_time_ns(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                           const models::BackTestState& state) {
        if (!host_params.slippage_model_.has_value() || host_params.slippage_model_.value() == "none") {
            return 0;
        }

        if (host_params.slippage_model_.value() == "time_based") {
            int64_t base_delay_ns = static_cast<int64_t>(host_params.slippage_.value_or(0.0) * constants::MONEY_SCALED_BASE);
            return base_delay_ns;
        }

        if (host_params.slippage_model_.value() == "time_volume_based") {
            const auto order_value = state.current_prices_.at(instruction.symbol_) * instruction.quantity_;
            const auto volume_value = state.current_volumes_.at(instruction.symbol_);
            const double size_ratio = static_cast<double>(volume_value) / order_value.to_dollars();

            const double delay_seconds = host_params.slippage_.value_or(1.0) * size_ratio;
            return static_cast<int64_t>(delay_seconds * constants::MONEY_SCALED_BASE);
        }

        return 0;
    }

}  // namespace simulators
