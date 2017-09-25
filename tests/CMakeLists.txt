cmake_minimum_required(VERSION 3.2)
project(const_string_tests LANGUAGES CXX)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /EHsc -D_SCL_SECURE_NO_WARNINGS /permissive-")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -fsanitize=undefined  -Wall -Wextra -Werror -Wno-unused-variable -fno-omit-frame-pointer -pthread")
endif()

find_path(CatchPath catch.hpp)

add_executable(const_string_test main.cpp tests.cpp)
target_link_libraries(const_string_test PUBLIC const_string)
target_include_directories(const_string_test PUBLIC ${CatchPath})

include(ParseAndAddCatchTests.cmake)
ParseAndAddCatchTests(const_string_test)
