#include "data_store.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../http/api/stock_api.hpp"

namespace forge {
    void DataStore::store_bars_by_plugin_name(const std::string& plugin_name, const std::string& symbol, const http::stock_api::AggregateBars& bars) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (bars_.find(plugin_name) == bars_.end()) {
            bars_[plugin_name] = std::unordered_map<std::string, http::stock_api::AggregateBars>();
        }

        bars_[plugin_name][symbol] = bars;
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::optional<http::stock_api::AggregateBars> DataStore::get_bars(const std::string& plugin_name, const std::string& symbol) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto plugin_it = bars_.find(plugin_name);
        if (plugin_it == bars_.end()) {
            return std::nullopt;
        }

        auto symbol_it = plugin_it->second.find(symbol);
        if (symbol_it == plugin_it->second.end()) {
            return std::nullopt;
        }

        return symbol_it->second;
    }

    std::optional<std::unordered_map<std::string, http::stock_api::AggregateBars>> DataStore::get_all_bars_for_plugin(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = bars_.find(plugin_name);
        if (it == bars_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    std::vector<std::string> DataStore::get_symbols_for_plugin(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> symbols;
        auto it = bars_.find(plugin_name);
        if (it != bars_.end()) {
            symbols.reserve(it->second.size());
            for (const auto& [symbol, _] : it->second) {
                symbols.push_back(symbol);
            }
        }

        return symbols;
    }

    bool DataStore::has_plugin_data(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return bars_.find(plugin_name) != bars_.end();
    }

    void DataStore::clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        bars_.clear();
    }
}  // namespace forge
