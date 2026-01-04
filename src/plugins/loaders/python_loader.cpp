#include "python_loader.hpp"

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "../../http/api/stock_api.hpp"
#include "../../simulators/back_test/abi_converter.hpp"
#include "../../simulators/back_test/models.hpp"
#include "../../simulators/back_test/state.hpp"
#include "../abi/abi.h"
#include "../manifest/manifest.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access, cppcoreguidelines-owning-memory)

namespace py = pybind11;

namespace plugins::loaders {

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

            auto* pp = new PyPlugin{plugin_instance, {}, {}, {}};

            pp->vtable_.destroy = [](void* self) { delete static_cast<PyPlugin*>(self); };

            pp->vtable_.on_init = [](void* self, const PluginOptions* opts) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                py::dict plugin_options_dict = py::dict{};
                if (opts != nullptr) {
                    for (size_t i = 0; i < opts->count_; ++i) {
                        plugin_options_dict[opts->items_[i].key_] = opts->items_[i].value_;
                    }
                }
                auto py_result = python_plugin.obj_.attr("on_init")(plugin_options_dict);

                return PythonLoader::to_plugin_result(python_plugin, py_result);
            };

            pp->vtable_.on_start = [](void* self) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto py_result = python_plugin.obj_.attr("on_start")();
                return PythonLoader::to_plugin_result(python_plugin, py_result);
            };

            pp->vtable_.on_bar = [](void* self, const CBar* bar, const CState* state) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);

                py::dict state_dict;
                state_dict["cash"] = state->cash_;

                py::list positions_list;
                for (size_t i = 0; i < state->positions_count_; ++i) {
                    py::dict pos;
                    pos["symbol"] = state->positions_[i].symbol_;
                    pos["quantity"] = state->positions_[i].quantity_;
                    pos["average_price"] = state->positions_[i].average_price_;
                    positions_list.append(pos);
                }
                state_dict["positions"] = positions_list;

                py::list fills_list;
                for (size_t i = 0; i < state->new_fills_count_; ++i) {
                    py::dict fill;
                    fill["symbol"] = state->new_fills_[i].symbol_;
                    fill["quantity"] = state->new_fills_[i].quantity_;
                    fill["price"] = state->new_fills_[i].price_;
                    fill["created_at_ns"] = state->new_fills_[i].created_at_ns_;
                    fill["uuid"] = state->new_fills_[i].uuid_;
                    fill["action"] = state->new_fills_[i].action_;
                    fills_list.append(fill);
                }
                state_dict["new_fills"] = fills_list;

                py::list exit_orders_list;
                for (size_t i = 0; i < state->new_exit_orders_count_; ++i) {
                    py::dict exit_order;
                    const auto& order = state->new_exit_orders_[i];
                    exit_order["type"] = order.type_;
                    if (order.type_ == EXIT_ORDER_STOP_LOSS) {
                        exit_order["symbol"] = order.data_.stop_loss_.symbol_;
                        exit_order["trigger_quantity"] = order.data_.stop_loss_.trigger_quantity_;
                        exit_order["stop_loss_price"] = order.data_.stop_loss_.stop_loss_price_;
                        exit_order["fill_uuid"] = order.data_.stop_loss_.fill_uuid_;
                    } else if (order.type_ == EXIT_ORDER_TAKE_PROFIT) {
                        exit_order["symbol"] = order.data_.take_profit_.symbol_;
                        exit_order["trigger_quantity"] = order.data_.take_profit_.trigger_quantity_;
                        exit_order["take_profit_price"] = order.data_.take_profit_.take_profit_price_;
                        exit_order["fill_uuid"] = order.data_.take_profit_.fill_uuid_;
                    }
                    exit_orders_list.append(exit_order);
                }
                state_dict["new_exit_orders"] = exit_orders_list;

                py::list equity_curve_list;
                for (size_t i = 0; i < state->equity_curve_count_; ++i) {
                    py::dict equity;
                    equity["timestamp_ns"] = state->equity_curve_[i].timestamp_ns_;
                    equity["equity"] = state->equity_curve_[i].equity_;
                    equity["return"] = state->equity_curve_[i].return_;
                    equity["max_drawdown"] = state->equity_curve_[i].max_drawdown_;
                    equity["sharpe_ratio"] = state->equity_curve_[i].sharpe_ratio_;
                    equity["sortino_ratio"] = state->equity_curve_[i].sortino_ratio_;
                    equity["calmar_ratio"] = state->equity_curve_[i].calmar_ratio_;
                    equity["tail_ratio"] = state->equity_curve_[i].tail_ratio_;
                    equity["value_at_risk"] = state->equity_curve_[i].value_at_risk_;
                    equity["conditional_value_at_risk"] = state->equity_curve_[i].conditional_value_at_risk_;
                    equity_curve_list.append(equity);
                }
                state_dict["equity_curve"] = equity_curve_list;

                py::dict bar_dict;
                bar_dict["symbol"] = bar->symbol_;
                bar_dict["unix_ts_ns"] = bar->unix_ts_ns_;
                bar_dict["open"] = bar->open_;
                bar_dict["high"] = bar->high_;
                bar_dict["low"] = bar->low_;
                bar_dict["close"] = bar->close_;
                bar_dict["volume"] = bar->volume_;

                auto py_result = python_plugin.obj_.attr("on_bar")(bar_dict, state_dict);

                return PythonLoader::to_plugin_result(python_plugin, py_result);
            };

            pp->vtable_.on_end = [](void* self, const char** json_out) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto out = python_plugin.obj_.attr("on_end")().cast<std::string>();
                char* heap = new char[out.size() + 1];
                memcpy(heap, out.c_str(), out.size() + 1);
                *json_out = heap;
                return PluginResult{0, nullptr, nullptr, 0};
            };

            pp->vtable_.free_string = [](void*, const char* p) { delete[] p; };

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
            return PluginResult{1, "Invalid API Version", nullptr, 0};
        }

        if (exp_.vtable_.on_start == nullptr) {
            return PluginResult{1, "Undefined Method on_start", nullptr, 0};
        }

        return exp_.vtable_.on_start(exp_.instance_);
    }

    PluginResult PythonLoader::on_bar(const http::stock_api::AggregateBarResult& bar, simulators::State& state) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", nullptr, 0};
        }

        if (exp_.vtable_.on_bar == nullptr) {
            return PluginResult{1, "Undefined Method on_bar", nullptr, 0};
        }

        CBar c_bar{.symbol_ = bar.symbol_.c_str(),
                   .unix_ts_ns_ = bar.unix_ts_ns_,
                   .open_ = bar.open_,
                   .high_ = bar.high_,
                   .low_ = bar.low_,
                   .close_ = bar.close_,
                   .volume_ = bar.volume_};

        CState c_state = abi_converter_.to_c_state(state);

        return exp_.vtable_.on_bar(exp_.instance_, &c_bar, &c_state);
    }

    PluginResult PythonLoader::on_end(const char** json_out) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", nullptr, 0};
        }

        if (exp_.vtable_.on_end == nullptr) {
            return PluginResult{1, "Undefined Method on_end", nullptr, 0};
        }

        return exp_.vtable_.on_end(exp_.instance_, json_out);
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

    PluginResult PythonLoader::to_plugin_result(PyPlugin& python_plugin, py::object& py_result) {
        python_plugin.current_instructions_.clear();
        python_plugin.instruction_strings_.clear();

        int status_code = 0;
        py::list instructions_list;

        try {
            auto result_tuple = py_result.cast<py::tuple>();
            status_code = result_tuple[0].cast<int>();
            instructions_list = result_tuple[1].cast<py::list>();
        } catch (...) {
            status_code = py_result.cast<int>();
        }

        for (auto item : instructions_list) {
            auto inst_dict = item.cast<py::dict>();

            python_plugin.instruction_strings_.push_back(inst_dict["symbol"].cast<std::string>());
            const char* symbol = python_plugin.instruction_strings_.back().c_str();

            python_plugin.instruction_strings_.push_back(inst_dict["action"].cast<std::string>());
            const char* action = python_plugin.instruction_strings_.back().c_str();

            CInstruction c_inst;
            if (inst_dict.contains("quantity")) {
                c_inst.type_ = INSTRUCTION_TYPE_ORDER;

                python_plugin.instruction_strings_.push_back(inst_dict.contains("order_type") ? inst_dict["order_type"].cast<std::string>() : "market");
                const char* order_type = python_plugin.instruction_strings_.back().c_str();

                c_inst.data_.order_.symbol_ = symbol;
                c_inst.data_.order_.action_ = action;
                c_inst.data_.order_.quantity_ = inst_dict["quantity"].cast<double>();
                c_inst.data_.order_.order_type_ = order_type;
                c_inst.data_.order_.limit_price_ =
                    inst_dict.contains("limit_price") ? inst_dict["limit_price"].cast<int64_t>() : models::NULL_MARKET_TRIGGER_PRICE;
                c_inst.data_.order_.stop_loss_price_ =
                    inst_dict.contains("stop_loss_price") ? inst_dict["stop_loss_price"].cast<int64_t>() : models::NULL_MARKET_TRIGGER_PRICE;
                c_inst.data_.order_.take_profit_price_ =
                    inst_dict.contains("take_profit_price") ? inst_dict["take_profit_price"].cast<int64_t>() : models::NULL_MARKET_TRIGGER_PRICE;
                c_inst.data_.order_.leverage_ = inst_dict.contains("leverage") ? inst_dict["leverage"].cast<double>() : 1.0;
            } else {
                c_inst.type_ = INSTRUCTION_TYPE_SIGNAL;
                c_inst.data_.signal_.symbol_ = symbol;
                c_inst.data_.signal_.action_ = action;
            }

            python_plugin.current_instructions_.push_back(c_inst);
        }

        return PluginResult{status_code, status_code == 0 ? nullptr : "on_bar failed",
                            python_plugin.current_instructions_.empty() ? nullptr : python_plugin.current_instructions_.data(),
                            python_plugin.current_instructions_.size()};
    }

}  // namespace plugins::loaders

// NOLINTEND(cppcoreguidelines-pro-type-union-access, cppcoreguidelines-owning-memory)
