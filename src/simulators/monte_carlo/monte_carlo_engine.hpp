#ifndef QUANT_SIMULATORS_MONTE_CARLO_ENGINE_HPP
#define QUANT_SIMULATORS_MONTE_CARLO_ENGINE_HPP

#pragma once

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"

namespace simulators {
    struct MonteCarloReport {
        std::string some_value_;
        std::string another_value_;
    };

    class MonteCarloEngine {
       public:
        MonteCarloEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store);
        void run();
        [[nodiscard]] const MonteCarloReport& get_report();

       private:
        const plugins::loaders::IPluginLoader* plugin_;
        const forge::DataStore* data_store_;
        MonteCarloReport report_;
    };
}  // namespace simulators

#endif
