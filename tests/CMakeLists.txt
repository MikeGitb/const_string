cmake_minimum_required(VERSION 3.9)
project(const_string_tests LANGUAGES CXX)

option(CONST_STRING_RUN_SANITIZERS "Run tests with ubsan and address sanitizer" OFF)
option(CONST_STRING_COVERAGE "Generate coverage data" OFF)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
	STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_compile_options(/permissive- )
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
else()
	add_compile_options( -Wall -Wextra )

	if(${CONST_STRING_RUN_SANITIZERS})
		add_compile_options(
				-g
				-O1
				-fno-omit-frame-pointer
				-fsanitize=undefined
				-fsanitize=address
		)
		link_libraries(
				-fsanitize=undefined
				-fsanitize=address
		)
	endif()
endif()

find_package(Catch2 CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(const_string_test main.cpp tests.cpp test_split.cpp test_substr.cpp)
target_link_libraries(const_string_test PUBLIC const_string Catch2::Catch2 Threads::Threads)
target_compile_definitions(const_string_test PUBLIC -DCONST_STRING_DEBUG_HOOKS)


include(ParseAndAddCatchTests)
ParseAndAddCatchTests(const_string_test)

if(${CONST_STRING_COVERAGE})
	target_compile_options(const_string_test
		PUBLIC
			-fprofile-arcs
			-ftest-coverage
			-fno-omit-frame-pointer
	)
	target_link_libraries(const_string_test
		PUBLIC
			-lgcov --coverage
	)
endif()



add_executable(const_string_benchmark benchmark_split.cpp)
target_compile_definitions(const_string_benchmark PUBLIC -DCONST_STRING_DEBUG_HOOKS)
target_link_libraries(const_string_benchmark PUBLIC const_string Threads::Threads)
