add_library(simulators_monte_carlo STATIC monte_carlo_engine.cpp)
target_include_directories(simulators_monte_carlo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(simulators_monte_carlo
        PUBLIC
            simulators_back_test
)
