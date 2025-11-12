#ifndef QUANT_FORGE_FORGE_HPP
#define QUANT_FORGE_FORGE_HPP

#pragma once

#include <memory>

#include "../../http/api/stock_api.hpp"
#include "../../http/client/curl_easy.hpp"
#include "../../plugins/manager/plugin_manager.hpp"
#include "../../renderers/interface.hpp"
#include "../stores/data_store.hpp"
#include "../stores/report_store.hpp"

namespace forge {
    struct InitializationOptions {
        std::string loader_;
        std::string root_path_;
    };

    struct ThreadPoolOptions {
        unsigned int io_threads_ = 2;
        unsigned int compute_threads_ = 2;
    };

    class ForgeEngine {
       public:
        void set_stock_api(std::unique_ptr<http::stock_api::StockAPI> stock_api);
        void set_plugin_manager(std::unique_ptr<plugins::manager::PluginManager> plugin_manager);
        void set_thread_pools(const ThreadPoolOptions& thread_pool_options);
        void set_data_store(std::unique_ptr<DataStore> data_store);
        void set_report_store(std::unique_ptr<ReportStore> report_store);
        void set_renderer(std::unique_ptr<renderers::IRenderer> renderer);

        [[nodiscard]] const ThreadPoolOptions& get_thread_pool_options() const;

        void initialize(const InitializationOptions& initialization_options) const;
        void fetch_data() const;
        void run() const;
        void report() const;

       private:
        ThreadPoolOptions thread_pool_options_;
        std::unique_ptr<plugins::manager::PluginManager> plugin_manager_;
        std::unique_ptr<http::stock_api::StockAPI> stock_api_;
        std::unique_ptr<forge::DataStore> data_store_;
        std::unique_ptr<forge::ReportStore> report_store_;
        std::unique_ptr<renderers::IRenderer> renderer_;
    };

    class ForgeEngineBuilder {
       public:
        ForgeEngineBuilder();

        ForgeEngineBuilder& with_data_provider(std::unique_ptr<http::stock_api::IStockDataProvider> provider);
        ForgeEngineBuilder& with_http_client(std::unique_ptr<http::client::CurlEasy> http_client);
        ForgeEngineBuilder& with_thread_pools(const ThreadPoolOptions& thread_pool_options);
        ForgeEngineBuilder& with_plugin_names(const std::vector<std::string>& plugin_names);
        ForgeEngineBuilder& with_renderer(std::unique_ptr<renderers::IRenderer> renderer);
        ForgeEngineBuilder& validate();
        std::unique_ptr<ForgeEngine> build();

       private:
        std::unique_ptr<ForgeEngine> forge_engine_;
        std::unique_ptr<http::stock_api::IStockDataProvider> data_provider_;
        std::unique_ptr<http::client::CurlEasy> http_client_;
        std::vector<std::string> plugin_names_;
    };

}  // namespace forge

#endif
