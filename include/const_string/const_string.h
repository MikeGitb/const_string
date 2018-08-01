#ifndef CONST_STRING_CONST_STRING_H
#define CONST_STRING_CONST_STRING_H

#include "detail/ref_cnt_buf.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

class const_zstring;
class const_string : public std::string_view {
	using Base_t = std::string_view;

public:
	/* #################### CTORS ########################## */
	// Default ConstString points at empty string
	constexpr const_string() noexcept = default;

	const_string( std::string_view other ) { _copyFrom( other ); }

	// NOTE: Use only for string literals (arrays with static storage duration)!!!
	template<size_t N>
	constexpr const_string( const char ( &other )[N] ) noexcept
		: std::string_view( other )
	// we don't have to initialize the shared_ptr to anything as string litterals already have static lifetime
	{
	}

	// don't accept c-strings in the form of pointer
	// if you need to create a const_string from a c string use the explicit conversion to string_view
	template<class T>
	const_string( T const* const& other ) = delete;

	/* ############### Special member functions ######################################## */
	const_string( const const_string& other ) noexcept = default;
	const_string& operator=( const const_string& other ) noexcept = default;

	const_string( const_string&& other ) noexcept
		: std::string_view( std::exchange( other._as_strview(), std::string_view{} ) )
		, _data( std::move( other._data ) )
	{
	}

	const_string& operator=( const_string&& other ) noexcept
	{
		this->_as_strview() = std::exchange( other._as_strview(), std::string_view{} );
		_data               = std::move( other._data );
		return *this;
	}

	/* ################## String functions  ################################# */
	const_string substr( size_t offset = 0, size_t count = npos ) const
	{
		const_string retval;
		retval._as_strview() = this->_as_strview().substr( offset, count );
		retval._data         = this->_data;
		return retval;
	}

	const_string substr( std::string_view range ) const
	{
		assert( data() <= range.data() && range.data() + range.size() <= data() + size() );
		const_string retval;
		retval._as_strview() = range;
		retval._data         = this->_data;
		return retval;
	}

	const_string substr( iterator start, iterator end ) const
	{
		// UGLY: start-begin()+data() is necessary to convert from an iterator to a pointer on platforms where they are
		// not the same type
		return substr( std::string_view( start - begin() + data(), static_cast<size_type>( end - start ) ) );
	}

	const_string substr_sentinel( size_t offset, char sentinel ) const
	{
		const auto size = this->find( sentinel, offset );
		return substr( offset, size == npos ? this->size() - offset : size - offset );
	}

	enum class Split { Drop, Before, After };

	std::pair<const_string, const_string> split( std::size_t i ) const
	{
		assert( i < size() || i == npos );
		if( i == npos ) {
			return {*this, {}};
		}
		return {substr( 0, i ), substr( i, npos )};
	}

	std::pair<const_string, const_string> split( std::size_t i, Split s ) const
	{
		assert( i < size() || i == npos );
		if( i == npos ) {
			return {*this, {}};
		}
		return {substr( 0, i + ( s == Split::After ) ), substr( i + ( s == Split::After || s == Split::Drop ), npos )};
	}

	std::pair<const_string, const_string> split_first( char c = ' ', Split s = Split::Drop ) const
	{
		auto pos = this->find( c );
		return split( pos, s );
	}

	std::pair<const_string, const_string> split_last( char c = ' ', Split s = Split::Drop ) const
	{
		auto pos = this->rfind( c );
		return split( pos, s );
	}

	std::vector<const_string> split_full( char delimiter ) const
	{
		std::vector<const_string> ret;
		if( size() == 0 ) {
			return ret;
		}
		ret.reserve( 5 ); // Arbitrarily chosen value TODO: check if actually beneficial

		std::string_view self_view = this->_as_strview();

		std::size_t start_pos = 0;
		std::size_t found_pos = 0;

		while( found_pos != std::string_view::npos && start_pos != this->size() ) {

			found_pos = this->find( delimiter, start_pos );

			// std::string_view::substr(offset,count) allows count to be bigger than size,
			// so we don't have to check for npos here
			const auto new_slice = self_view.substr( start_pos, found_pos - start_pos );

			// ref count will be incremented at the end of the function, once the total number of slices will be known
			// constructor is private, so we can't use emplace_back here
			ret.push_back( const_string( new_slice, _data, detail::defer_ref_cnt_tag_t{} ) );

			start_pos = found_pos + 1;
		}

		_data.add_ref_cnt( static_cast<int>( ret.size() ) );

		return ret;
	}

	bool isZeroTerminated() const { return this->data()[size()] == '\0'; }

	const_zstring unshare() const;
	const_zstring createZStr() const&;
	const_zstring createZStr() &&;

protected:
	detail::atomic_ref_cnt_buffer _data;

	class static_lifetime_tag {
	};
	constexpr const_string( std::string_view sv, static_lifetime_tag )
		: std::string_view( sv )
	{
	}

	constexpr const_string( std::string_view                     sv,
							const detail::atomic_ref_cnt_buffer& data,
							detail::defer_ref_cnt_tag_t )
		: std::string_view( sv )
		, _data{data, detail::defer_ref_cnt_tag_t{}}
	{
	}

	/**
	 * private constructor, that takes ownership of a buffer and a size (used in _copyFrom and _concat_impl)
	 */
	const_string( detail::atomic_ref_cnt_buffer&& data, size_t size )
		: std::string_view( data.get(), size )
		, _data( std::move( data ) )
	{
	}

