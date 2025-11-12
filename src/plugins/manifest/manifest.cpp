
#include "manifest.hpp"

#include <simdjson.h>

#include <filesystem>
#include <string>

namespace plugins::manifest {
    namespace parser {
        template <typename T>
        static T parse_value(simdjson::simdjson_result<T> result, const ParserOptions<T>& options) {
            if (result.error() != simdjson::error_code::SUCCESS) {
                throw std::runtime_error(options.error_message_);
            }

            auto value = result.value();

            if (options.allowed_values_.empty()) {
                return T(value);
            }

            if (!std::ranges::any_of(options.allowed_values_, [value](const T& allowed_value) { return allowed_value == value; })) {
                throw std::runtime_error(options.error_message_);
            }

            return T(value);
        }

    }  // namespace parser

    bool PluginManifest::is_python() const { return kind_ == "python"; };

    bool PluginManifest::is_native() const { return kind_ == "native"; };

    bool PluginManifest::is_one_of(const std::vector<std::string>& plugin_names) const {
        return std::ranges::any_of(plugin_names, [this](const std::string& plugin_name) { return plugin_name == name_; });
    };

    simdjson::ondemand::document PluginManifest::load_from_file(const std::filesystem::path& path) {
        std::filesystem::path manifest_json_file = path / "manifest.json";

        if (!std::filesystem::exists(manifest_json_file) || !std::filesystem::is_regular_file(manifest_json_file)) {
            throw std::runtime_error("Manifest file not found");
        }

        simdjson::ondemand::parser parser;
        simdjson::padded_string manifest_json = simdjson::padded_string::load(manifest_json_file.string());
        simdjson::ondemand::document manifest_document = parser.iterate(manifest_json);

        return manifest_document;
    };

    std::string PluginManifest::get_name() const { return name_; };

    int PluginManifest::get_api_version() const { return api_version_; };

    std::string PluginManifest::get_kind() const { return kind_; };

    std::string PluginManifest::get_entry() const { return entry_; };

    enum class OptimizationMode { NONE, GRID_SEARCH, BAYESIAN, GENETIC };

    void PluginManifest::parse_json(simdjson::ondemand::document& doc) {
        name_ = parser::parse_value(doc["name"].get_string(), NAME_PARSER_OPTIONS);
        kind_ = parser::parse_value(doc["kind"].get_string(), KIND_PARSER_OPTIONS);
        entry_ = parser::parse_value(doc["entry"].get_string(), ENTRY_PARSER_OPTIONS);
        description_ = parser::parse_value(doc["description"].get_string(), DESCRIPTION_PARSER_OPTIONS);
        author_ = parser::parse_value(doc["author"].get_string(), AUTHOR_PARSER_OPTIONS);
        version_ = parser::parse_value(doc["version"].get_string(), VERSION_PARSER_OPTIONS);
        api_version_ = int(parser::parse_value(doc["api_version"].get_int64(), API_VERSION_PARSER_OPTIONS));

        std::vector<Symbol> parsed_symbols;
        auto raw_symbols = doc["host_params"]["symbols"].get_array();
        if (raw_symbols.error() != simdjson::error_code::SUCCESS) {
            throw std::runtime_error("Invalid symbols");
        }

        for (auto raw_symbol : raw_symbols.value()) {
            auto raw_symbol_object = raw_symbol.get_object();
            if (raw_symbol_object.error() != simdjson::error_code::SUCCESS) {
                throw std::runtime_error("Invalid symbol object");
            }

            auto obj = raw_symbol_object.value();
            parsed_symbols.emplace_back(parser::parse_value<bool>(obj["primary"].get_bool(), PRIMARY_PARSER_OPTIONS),
                                        int(parser::parse_value(obj["timespan"].get_int64(), TIMESPAN_PARSER_OPTIONS)),
                                        std::string(parser::parse_value(obj["symbol"].get_string(), SYMBOL_PARSER_OPTIONS)),
                                        std::string(parser::parse_value(obj["timespan_unit"].get_string(), TIMESPAN_UNIT_PARSER_OPTIONS)));
        }

        host_params_ = HostParams{
            .market_hours_only_ = parser::parse_value<bool>(doc["host_params"]["market_hours_only"].get_bool(), MARKET_HOURS_ONLY_PARSER_OPTIONS),
            .allow_fractional_shares_ =
                parser::parse_value<bool>(doc["host_params"]["allow_fractional_shares"].get_bool(), ALLOW_FRACTIONAL_SHARES_PARSER_OPTIONS),
            .monte_carlo_runs_ = int(parser::parse_value(doc["host_params"]["monte_carlo_runs"].get_int64(), MONTE_CARLO_RUNS_PARSER_OPTIONS)),
            .monte_carlo_seed_ = int(parser::parse_value(doc["host_params"]["monte_carlo_seed"].get_int64(), MONTE_CARLO_SEED_PARSER_OPTIONS)),
            .initial_capital_ = (long long)(parser::parse_value(doc["host_params"]["initial_capital"].get_int64(), INITIAL_CAPITAL_PARSER_OPTIONS)),
            .backtest_start_datetime_ =
                std::string(parser::parse_value(doc["host_params"]["backtest_start_datetime"].get_string(), BACKTEST_START_DATETIME_PARSER_OPTIONS)),
            .backtest_end_datetime_ =
                std::string(parser::parse_value(doc["host_params"]["backtest_end_datetime"].get_string(), BACKTEST_END_DATETIME_PARSER_OPTIONS)),
            .leverage_ = std::optional<int>(parser::parse_value(doc["host_params"]["leverage"].get_int64(), LEVERAGE_PARSER_OPTIONS)),
            .commission_ = std::optional<double>(parser::parse_value(doc["host_params"]["commission"].get_double(), COMMISSION_PARSER_OPTIONS)),
            .commission_type_ =
                std::optional<std::string>(parser::parse_value(doc["host_params"]["commission_type"].get_string(), COMMISSION_TYPE_PARSER_OPTIONS)),
            .slippage_ = std::optional<double>(parser::parse_value(doc["host_params"]["slippage"].get_double(), SLIPPAGE_PARSER_OPTIONS)),
            .slippage_model_ =
                std::optional<std::string>(parser::parse_value(doc["host_params"]["slippage_model"].get_string(), SLIPPAGE_MODEL_PARSER_OPTIONS)),
            .tax_ = std::optional<double>(parser::parse_value(doc["host_params"]["tax"].get_double(), TAX_PARSER_OPTIONS)),
            .currency_ = std::optional<std::string>(parser::parse_value(doc["host_params"]["currency"].get_string(), CURRENCY_PARSER_OPTIONS)),
            .timezone_ = std::optional<std::string>(parser::parse_value(doc["host_params"]["timezone"].get_string(), TIMEZONE_PARSER_OPTIONS)),
            .optimization_mode_ =
                std::optional<std::string>(parser::parse_value(doc["host_params"]["optimization_mode"].get_string(), OPTIMIZATION_MODE_PARSER_OPTIONS)),
            .symbols_ = parsed_symbols,
        };

        auto result = simdjson::to_json_string(doc["strategy_params"]);
        if (result.error() != simdjson::error_code::SUCCESS) {
            throw std::runtime_error("Error converting strategy params to JSON string");
        }
        strategy_params_ = std::string(result.value());
    };

