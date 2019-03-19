#include <const_string/const_string.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <iostream>

#include "helpers.hpp"

TEST_CASE( "Split Position" )
{
	const_string s( "Hello World" );
	{
		auto [h, w] = s.split_at_pos( 5 );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split_at_pos( 5, const_string::Split::Before );
		REQUIRE( h == "Hello" );
		REQUIRE( w == " World" );
	}
	{
		auto [h, w] = s.split_at_pos( 5, const_string::Split::After );
		REQUIRE( h == "Hello " );
		REQUIRE( w == "World" );
	}
	{
		auto [h, w] = s.split_at_pos( 5, const_string::Split::Drop );
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

TEST_CASE( "Split lazy" )
{
	std::vector<const_string> ref{"Hello", "my", "dear!", "How", "are", "you?"};

	std::string  base = "Hello my dear! How are you?";
	const_string s( base );

	auto ref_it = ref.begin();
	for( const auto& word : s.split_lazy( ' ' ) ) {
		CHECK( word == *ref_it++ );
	}
}

TEST_CASE( "Fuzzy Split" )
{
	const std::vector<char> split_chars{' ', ':', '/', ';', ','};
	auto                    cstrings = to_const_strings( generate_random_strings( 2 ) );
	for( char split_char : split_chars ) {
		std::vector<std::vector<const_string>> tmp( cstrings.size() );

		size_t i = 0;

		for( auto s : cstrings ) {
			auto words1 = s.split_full( split_char );

			auto w1it = words1.begin();

			std::vector<const_string> words2;
			for( auto w : s.split_lazy( split_char ) ) {
				if( w1it == words1.end() ) {
					std::cout << words1.size() << std::endl;
				}
				CHECK( w1it != words1.end());
				CHECK( std::string_view(w) == std::string_view(*w1it++) );
				words2.push_back( std::move( w ) );
			}
			CHECK( words1 == words2 );
			if( words1 != words2 ) {
				auto it = std::mismatch( words1.begin(), words1.end(), words2.begin(), words2.end() );
				std::cout << "\"" << *it.first << "\"!=\"" << *it.second << "\"" << std::endl;
			}

			tmp[i++] = std::move( words1 );
		}
		cstrings = flatten( tmp );
	}
}
