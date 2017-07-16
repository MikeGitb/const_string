#include <iostream>
#include <const_string/const_string.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace std::string_literals;

TEST_CASE("Construction from literal", "[const_string]")
{
	const_string str1 = "Hello World";
	REQUIRE(str1 == "Hello World");

	const_string str2{ "Hello World" };
	REQUIRE(str2 == "Hello World");

	const_string str3 = { "Hello World" };
	REQUIRE(str3 == "Hello World");
}

TEST_CASE("Construction from std::string", "[const_string]")
{
	const_string str1 = "Hello World"s;
	REQUIRE(str1 == "Hello World");

	const_string str2{ "Hello World"s };
	REQUIRE(str2 == "Hello World");

	auto stdstr = "Hello World"s;
	const_string str3{ stdstr };
	REQUIRE(str3 == "Hello World");
}

TEST_CASE("Copy", "[const_string]")
{
	const_string str1;
	{
		//no heap allocation
		const_string tcs = "Hello World";
		str1 = tcs;
	}
	REQUIRE(str1 == "Hello World");

	const_string str2;
	{
		//heap allocated
		const_string tcs = std::string("Hello World");
		str2 = tcs;
	}
	REQUIRE(str2 == "Hello World");

}

TEST_CASE("concat", "[const_string]")
{
	const_string cs = " How are you?";
	auto combined = concat("Hello", " World "s, '!', cs);
	REQUIRE(combined == "Hello World ! How are you?");
	REQUIRE(combined.isZeroTerminated());
}

//int main() {
//	std::cout << "Hello World\n";
//}