#include <const_string/const_string.h>
#include <iostream>
#include <string>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace std::literals;

void requireZero(std::string_view view) {
	REQUIRE(view.data()[view.size()] == '\0');
}

TEST_CASE("Provides standard container type defs", "[const_string]")
{
	using T = const_string;
	T::traits_type 	traits{};
	T::value_type 	v{};
	T::pointer		p{};
	T::const_pointer cp{};
	T::reference 	r = v;
	T::const_reference 	cr = v;
	T::const_iterator  cit{};
	T::iterator 	it{};
	T::reverse_iterator 	rit{};
	T::const_reverse_iterator 	crit{};
	T::size_type 	s{};
	T::difference_type 	d{};
}

TEST_CASE("Provides standard container type defs 2", "[const_zstring]")
{
	using T = const_zstring;
	T::traits_type 	traits{};
	T::value_type 	v{};
	T::pointer		p{};
	T::const_pointer cp{};
	T::reference 	r = v;
	T::const_reference 	cr = v;
	T::const_iterator  cit{};
	T::iterator 	it{};
	T::reverse_iterator 	rit{};
	T::const_reverse_iterator 	crit{};
	T::size_type 	s{};
	T::difference_type 	d{};
}

TEST_CASE("Construction from literal", "[const_string]")
{
	const_string str1 = "Hello World";
	REQUIRE(str1 == "Hello World");

	const_string str2{ "Hello World" };
	REQUIRE(str2 == "Hello World");

	const_string str3 = { "Hello World" };
	REQUIRE(str3 == "Hello World");
}

TEST_CASE("Construction empty", "[const_string]")
{
	const_string str1 = "";
	const_string str2{};
	REQUIRE(str1 == str2);
}

TEST_CASE("Construction from std::string", "[const_string]")
{
	const_string str1("Hello World");
	REQUIRE(str1 == "Hello World");

	const_string str2{ "Hello World"s };
	REQUIRE(str2 == "Hello World");

	auto stdstr = "Hello World"s ;
	const_string str3{ stdstr };
	REQUIRE(str3 == "Hello World");
}

TEST_CASE("Construction from temporary std::string", "[const_string]")
{
	const_string cs = []
	{
		auto stdstr = "Hello World"s;
		const_string cs{ stdstr };
		stdstr[0] = 'M'; // modify original string to make sure we really have an independent copy
		REQUIRE(cs != stdstr);
		return cs;
	}();
	REQUIRE(cs == "Hello World");
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
		const_string tcs{"Hello World"s };
		str2 = tcs;
	}
	REQUIRE(str2 == "Hello World");

}

TEST_CASE("concat", "[const_string]")
{
	const_string cs = "How are you?";
	auto combined = concat("Hello", " World! "s,  cs);
	REQUIRE(combined == "Hello World! How are you?");
	REQUIRE(combined.isZeroTerminated());
	requireZero(combined);
}

//int main() {
//	std::cout << "Hello World\n";
//}