	friend void swap( const_string& l, const_string& r )
	{
		using std::swap;
		swap( l._as_strview(), r._as_strview() );
		swap( l._data, r._data );
	}

	std::string_view& _as_strview() { return static_cast<std::string_view&>( *this ); }

	const std::string_view& _as_strview() const { return static_cast<const std::string_view&>( *this ); }

	void _copyFrom( const std::string_view other )
	{
		if( other.data() == nullptr ) {
			this->_as_strview() = std::string_view{""};
			return;
		}
		// create buffer and copy data over
		auto data = detail::allocate_null_terminated_char_buffer( static_cast<int>( other.size() ) );
		std::copy_n( other.data(), other.size(), data.get() );

		// initialize ConstString data fields;
		*this = const_string( std::move( data ), other.size() );
	}
};

class const_zstring : public const_string {
	using const_string::const_string;

public:
	constexpr const_zstring()
		: const_string( detail::getEmptyZeroTerminatedStringView(), const_string::static_lifetime_tag{} )
	{
	}
	const_zstring( std::string_view other )
		: const_string( other.data() == nullptr ? detail::getEmptyZeroTerminatedStringView() : other )
	{
	}

	const_zstring( const const_string& other )
		: const_string( other.createZStr() )
	{
	}

	const_zstring( const_string&& other )
		: const_string( std::move( other ).createZStr() )
	{
	}

	// NOTE: Use only for string literals (arrays with static storage duration)!!!
	template<size_t N>
	constexpr const_zstring( const char ( &other )[N] ) noexcept
		: const_string( other )
	{
	}
	const char* c_str() const { return this->data(); }

private:
	template<class ARG1, class... ARGS>
	friend auto concat( const ARG1 arg1, const ARGS&... args )
		-> std::enable_if_t<std::is_convertible_v<ARG1, std::string_view>, const_zstring>;

	template<class T>
	friend auto concat( const T& args ) -> std::enable_if_t<!std::is_convertible_v<T, std::string_view>, const_zstring>;

	//######## impl helper for concat ###############
	static void _addTo( char*& buffer, const std::string_view str )
	{
		buffer = std::copy_n( str.data(), str.size(), buffer );
	}

	template<class... ARGS>
	inline static void _write_to_buffer( char* buffer, const ARGS&... args )
	{
		( _addTo( buffer, args ), ... );
	}

	template<class... ARGS>
	inline static const_zstring _concat_var_impl( const ARGS&... args )
	{
		const size_t newSize = ( 0 + ... + args.size() );
		auto         data    = detail::allocate_null_terminated_char_buffer( static_cast<int>( newSize ) );
		_write_to_buffer( data.get(), args... );
		return const_zstring( std::move( data ), newSize );
	}

	template<class T>
	inline static const_zstring _concat_range_impl( const std::vector<T>& args )
	{
		const size_t newSize
			= std::accumulate( args.begin(), args.end(), std::size_t( 0 ), []( std::size_t s, const auto& str ) {
				  return s + str.size();
			  } );

		auto data = detail::allocate_null_terminated_char_buffer( static_cast<int>( newSize ) );
		auto ptr  = data.get();
		for( auto&& e : args ) {
			_addTo( ptr, std::string_view( e ) );
		}
		return const_zstring( std::move( data ), newSize );
	}
};

inline const_zstring const_string::unshare() const
{
	return const_zstring( static_cast<std::string_view>( *this ) );
}

inline const_zstring const_string::createZStr() const&
{
	if( isZeroTerminated() ) {
		return *this; // just copy
	} else {
		return unshare();
	}
}

inline const_zstring const_string::createZStr() &&
{
	if( isZeroTerminated() ) {
		return std::move( *this ); // already zero terminated - just move
	} else {
		return unshare();
	}
}

/**
 * Function that can concatenate an arbitrary number of objects from which a std::string_view can be constructed
 */
template<class ARG1, class... ARGS>
auto concat( const ARG1 arg1, const ARGS&... args )
	-> std::enable_if_t<std::is_convertible_v<ARG1, std::string_view>, const_zstring>
{
	return const_zstring::_concat_var_impl( std::string_view( arg1 ), std::string_view( args )... );
}

template<class T>
auto concat( const T& args ) -> std::enable_if_t<!std::is_convertible_v<T, std::string_view>, const_zstring>
{
	return const_zstring::_concat_range_impl( args );
}

inline const const_string& getEmptyConstString()
{
	const static const_string str{};
	return str;
}

#define CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( NAME, CMP )                                                       \
	template<class T1,                                                                                                 \
			 class T2,                                                                                                 \
			 class = std::enable_if_t<std::is_convertible<T1, std::string_view>::value>,                               \
			 class = std::enable_if_t<std::is_convertible<T2, std::string_view>::value>>                               \
	constexpr bool NAME( T1&& lhs, T2&& rhs ) noexcept                                                                 \
	{                                                                                                                  \
		return static_cast<std::string_view>( lhs ) CMP static_cast<std::string_view>( rhs );                          \
	}

CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( operator==, ==)
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( operator!=, !=)
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( operator<, <)
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( operator<=, <=)
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( operator>, >)
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR( operator>=, >=)

#undef CONST_STRING_DEFINE_CONST_STRING_COMPARATOR

#endif
