#include "back_test_engine.hpp"

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"

namespace simulators {

    BackTestEngine::BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store)
        : plugin_(plugin), data_store_(data_store) {}

    void BackTestEngine::run() {
        report_ = {
            .some_value_ = "some_value",
            .another_value_ = "another_value",
        };
    }

    const BackTestReport& BackTestEngine::get_report() { return report_; }

}  // namespace simulators
