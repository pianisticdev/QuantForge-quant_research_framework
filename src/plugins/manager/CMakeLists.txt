add_library(plugins_manager STATIC plugin_manager.cpp)
target_include_directories(plugins_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(plugins_manager
        PUBLIC
            plugins_loaders
            plugins_manifest
)
