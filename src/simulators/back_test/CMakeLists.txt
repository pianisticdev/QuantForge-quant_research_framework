add_library(simulators_back_test STATIC back_test_engine.cpp)
target_include_directories(simulators_back_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
