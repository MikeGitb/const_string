cmake_minimum_required(VERSION 3.1)

add_library(const_string INTERFACE)
target_include_directories(const_string INTERFACE include)
target_sources(const_string INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/const_string/const_string.h)

add_subdirectory(tests)
