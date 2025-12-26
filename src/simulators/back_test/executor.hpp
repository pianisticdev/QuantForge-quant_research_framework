#ifndef QUANT_SIMULATORS_BACK_TEST_EXECUTOR_HPP
#define QUANT_SIMULATORS_BACK_TEST_EXECUTOR_HPP

#pragma once

#include "../../plugins/manifest/manifest.hpp"
#include "./models.hpp"
#include "./state.hpp"

namespace simulators {

    class Executor {
       public:
        [[nodiscard]] static models::ExecutionResult execute_order(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                   const simulators::State& state);
        [[nodiscard]] static models::Order signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                           const simulators::State& state);
        [[nodiscard]] static models::ExecutionResult execute_sell(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                  const simulators::State& state);
        [[nodiscard]] static std::pair<double, double> get_fillable_and_remaining_quantities(const models::Order& order,
                                                                                             const plugins::manifest::HostParams& host_params,
                                                                                             const simulators::State& state);

        [[nodiscard]] static std::vector<models::ExitOrder> create_exit_orders(const models::Order& order, const models::Fill& fill,
                                                                               const simulators::State& state, double position_opening_quantity,
                                                                               double new_position);
        [[nodiscard]] static Money calculate_cash_delta(const models::Order& order, Money fill_price, double fillable_quantity, Money commission);
        [[nodiscard]] static Money calculate_fill_price(const models::Order& order, const simulators::State& state);

        [[nodiscard]] static std::optional<std::string> validate_margin(const models::Order& order, Money fill_price, Money commission,
                                                                        const plugins::manifest::HostParams& host_params, const simulators::State& state,
                                                                        double position_opening_quantity, double new_position_quantity, Money cash_delta);

        [[nodiscard]] static double calculate_position_opening_quantity(const models::Order& order, double fillable_quantity, double current_position_quantity,
                                                                        double new_position_quantity);

        [[nodiscard]] static Money calculate_margin_required(double position_value_dollars, double leverage, double initial_margin_pct);
    };

}  // namespace simulators

#endif
