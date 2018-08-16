#include <const_string/const_string.h>

#include <chrono>

#include <algorithm>
#include <numeric>

#include <iostream>
#include <random>
#include <string>
#include <vector>

std::vector<std::string> generate_random_strings(int cnt)
{
	std::vector<std::string> ret;

	std::string base(
		":::::::::::::::::::::::: "
		"                                                                ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,     "
		"sfladskjflasdjfslfslafdjlsadfjsafdsaljfdlsafdkjsaldfsadfsafowieuroquwreoqweurowqureowqieuroiwqeurouqwnfsalfvnn"
		"sa"
		"::::::::::::::::::::::::::"
		"safsaHHQWWEEERRTTZUIOPPPOLksdfkasdfasfdsafdljdflsafdoweroiequrwoieurqoureowquerwqorewqureiwKKJHGFDSASDFGHFDsaf"
		"dsaf"
		"d                                                                "
		"safdsafdsadfsadfsadfsadfsfasdfsadfsadfsadfsadfdsfafdafsfdsfwfwafs"
		"adfasfwafSAYXCVBVCXCVBNMNBVCXSDFGH          :::::::::,,,,,,,,,,,,,;;;;;;;;;;;////////////"
		"1231231981239123129387129387129381239817239172398127398217398172398123918273981739127391828312938721"
		"91273198273192739182739812739821731287319823712938172398712931298371982317293127381928319273921319237129" );

	base = base + base + base + base + base + base + base;

	for( int i = 0; i < cnt; ++i ) {
		std::shuffle( base.begin(), base.end(), std::random_device{} );
		ret.push_back( base );
	}
	return ret;
}

std::vector<const_string> flatten( std::vector<std::vector<const_string>>& collections )
{
	const std::size_t total_size
		= std::accumulate( collections.begin(), collections.end(), size_t( 0 ), []( size_t size, const auto& e ) {
			  return size + e.size();
		  } );
	std::vector<const_string> ret( total_size );

	auto out_it = ret.begin();

	for( auto&& c : collections ) {
		out_it = std::move( c.begin(), c.end(), out_it );
	}

	return ret;
}

template<int Algo>
const_string run( const std::vector<const_string>& s, const std::vector<char>& split_chars )
{
	auto cstrings = s;
	for( char split_char : split_chars ) {
		std::vector<std::vector<const_string>> tmp( cstrings.size() );

		size_t i = 0;

		for( auto&& s : cstrings ) {
			static_assert( 0 < Algo && Algo < 2 , "No algorithm with that number available at the moment" );
			if constexpr( Algo == 1 ) {
				tmp[i++] = s.split_full( split_char );
			} /*else {
				// put other algorithm here
			} */
		}
		cstrings = flatten( tmp );
	}

	return concat( cstrings );
}


template<int Algo>
// __declspec(noinline)
void test_algo( const std::vector<const_string>& s, const std::vector<char>& split_chars )
{
	using namespace std::chrono;
	for( int z = 0; z < 10; ++z ) {
		std::chrono::nanoseconds total{};
		for( int i = 0; i < 20; ++i ) {

			auto start = steady_clock::now();

			run<Algo>( s, split_chars );

			auto end = steady_clock::now();
			total += ( end - start );
		}
		std::cout << total / std::chrono::milliseconds{1} / 20 << std::endl;
	}

}



int main()
{
	const auto my_strings = generate_random_strings(40);

	const std::vector<char>   split_chars{' ', ':', '/', ';', ','};
	std::vector<const_string> cstrings;

	for( const auto& s : my_strings ) {
		cstrings.push_back( const_string( s ) );
	}

	test_algo<1>( cstrings, split_chars );
	std::cout << "========================================================" << std::endl;
	//test_algo<2>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<3>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<3>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<2>( cstrings, split_chars );
	//std::cout << "========================================================" << std::endl;
	//test_algo<1>( cstrings, split_chars );
	std::cout << detail::stats().get_total_allocs() << std::endl;
	std::cout << "========================================================" << std::endl;
}
