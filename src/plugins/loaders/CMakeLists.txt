add_library(plugins_loaders STATIC native_loader.cpp python_loader.cpp)
target_include_directories(plugins_loaders PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(plugins_loaders
        PUBLIC
            plugins_abi
            plugins_manifest
            http_api
            pybind11::embed
            pybind11::pybind11
)
