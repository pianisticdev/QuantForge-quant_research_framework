add_library(forge_stores STATIC data_store.cpp report_store.cpp)
target_include_directories(forge_stores PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(forge_stores
        PUBLIC http_api simulators_back_test simulators_monte_carlo
)
