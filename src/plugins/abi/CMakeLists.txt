add_library(plugins_abi INTERFACE)
target_include_directories(plugins_abi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(plugins_abi PUBLIC)