#ifndef QUANT_FORGE_PLUGINS_MANIFEST_MANIFEST_HPP
#define QUANT_FORGE_PLUGINS_MANIFEST_MANIFEST_HPP

#include <simdjson.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "../abi/abi.h"

namespace plugins::manifest {

    template <typename T>
    struct ParserOptions {
        bool is_required_ = true;
        std::vector<T> allowed_values_;
        T fallback_value_;
        std::string error_message_;
    };

    // See: manifest.schema.json for more details.
    struct Symbol {
        bool primary_;
        int timespan_;
        std::string symbol_;
        std::string timespan_unit_;

        Symbol(bool primary, int timespan, std::string symbol, std::string timespan_unit)
            : primary_(primary), timespan_(timespan), symbol_(std::move(symbol)), timespan_unit_(std::move(timespan_unit)) {}
    };

    // See: manifest.schema.json for more details.
    struct HostParams {
        std::optional<bool> market_hours_only_;
        std::optional<bool> allow_fractional_shares_;
        int monte_carlo_runs_;
        int monte_carlo_seed_;
        std::optional<int> leverage_;
        std::optional<double> commission_;
        std::optional<double> slippage_;
        std::optional<double> tax_;
        long long initial_capital_;
        std::optional<std::string> commission_type_;
        std::optional<std::string> slippage_model_;
        std::optional<std::string> currency_;
        std::optional<std::string> timezone_;
        std::optional<std::string> optimization_mode_;
        std::string backtest_start_datetime_;
        std::string backtest_end_datetime_;
        std::vector<Symbol> symbols_;
    };

    inline const ParserOptions<std::string_view> NAME_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid name"};
    inline const ParserOptions<std::string_view> KIND_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {"python", "native"}, .fallback_value_ = "", .error_message_ = "Invalid kind"};
    inline const ParserOptions<std::string_view> ENTRY_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid entry"};
    inline const ParserOptions<std::string_view> DESCRIPTION_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = false, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid description"};
    inline const ParserOptions<std::string_view> AUTHOR_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = false, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid author"};
    inline const ParserOptions<long long> API_VERSION_PARSER_OPTIONS =
        ParserOptions<long long>{.is_required_ = true, .allowed_values_ = {PLUGIN_API_VERSION}, .fallback_value_ = 0, .error_message_ = "Invalid api version"};
    inline const ParserOptions<std::string_view> VERSION_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid version"};
    inline const ParserOptions<std::string_view> STRATEGY_PARAMS_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid strategy params"};
    inline const ParserOptions<bool> PRIMARY_PARSER_OPTIONS =
        ParserOptions<bool>{.is_required_ = true, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid primary"};
    inline const ParserOptions<long long> TIMESPAN_PARSER_OPTIONS =
        ParserOptions<long long>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid timespan"};
    inline const ParserOptions<std::string_view> SYMBOL_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid symbol"};
    inline const ParserOptions<std::string_view> TIMESPAN_UNIT_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true,
                                        .allowed_values_ = {"second", "minute", "hour", "day", "week", "month", "year"},
                                        .fallback_value_ = "",
                                        .error_message_ = "Invalid timespan unit"};
    inline const ParserOptions<bool> MARKET_HOURS_ONLY_PARSER_OPTIONS =
        ParserOptions<bool>{.is_required_ = true, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid market hours only"};
    inline const ParserOptions<bool> ALLOW_FRACTIONAL_SHARES_PARSER_OPTIONS = ParserOptions<bool>{
        .is_required_ = true, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid allow fractional shares"};
    inline const ParserOptions<long long> MONTE_CARLO_RUNS_PARSER_OPTIONS =
        ParserOptions<long long>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid monte carlo runs"};
    inline const ParserOptions<long long> MONTE_CARLO_SEED_PARSER_OPTIONS =
        ParserOptions<long long>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid monte carlo seed"};
    inline const ParserOptions<long long> LEVERAGE_PARSER_OPTIONS =
        ParserOptions<long long>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid leverage"};
    inline const ParserOptions<double> COMMISSION_PARSER_OPTIONS =
        ParserOptions<double>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid commission"};
    inline const ParserOptions<std::string_view> COMMISSION_TYPE_PARSER_OPTIONS = ParserOptions<std::string_view>{
        .is_required_ = true, .allowed_values_ = {"per_share", "percentage", "flat"}, .fallback_value_ = "", .error_message_ = "Invalid commission type"};
    inline const ParserOptions<double> SLIPPAGE_PARSER_OPTIONS =
        ParserOptions<double>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid slippage"};
    inline const ParserOptions<std::string_view> SLIPPAGE_MODEL_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true,
                                        .allowed_values_ = {"none", "fixed", "percentage", "volume_based"},
                                        .fallback_value_ = "",
                                        .error_message_ = "Invalid slippage model"};
    inline const ParserOptions<double> TAX_PARSER_OPTIONS =
        ParserOptions<double>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid tax"};
    inline const ParserOptions<std::string_view> CURRENCY_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {"USD"}, .fallback_value_ = "", .error_message_ = "Invalid currency"};
    inline const ParserOptions<std::string_view> TIMEZONE_PARSER_OPTIONS = ParserOptions<std::string_view>{
        .is_required_ = true, .allowed_values_ = {"America/New_York"}, .fallback_value_ = "", .error_message_ = "Invalid timezone"};
    inline const ParserOptions<std::string_view> OPTIMIZATION_MODE_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true,
                                        .allowed_values_ = {"none", "grid_search", "bayesian", "genetic"},
                                        .fallback_value_ = "",
                                        .error_message_ = "Invalid optimization mode"};
    inline const ParserOptions<std::string_view> BACKTEST_START_DATETIME_PARSER_OPTIONS = ParserOptions<std::string_view>{
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid backtest start datetime"};
    inline const ParserOptions<std::string_view> BACKTEST_END_DATETIME_PARSER_OPTIONS =
        ParserOptions<std::string_view>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid backtest end datetime"};
    inline const ParserOptions<long long> INITIAL_CAPITAL_PARSER_OPTIONS =
        ParserOptions<long long>{.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid initial capital"};

    class PluginManifest {
       public:
        PluginManifest() = default;

        ~PluginManifest() = default;

        PluginManifest(const PluginManifest&) = delete;
        PluginManifest& operator=(const PluginManifest&) = delete;
        PluginManifest(PluginManifest&&) = delete;
        PluginManifest& operator=(PluginManifest&&) = delete;

        [[nodiscard]] bool is_python() const;
        [[nodiscard]] bool is_native() const;
        [[nodiscard]] bool is_one_of(const std::vector<std::string>& plugin_names) const;

        // Load from dir is the initial manifest loading option.
        // In the future there may be other ways to load the manifest.
        [[nodiscard]] static simdjson::ondemand::document load_from_file(const std::filesystem::path& path);
        void parse_json(simdjson::ondemand::document& doc);

        [[nodiscard]] std::string get_name() const;
        [[nodiscard]] int get_api_version() const;
        [[nodiscard]] std::string get_kind() const;
        [[nodiscard]] std::string get_entry() const;
        [[nodiscard]] PluginOptions get_options() const;

       private:
        int api_version_ = 0;
        std::string name_;
        std::string kind_;
        std::string entry_;
        std::optional<std::string> description_;
        std::optional<std::string> author_;
        std::string version_;
        HostParams host_params_;
        std::string strategy_params_;

        mutable std::vector<PluginConfigKV> cached_options_;
        mutable std::vector<std::string> cached_option_strings_;
    };

}  // namespace plugins::manifest

#endif
