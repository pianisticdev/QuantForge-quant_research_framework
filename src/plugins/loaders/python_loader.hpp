#ifndef QUANT_FORGE_PLUGINS_LOADERS_PYTHON_LOADER_HPP
#define QUANT_FORGE_PLUGINS_LOADERS_PYTHON_LOADER_HPP

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <string>
#include <vector>

#include "../../http/api/stock_api.hpp"
#include "../../simulators/back_test/abi_converter.hpp"
#include "../../simulators/back_test/state.hpp"
#include "../abi/abi.h"
#include "../manifest/manifest.hpp"
#include "interface.hpp"

namespace py = pybind11;

namespace plugins::loaders {
    struct PyPlugin {
        py::object obj_;
        PluginVTable vtable_;
        std::vector<CInstruction> current_instructions_;
        std::vector<std::string> instruction_strings_;
    };

    class PythonLoader : public IPluginLoader {
       public:
        explicit PythonLoader(std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest);
        ~PythonLoader() override = default;

        PythonLoader(const PythonLoader&) = delete;
        PythonLoader& operator=(const PythonLoader&) = delete;
        PythonLoader(PythonLoader&&) = delete;
        PythonLoader& operator=(PythonLoader&&) = delete;

        void load_plugin(const SimulatorContext& ctx) override;
        void on_init() const override;
        [[nodiscard]] PluginResult on_start() const override;
        [[nodiscard]] PluginResult on_bar(const http::stock_api::AggregateBarResult& bar, simulators::State& state) const override;  // Changed
        [[nodiscard]] PluginResult on_end(const char** json_out) const override;
        void free_string(const char* str) const override;
        [[nodiscard]] std::string get_plugin_name() const override;
        void unload_plugin() override;
        [[nodiscard]] PluginExport* get_plugin_export() const override;
        [[nodiscard]] plugins::manifest::HostParams get_host_params() const override;

       private:
        [[nodiscard]] static PluginResult to_plugin_result(PyPlugin& python_plugin, py::object& py_result);

        std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest_;
        mutable PluginExport exp_{};
        mutable simulators::ABIConverter abi_converter_;  // Add this for state conversion
    };
}  // namespace plugins::loaders

#endif
