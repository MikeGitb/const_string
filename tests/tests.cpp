#include <const_string/const_string.h>
#include <iostream>
#include <string>
#include <thread>

#include <catch2/catch.hpp>

using namespace std::literals;

void requireZero( std::string_view view )
{
	REQUIRE( view.data()[view.size()] == '\0' );
}

TEST_CASE( "Provides standard container type defs", "[const_string]" )
{
	using T = const_string;
	[[maybe_unused]] T::traits_type            traits{};
	[[maybe_unused]] T::value_type             v{};
	[[maybe_unused]] T::pointer                p{};
	[[maybe_unused]] T::const_pointer          cp{};
	[[maybe_unused]] T::reference              r  = v;
	[[maybe_unused]] T::const_reference        cr = v;
	[[maybe_unused]] T::const_iterator         cit{};
	[[maybe_unused]] T::iterator               it{};
	[[maybe_unused]] T::reverse_iterator       rit{};
	[[maybe_unused]] T::const_reverse_iterator crit{};
	[[maybe_unused]] T::size_type              s{};
	[[maybe_unused]] T::difference_type        d{};
	REQUIRE( true );
}

TEST_CASE( "Provides standard container type defs 2", "[const_zstring]" )
{
	using T = const_zstring;
	[[maybe_unused]] T::traits_type            traits{};
	[[maybe_unused]] T::value_type             v{};
	[[maybe_unused]] T::pointer                p{};
	[[maybe_unused]] T::const_pointer          cp{};
	[[maybe_unused]] T::reference              r  = v;
	[[maybe_unused]] T::const_reference        cr = v;
	[[maybe_unused]] T::const_iterator         cit{};
	[[maybe_unused]] T::iterator               it{};
	[[maybe_unused]] T::reverse_iterator       rit{};
	[[maybe_unused]] T::const_reverse_iterator crit{};
	[[maybe_unused]] T::size_type              s{};
	[[maybe_unused]] T::difference_type        d{};
	REQUIRE( true );
}

TEST_CASE( "Construction from literal", "[const_string]" )
{
	const_string str1 = "Hello World";
	REQUIRE( str1 == "Hello World" );

	const_string str2{"Hello World"};
	REQUIRE( str2 == "Hello World" );

	const_string str3 = {"Hello World"};
	REQUIRE( str3 == "Hello World" );
}

TEST_CASE( "Construction empty", "[const_string]" )
{
	const_string str1 = "";
	const_string str2{};
	REQUIRE( str1 == str2 );
}

TEST_CASE( "Construction from std::string", "[const_string]" )
{
	const_string str1( "Hello World" );
	REQUIRE( str1 == "Hello World" );

	const_string str2{"Hello World"s};
	REQUIRE( str2 == "Hello World" );

	auto         stdstr = "Hello World"s;
	const_string str3{stdstr};
	REQUIRE( str3 == "Hello World" );
}

TEST_CASE( "Construction from temporary std::string", "[const_string]" )
{
	const_string cs = [] {
		auto         stdstr = "Hello World"s;
		const_string cs{stdstr};
		stdstr[0] = 'M'; // modify original string to make sure we really have an independent copy
		REQUIRE( cs != stdstr );
		return cs;
	}();
	REQUIRE( cs == "Hello World" );
}

TEST_CASE( "Copy", "[const_string]" )
{
	const_string str1;
	{
		// no heap allocation
		const_string tcs = "Hello World";
		str1             = tcs;
	}
	REQUIRE( str1 == "Hello World" );

	const_string str2;
	{
		// heap allocated
		const_string tcs{"Hello World"s};
		str2 = tcs;
	}
	REQUIRE( str2 == "Hello World" );
}

TEST_CASE( "concat", "[const_string]" )
{
	const_string cs       = "How are you?";
	auto         combined = concat( "Hello", " World! "s, cs );
	REQUIRE( combined == "Hello World! How are you?" );
	REQUIRE( combined.isZeroTerminated() );
	requireZero( combined );
}

TEST_CASE( "thread" )
{
	constexpr int iterations = 1'000'000;
	using namespace std::literals;
	const std::string    cpps1 = "Good";
	const std::string    cpps2 = "Bad";
	const_string         s1{"Hello"s};
	const_string         s2{"World!"s};
	std::atomic_uint64_t total = 0;

	std::atomic_int total_cpps1_fail_cnt = 0;
	std::atomic_int total_cpps2_fail_cnt = 0;
	std::atomic_int total_s1_fail_cnt    = 0;
	std::atomic_int total_s2_fail_cnt    = 0;

	auto f = [&, s1, s2] {
		std::size_t sum    = 0;
		int cpps1_fail_cnt = 0;
		int cpps2_fail_cnt = 0;
		int s1_fail_cnt    = 0;
		int s2_fail_cnt    = 0;
		for( int i = 0; i < iterations; i++ ) {
			{
				const_string cs{cpps1};
				sum += cs.size();
				cpps1_fail_cnt += cs[0] != 'G';
			}
			{
				const_string cs{cpps2};
				sum += cs.size();
				cpps2_fail_cnt += cs[0] != 'B';
			}
			{
				auto s = s1;
				sum += s.size();
				s1_fail_cnt += s[0] != 'H';
				s = s2;
				s2_fail_cnt += s[0] != 'W';
				sum += s.size();
			}
		}
		total += sum;
		total_cpps1_fail_cnt += cpps1_fail_cnt;
		total_cpps2_fail_cnt += cpps2_fail_cnt;
		total_s1_fail_cnt += s1_fail_cnt;
		total_s2_fail_cnt += s2_fail_cnt;
	};
	std::thread th1( f );
	std::thread th2( f );
	th1.join();
	th2.join();
	REQUIRE( total == ( s1.size() + s2.size() + cpps1.size() + cpps2.size() ) * 2 * iterations );
	REQUIRE( total_cpps1_fail_cnt == 0 );
	REQUIRE( total_cpps2_fail_cnt == 0 );
	REQUIRE( total_s1_fail_cnt == 0 );
	REQUIRE( total_s2_fail_cnt == 0 );
}
