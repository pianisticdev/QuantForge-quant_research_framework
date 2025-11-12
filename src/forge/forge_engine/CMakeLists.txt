add_library(forge_engine STATIC forge.cpp)
target_include_directories(forge_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(forge_engine
        PUBLIC forge_stores http_api http_client plugins_manager simulators_back_test simulators_monte_carlo utils
)
