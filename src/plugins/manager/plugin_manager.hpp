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
        PluginManager();

        void load_plugins_from_dir(const std::filesystem::path& root, const std::vector<std::string>& plugin_names);

        ~PluginManager();
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;
        PluginManager(PluginManager&&) = delete;
        PluginManager& operator=(PluginManager&&) = delete;

       private:
        PluginExport* lookup(const std::string& name);
        SimulatorContext ctx_{};
        std::unordered_map<std::string, std::unique_ptr<plugins::loaders::IPluginLoader>> plugin_map_by_name_;
    };

}  // namespace plugins::manager

#endif
