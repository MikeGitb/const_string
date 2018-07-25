#include <const_string/const_string.h>

#include <catch2/catch.hpp>



TEST_CASE("Substring", "[const_string]")
{
	const_string cs = "HelloWorld";
	{
		auto s = cs.substr(5);
		REQUIRE(s == "World");
	}
	{
		auto s = cs.substr(5, 2);
		REQUIRE(s == "Wo");
	}
	{
		std::string_view sv(cs);
		auto ssv = cs.substr(5);
		auto s = cs.substr(ssv);

		REQUIRE(s == ssv);
	}
	{
		auto s = cs.substr(cs.begin() + 2, cs.end());
		REQUIRE(s == "lloWorld");
	}
}