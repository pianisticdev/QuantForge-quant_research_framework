#ifndef QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP
#define QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP

#pragma once

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"

namespace simulators {
    struct BackTestReport {
        std::string some_value_;
        std::string another_value_;
    };

    class BackTestEngine {
       public:
        BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store);
        void run();
        [[nodiscard]] const BackTestReport& get_report();

       private:
        const plugins::loaders::IPluginLoader* plugin_;
        const forge::DataStore* data_store_;
        BackTestReport report_;
    };
}  // namespace simulators

#endif