    PluginOptions PluginManifest::get_options() const {
        cached_options_.clear();
        cached_option_strings_.clear();

        auto store_string = [this](std::string str) -> const char* {
            cached_option_strings_.emplace_back(std::move(str));
            return cached_option_strings_.back().c_str();
        };

        auto add_kv = [this, &store_string](const std::string& key, const std::string& value) {
            cached_options_.push_back(PluginConfigKV{store_string(key), store_string(value)});
        };

        auto add_optional = [&add_kv](const std::string& key, const auto& opt_value) {
            if (opt_value.has_value()) {
                if constexpr (std::is_same_v<std::decay_t<decltype(*opt_value)>, std::string>) {
                    add_kv(key, *opt_value);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(*opt_value)>, bool>) {
                    add_kv(key, *opt_value ? "true" : "false");
                } else {
                    add_kv(key, std::to_string(*opt_value));
                }
            }
        };

        add_optional("market_hours_only", host_params_.market_hours_only_);
        add_optional("allow_fractional_shares", host_params_.allow_fractional_shares_);
        add_kv("monte_carlo_runs", std::to_string(host_params_.monte_carlo_runs_));
        add_kv("monte_carlo_seed", std::to_string(host_params_.monte_carlo_seed_));
        add_kv("initial_capital", std::to_string(host_params_.initial_capital_));
        add_kv("backtest_start_datetime", host_params_.backtest_start_datetime_);
        add_kv("backtest_end_datetime", host_params_.backtest_end_datetime_);

        add_optional("leverage", host_params_.leverage_);
        add_optional("commission", host_params_.commission_);
        add_optional("commission_type", host_params_.commission_type_);
        add_optional("slippage", host_params_.slippage_);
        add_optional("slippage_model", host_params_.slippage_model_);
        add_optional("tax", host_params_.tax_);
        add_optional("currency", host_params_.currency_);
        add_optional("timezone", host_params_.timezone_);
        add_optional("optimization_mode", host_params_.optimization_mode_);

        for (size_t i = 0; i < host_params_.symbols_.size(); ++i) {
            const auto& sym = host_params_.symbols_[i];
            std::string prefix = "symbol_" + std::to_string(i) + "_";

            add_kv(prefix + "symbol", sym.symbol_);
            add_kv(prefix + "primary", sym.primary_ ? "true" : "false");
            add_kv(prefix + "timespan", std::to_string(sym.timespan_));
            add_kv(prefix + "timespan_unit", sym.timespan_unit_);
        }

        add_kv("symbol_count", std::to_string(host_params_.symbols_.size()));

        add_kv("strategy_params", strategy_params_);

        return PluginOptions{
            .items_ = cached_options_.data(),
            .count_ = cached_options_.size(),
        };
    };

    HostParams PluginManifest::get_host_params() const { return host_params_; }

}  // namespace plugins::manifest
