#include <const_string/const_string.h>

#include <chrono>

#include <algorithm>
#include <numeric>

#include <iostream>
#include <random>
#include <string>
#include <vector>


#include "helpers.hpp"

// this will succcessively split the strings with each split_char
template<int Algo>
const_string run( const std::vector<const_string>& strings, const std::vector<char>& split_chars )
{
	auto cstrings = strings;
	for( char split_char : split_chars ) {
		std::vector<std::vector<const_string>> tmp( cstrings.size() );

		size_t i = 0;

		for( auto&& s : cstrings ) {
			static_assert( 0 < Algo && Algo < 3 , "No algorithm with that number available at the moment" );
			if constexpr( Algo == 1 ) {
				tmp[i++] = s.split_full( split_char );
			} else if constexpr( Algo == 2 ) {
				std::vector<const_string> words;
				for( auto&& w : s.split_lazy( split_char ) ) {
					words.push_back( std::move(w) );
				}
				tmp[i++] = std::move( words );
			}
		}
		cstrings = flatten( tmp );
	}

	return concat( cstrings );
}


template<int Algo>
// __declspec(noinline)
void test_algo( const std::vector<const_string>& s, const std::vector<char>& split_chars )
{
	const int runs = 20; // number of runs over which to average
	const int repetitions = 10;
	using namespace std::chrono;
	for( int z = 0; z < repetitions; ++z ) {
		std::chrono::nanoseconds total{};
		for( int i = 0; i < runs; ++i ) {

			auto start = steady_clock::now();

			run<Algo>( s, split_chars );

			auto end = steady_clock::now();
			total += ( end - start );
		}
		std::cout << total / std::chrono::milliseconds{1} / runs << "ms per run" << std::endl;
	}

}



int main()
{
	const std::vector<char>   split_chars{' ', ':', '/', ';', ','};
	const std::vector<const_string> cstrings = to_const_strings( generate_random_strings( 40 ) );



	test_algo<1>( cstrings, split_chars );
	std::cout << "========================================================" << std::endl;
	test_algo<2>( cstrings, split_chars );
	std::cout << "========================================================" << std::endl;
	//test_algo<3>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<3>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<2>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<1>( cstrings, split_chars );
	std::cout << "Total number of c_string allocations:" << detail::stats().get_total_allocs() << std::endl;
	std::cout << "========================================================" << std::endl;
}
