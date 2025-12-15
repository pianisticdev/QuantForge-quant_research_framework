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
    };

}  // namespace simulators

#endif
