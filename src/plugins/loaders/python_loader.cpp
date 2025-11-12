#include "python_loader.hpp"

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "../../http/api/stock_api.hpp"
#include "../abi/abi.h"
#include "../manifest/manifest.hpp"

namespace py = pybind11;

namespace plugins::loaders {
    struct PyPlugin {
        py::object obj_;
        PluginVTable vtable_;
    };

    PythonLoader::PythonLoader(std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest) : plugin_manifest_(std::move(plugin_manifest)) {}

    void PythonLoader::load_plugin(const SimulatorContext& ctx) {
        static bool py_up = false;

        if (!py_up) {
            py::initialize_interpreter();
            py_up = true;
        }

        try {
            py::gil_scoped_acquire gil;
            py::module python_module = py::module::import(plugin_manifest_->get_entry().c_str());
            py::object create_plugin_fn = python_module.attr("create_plugin");
            py::capsule ctx_capsule((void*)&ctx, "SimulatorContext", [](void*) {});
            py::object plugin_instance = create_plugin_fn(ctx_capsule);

            auto* pp = new PyPlugin{plugin_instance, {}};  // NOLINT(cppcoreguidelines-owning-memory)

            pp->vtable_.destroy = [](void* self) {
                delete static_cast<PyPlugin*>(self);  // NOLINT(cppcoreguidelines-owning-memory)
            };

            pp->vtable_.on_init = [](void* self, const PluginOptions* opts) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                py::dict plugin_options_dict = py::dict{};
                if (opts != nullptr) {
                    for (size_t i = 0; i < opts->count_; ++i) {
                        plugin_options_dict[opts->items_[i].key_] = opts->items_[i].value_;
                    }
                }
                auto fn_result = python_plugin.obj_.attr("on_init")(plugin_options_dict).cast<int>();
                return PluginResult{fn_result, fn_result == 0 ? nullptr : "on_init failed"};
            };

            pp->vtable_.on_start = [](void* self) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto fn_result = python_plugin.obj_.attr("on_start")().cast<int>();
                return PluginResult{fn_result, fn_result == 0 ? nullptr : "on_start failed"};
            };

            pp->vtable_.on_bar = [](void* self, const Bar* bar) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto fn_result = python_plugin.obj_.attr("on_bar")(bar->unix_ts_ns_, bar->open_, bar->high_, bar->low_, bar->close_, bar->volume_).cast<int>();
                return PluginResult{fn_result, fn_result == 0 ? nullptr : "on_bar failed"};
            };

            pp->vtable_.on_end = [](void* self, const char** json_out) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto out = python_plugin.obj_.attr("on_end")().cast<std::string>();
                char* heap = new char[out.size() + 1];  // NOLINT(cppcoreguidelines-owning-memory)
                memcpy(heap, out.c_str(), out.size() + 1);
                *json_out = heap;
                return PluginResult{0, nullptr};
            };

            pp->vtable_.free_string = [](void*, const char* p) {
                delete[] p;  // NOLINT(cppcoreguidelines-owning-memory)
            };

            exp_ = PluginExport{PLUGIN_API_VERSION, pp, pp->vtable_};
        } catch (const py::error_already_set& e) {
            exp_ = PluginExport{0, nullptr, {}};
        } catch (...) {
            exp_ = PluginExport{0, nullptr, {}};
        }
    }

    void PythonLoader::on_init() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.on_init == nullptr) {
            return;
        }

        auto options = plugin_manifest_->get_options();
        exp_.vtable_.on_init(exp_.instance_, &options);
    }

    PluginResult PythonLoader::on_start() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version"};
        }

        if (exp_.vtable_.on_start == nullptr) {
            return PluginResult{1, "Undefined Method on_start"};
        }

        return exp_.vtable_.on_start(exp_.instance_);
    }

    PluginResult PythonLoader::on_bar(const http::stock_api::AggregateBarResult& bar) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version"};
        }

        if (exp_.vtable_.on_bar == nullptr) {
            return PluginResult{1, "Undefined Method on_bar"};
        }

        Bar plugin_bar = plugins::loaders::to_plugin_bar(bar);
        return exp_.vtable_.on_bar(exp_.instance_, &plugin_bar);
    }

    PluginResult PythonLoader::on_end() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version"};
        }

        if (exp_.vtable_.on_end == nullptr) {
            return PluginResult{1, "Undefined Method on_end"};
        }

        const char* json_out = nullptr;

        return exp_.vtable_.on_end(exp_.instance_, &json_out);
    }

    void PythonLoader::free_string(const char* str) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.free_string == nullptr) {
            return;
        }

        return exp_.vtable_.free_string(exp_.instance_, str);
    }

    void PythonLoader::unload_plugin() {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.destroy != nullptr && exp_.instance_ != nullptr) {
            exp_.vtable_.destroy(exp_.instance_);
            exp_.instance_ = nullptr;
        }
    }

    PluginExport* PythonLoader::get_plugin_export() const { return &exp_; }

    std::string PythonLoader::get_plugin_name() const { return plugin_manifest_->get_name(); }

    plugins::manifest::HostParams PythonLoader::get_host_params() const { return plugin_manifest_->get_host_params(); }

}  // namespace plugins::loaders
