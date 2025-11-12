add_library(renderers STATIC console_renderer.cpp)
target_include_directories(renderers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(renderers
        PUBLIC simulators_back_test simulators_monte_carlo
)
