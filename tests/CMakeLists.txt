cmake_minimum_required(VERSION 3.1)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /EHsc -D_SCL_SECURE_NO_WARNINGS /permissive-")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -Wextra -Werror -Wno-unused-variable")
endif()

find_path(CATCH_INCLUDE_DIR catch.hpp)
include_directories(${CATCH_INCLUDE_DIR})

add_executable(const_string_test main.cpp)
target_link_libraries(const_string_test PUBLIC const_string)
target_sources(const_string_test  PUBLIC main.cpp)
