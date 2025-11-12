add_library(utils STATIC string_utils.cpp thread_pool.cpp)
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
