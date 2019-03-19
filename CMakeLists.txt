cmake_minimum_required(VERSION 3.9)
project(const_string LANGUAGES CXX)

add_library(const_string INTERFACE)

target_include_directories(const_string INTERFACE include)

target_compile_features(const_string INTERFACE cxx_std_17)

enable_testing()
add_subdirectory(tests)
