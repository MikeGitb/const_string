cmake_minimum_required(VERSION 3.9)
project(const_string_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(Catch2 CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(const_string_test main.cpp tests.cpp)
target_link_libraries(const_string_test PUBLIC const_string Catch2::Catch2 Threads::Threads)

if(MSVC)
	target_compile_options(const_string_test
		PUBLIC
			/permissive-
			-D_SCL_SECURE_NO_WARNINGS
	)
else()
	target_compile_options(const_string_test
		PUBLIC
			-fprofile-arcs
			-ftest-coverage
			-fno-omit-frame-pointer
			-g
			-fsanitize=undefined
			-Wall -Wextra -Werror -Wno-unused-variable
	)
endif()

include(ParseAndAddCatchTests)
ParseAndAddCatchTests(const_string_test)
