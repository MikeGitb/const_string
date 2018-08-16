cmake_minimum_required(VERSION 3.9)
project(const_string_tests LANGUAGES CXX)

option(CONST_STRING_RUN_SANITIZERS "Run tests with ubsan and address sanitizer" OFF)
option(CONST_STRING_COVERAGE "Generate coverage data" OFF)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
	add_compile_options(/permissive- )
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

find_package(Catch2 CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(const_string_test main.cpp tests.cpp test_split.cpp test_substr.cpp test_allocation.cpp test_ref_cnt_buf.cpp test_ref_cnt_buf2.cpp test_tree.cpp)
target_link_libraries(const_string_test PUBLIC const_string Catch2::Catch2 Threads::Threads)
target_compile_definitions(const_string_test PUBLIC -DCONST_STRING_DEBUG_HOOKS)

if(MSVC)
	add_compile_options(/permissive- )
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
#	target_compile_options(const_string_test
#		PUBLIC
#			/permissive-
#			-D_SCL_SECURE_NO_WARNINGS
#	)
else()
	target_compile_options(const_string_test
		PUBLIC
			-O1
			-Wall -Wextra
	)

	if(${CONST_STRING_RUN_SANITIZERS})
		target_compile_options(const_string_test
			PUBLIC
				-g
				-O1
				-fno-omit-frame-pointer
				-fsanitize=undefined
				-fsanitize=address
		)
		target_link_libraries(const_string_test
			PUBLIC
				-fsanitize=undefined
				-fsanitize=address
		)
	endif()
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
endif()

include(ParseAndAddCatchTests)
ParseAndAddCatchTests(const_string_test)

add_executable(const_string_benchmark benchmark_split.cpp)
target_compile_definitions(const_string_benchmark PUBLIC -DCONST_STRING_DEBUG_HOOKS)
target_link_libraries(const_string_benchmark PUBLIC const_string Threads::Threads)
