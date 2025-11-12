#ifndef QUANT_FORGE_PLUGIN_MANAGER_HPP
#define QUANT_FORGE_PLUGIN_MANAGER_HPP

#pragma once
#include <filesystem>
#include <unordered_map>

#include "../abi/abi.h"
#include "../loaders/interface.hpp"

namespace plugins::manager {

    class PluginManager {
       public:
        PluginManager(const std::vector<std::string>& plugin_names);

        void load_plugins_from_dir(const std::filesystem::path& root);

        ~PluginManager();
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;
        PluginManager(PluginManager&&) = delete;
        PluginManager& operator=(PluginManager&&) = delete;

        template <typename Func>
        void with_plugins(Func&& func) {
            for (auto& [plugin_name, loader] : plugin_map_by_name_) {
                func(loader.get());
            }
        }

       private:
        SimulatorContext ctx_{};
        std::unordered_map<std::string, std::unique_ptr<plugins::loaders::IPluginLoader>> plugin_map_by_name_;
        std::vector<std::string> plugin_names_;
    };

}  // namespace plugins::manager

#endif
