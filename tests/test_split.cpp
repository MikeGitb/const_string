#include <const_string/const_string.h>

#include <catch2/catch.hpp>

#include <iostream>

TEST_CASE( "Split Position" )
{
	const_string s( "Hello World" );
	{
		auto [h, w] = s.split( 5 );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split( 5, const_string::Split::Before );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split( 5, const_string::Split::After );
		REQUIRE( h == "Hello " );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split( 5, const_string::Split::Drop );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "World" );
	}
}

TEST_CASE( "Split Separator Single" )
{
	const_string s( "Hello World" );
	{
		auto [h, w] = s.split_first( ' ' );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_first( ' ', const_string::Split::Before );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split_first( ' ', const_string::Split::After );
		REQUIRE( h == "Hello " );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_first( ' ', const_string::Split::Drop );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "World" );
	}

	{
		auto [h, w] = s.split_last( ' ' );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_last( ' ', const_string::Split::Before );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split_last( ' ', const_string::Split::After );
		REQUIRE( h == "Hello " );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_last( ' ', const_string::Split::Drop );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "World" );
	}
}

TEST_CASE( "Split Separator multi" )
{
	const_string s( "Hello My World" );
	{
		auto [h, w] = s.split_first();
		REQUIRE( h == "Hello" );
		REQUIRE( w == "My World" );
	}
	{
		auto [h, w] = s.split_last();
		REQUIRE( h == "Hello My" );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_first( ' ' );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "My World" );
	}
	{
		auto [h, w] = s.split_first( ' ', const_string::Split::Before );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " My World" );
	}
	{
		auto [h, w] = s.split_first( ' ', const_string::Split::After );
		REQUIRE( h == "Hello " );
		REQUIRE( w == "My World" );
	}
	{
		auto [h, w] = s.split_first( ' ', const_string::Split::Drop );
		REQUIRE( h == "Hello" );
		REQUIRE( w == "My World" );
	}

	{
		auto [h, w] = s.split_last( ' ' );
		REQUIRE( h == "Hello My" );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_last( ' ', const_string::Split::Before );
		REQUIRE( h == "Hello My" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split_last( ' ', const_string::Split::After );
		REQUIRE( h == "Hello My " );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_last( ' ', const_string::Split::Drop );
		REQUIRE( h == "Hello My" );
		REQUIRE( w == "World" );
	}
}

TEST_CASE( "Split full" )
{
	std::vector<const_string> ref{"Hello", "my", "dear!", "How", "are", "you?"};

	std::string  base = "Hello my dear! How are you?";
	const_string s( base );

	const auto words = s.split_full( ' ' );

	CHECK( words.size() == 6 );
	CHECK( words == ref );
}
