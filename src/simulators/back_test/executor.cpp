#include "./executor.hpp"

#include "../../utils/constants.hpp"
#include "./equity_calculator.hpp"
#include "./exchange.hpp"
#include "./position_calculator.hpp"
#include "./state.hpp"

namespace simulators {
    std::pair<double, double> Executor::get_fillable_and_remaining_quantities(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                              const simulators::State& state) {
        if (host_params.fill_max_pct_of_volume_.has_value()) {
            auto bar_volume = static_cast<double>(state.current_bar_volumes_.at(order.symbol_));
            auto max_fill_quantity = bar_volume * host_params.fill_max_pct_of_volume_.value();

            if (order.quantity_ > max_fill_quantity) {
                auto remaining_quantity = order.quantity_ - max_fill_quantity;
                return {max_fill_quantity, remaining_quantity};
            }
        }

        return {order.quantity_, 0};
    }

    models::ExecutionResult Executor::execute_order(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                    const simulators::State& state) {
        if (order.quantity_ <= 0) {
            return models::ExecutionResultError("Order quantity must be positive");
        }

        if (state.current_bar_prices_.find(order.symbol_) == state.current_bar_prices_.end()) {
            return models::ExecutionResultError("No price data for symbol: " + order.symbol_);
        }

        if (state.current_bar_volumes_.find(order.symbol_) == state.current_bar_volumes_.end()) {
            return models::ExecutionResultError("No volume data for symbol: " + order.symbol_);
        }

        if (order.is_exit_order_ && order.source_fill_uuid_.has_value()) {
            const auto& uuid = order.source_fill_uuid_.value();
            bool fill_still_active = state.active_buy_fills_.contains(uuid) || state.active_sell_fills_.contains(uuid);

            if (!fill_still_active) {
                return models::ExecutionResultError("Exit order source fill no longer active - skipping");
            }
        }

        auto [fillable_quantity, remaining_quantity] = Executor::get_fillable_and_remaining_quantities(order, host_params, state);

        if (!host_params.allow_fractional_shares_.value_or(false)) {
            fillable_quantity = std::floor(fillable_quantity);
            if (fillable_quantity <= 0) {
                return models::ExecutionResultError("Order quantity is too small to execute");
            }
        }

        Money fill_price = calculate_fill_price(order, state);

        models::Fill fill(order.symbol_, order.action_, fillable_quantity, fill_price, state.current_timestamp_ns_);

        auto pos_it = state.positions_.find(order.symbol_);
        double current_position_quantity = (pos_it != state.positions_.end()) ? pos_it->second.quantity_ : 0.0;
        double new_position_quantity = current_position_quantity + (order.is_buy() ? fillable_quantity : -fillable_quantity);

        auto position_opening_quantity = calculate_position_opening_quantity(order, fillable_quantity, current_position_quantity, new_position_quantity);

        auto exit_orders = create_exit_orders(order, fill, state, position_opening_quantity, new_position_quantity);

        auto commission = Exchange::calculate_commision(fill, host_params);

        auto cash_delta = calculate_cash_delta(order, fill.price_, fillable_quantity, commission);

        auto validation_error =
            validate_margin(order, fill_price, commission, host_params, state, position_opening_quantity, new_position_quantity, cash_delta);
        if (validation_error.has_value()) {
            return models::ExecutionResultError(validation_error.value());
        }

        auto position = PositionCalculator::calculate_position(order, fillable_quantity, fill_price, state);

        if (remaining_quantity > 0) {
            models::Order partial_order = order;
            partial_order.quantity_ = remaining_quantity;
            partial_order.created_at_ns_ = state.current_timestamp_ns_;
            return models::ExecutionResultSuccess(cash_delta, std::make_optional(partial_order), position, fill, exit_orders);
        }

        return models::ExecutionResultSuccess(cash_delta, std::nullopt, position, fill, exit_orders);
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    double Executor::calculate_position_opening_quantity(const models::Order& order, double fillable_quantity, double current_position_quantity,
                                                         double new_position_quantity) {
        if (order.is_buy()) {
            if (current_position_quantity >= 0) {
                return fillable_quantity;
            }

            return std::max(0.0, new_position_quantity);
        }

        if (current_position_quantity <= 0) {
            return fillable_quantity;
        }

        return std::max(0.0, -new_position_quantity);
    }

    std::vector<models::ExitOrder> Executor::create_exit_orders(const models::Order& order, const models::Fill& fill, const simulators::State& state,
                                                                // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                                                                double position_opening_quantity, double new_position_quantity) {
        if (position_opening_quantity <= constants::EPSILON) {
            return {};
        }

        bool is_short_position_fill = order.is_sell() && new_position_quantity <= 0;

        std::vector<models::ExitOrder> exit_orders;
        if (order.stop_loss_price_.has_value()) {
            exit_orders.emplace_back(std::in_place_type<models::StopLossExitOrder>, order.symbol_, position_opening_quantity, order.stop_loss_price_.value(),
                                     fill.price_, state.current_timestamp_ns_, fill.uuid_, is_short_position_fill);
        }
        if (order.take_profit_price_.has_value()) {
            exit_orders.emplace_back(std::in_place_type<models::TakeProfitExitOrder>, order.symbol_, position_opening_quantity,
                                     order.take_profit_price_.value(), fill.price_, state.current_timestamp_ns_, fill.uuid_, is_short_position_fill);
        }
        return exit_orders;
    }

    Money Executor::calculate_cash_delta(const models::Order& order, Money fill_price, double fillable_quantity, Money commission) {
        auto fill_value = fill_price * fillable_quantity;

        if (order.is_buy()) {
            return (fill_value + commission) * -1;
        }

        if (order.is_sell()) {
            return fill_value - commission;
        }

        return Money(0);
    }

    Money Executor::calculate_fill_price(const models::Order& order, const simulators::State& state) {
        Money current_bar_close = state.get_symbol_close(order.symbol_);
        if (order.is_limit_order() && order.limit_price_.has_value()) {
            if (order.is_buy()) {
                return std::min(order.limit_price_.value(), current_bar_close);
            }
            return std::max(order.limit_price_.value(), current_bar_close);
        }
        return current_bar_close;
    }

    models::Order Executor::signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state) {
        std::optional<Money> stop_loss_price = PositionCalculator::calculate_signal_stop_loss_price(signal, host_params, state);
        std::optional<Money> take_profit_price = PositionCalculator::calculate_signal_take_profit_price(signal, host_params, state);
        double quantity = PositionCalculator::calculate_signal_position_size(signal, host_params, state);

        return {quantity, state.current_timestamp_ns_, signal.symbol_, signal.action_, constants::MARKET, std::nullopt, stop_loss_price, take_profit_price};
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    Money Executor::calculate_margin_required(double position_value_dollars, double leverage, double initial_margin_pct) {
        double margin_from_leverage = position_value_dollars / leverage;
        double margin_from_initial = position_value_dollars * initial_margin_pct;

        return Money::from_dollars(std::max(margin_from_leverage, margin_from_initial));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::optional<std::string> Executor::validate_margin(const models::Order& order, Money fill_price, Money commission,
                                                         const plugins::manifest::HostParams& host_params, const simulators::State& state,
                                                         // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                                                         double position_opening_quantity, double new_position_quantity, Money cash_delta) {
        if (order.is_sell() && !host_params.allow_short_selling_.value_or(true)) {
            if (new_position_quantity < 0) {
                return "Short selling is not allowed";
            }
        }

        double leverage = order.leverage_.value_or(1.0);
        double max_leverage = host_params.max_leverage_.value_or(1.0);

        if (leverage < 1.0) {
            return "Leverage must be >= 1.0";
        }

        if (leverage > max_leverage) {
            return "Order leverage exceeds maximum allowed";
        }

        if (position_opening_quantity <= constants::EPSILON) {
            if (order.is_buy()) {
                if ((state.cash_ + cash_delta).to_dollars() < constants::EPSILON) {
                    return "Insufficient cash to close position. Required: $" + std::to_string(cash_delta.to_dollars()) + ", Available: $" +
                           std::to_string(state.cash_.to_dollars());
                }
            }
            return std::nullopt;
        }

        double position_value_dollars = fill_price.to_dollars() * position_opening_quantity;
        double initial_margin_pct = host_params.initial_margin_pct_.value_or(1.0);

        Money margin_required = calculate_margin_required(position_value_dollars, leverage, initial_margin_pct);

        Money total_required = margin_required + commission;

        Money available_margin = EquityCalculator::calculate_available_margin(state);

        if (total_required > available_margin) {
            return "Insufficient margin. Required: $" + std::to_string(total_required.to_dollars()) + " (margin: $" +
                   std::to_string(margin_required.to_dollars()) + " + commission: $" + std::to_string(commission.to_dollars()) + "), Available: $" +
                   std::to_string(available_margin.to_dollars());
        }

        return std::nullopt;
    }
}  // namespace simulators
