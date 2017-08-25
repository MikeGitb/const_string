#ifndef CONST_STRING_CONST_STRING_H
#define CONST_STRING_CONST_STRING_H

#include "detail/ref_cnt_buf.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <string_view>

class const_zstring;
class const_string : public std::string_view {
	using Base_t = std::string_view;

public:
	/* #################### CTORS ########################## */
	// Default ConstString points at empty string
	constexpr const_string() noexcept = default;

	const_string( std::string_view other ) { _copyFrom( other ); }

	// NOTE: Use only for string literals (arrays with static storage duration)!!!
	template <size_t N>
	constexpr const_string( const char ( &other )[N] ) noexcept
		: std::string_view( other )
	// we don't have to initialize the shared_ptr to anything as string litterals already have static lifetime
	{
	}

	// don't accept c-strings in the form of pointer
	// if you need to create a const_string from a c string use the explicit conversion to string_view
	template <class T>
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
		_data				= std::move( other._data );
		return *this;
	}

	/* ################## String functions  ################################# */
	const_string substr( size_t offset = 0, size_t count = npos ) const
	{
		const_string retval;
		retval._as_strview() = this->_as_strview().substr( offset, count );
		retval._data		 = this->_data;
		return retval;
	}

	const_string substr_sentinel( size_t offset, char sentinel ) const
	{
		const auto size = this->find( sentinel, offset );
		return substr( offset, size == npos ? this->size() - offset : size - offset );
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

	static inline detail::atomic_ref_cnt_buffer _allocate_null_terminated_char_buffer( size_t size )
	{
		detail::atomic_ref_cnt_buffer data( size + 1 );
		data.get()[size] = '\0'; // zero terminate
		return data;
	}

	void _copyFrom( const std::string_view other )
	{
		if( other.data() == nullptr ) {
			this->_as_strview() = std::string_view{""};
			return;
		}
		// create buffer and copy data over
		auto data = _allocate_null_terminated_char_buffer( other.size() );
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
	template <size_t N>
	constexpr const_zstring( const char ( &other )[N] ) noexcept
		: const_string( other )
	{
	}
	const char* c_str() const { return this->data(); }

private:
	template <class... ARGS>
	friend const_zstring concat( const ARGS&... args );

	//######## impl helper for concat ###############
	static void _addTo( char*& buffer, const std::string_view str )
	{
		std::copy_n( str.data(), str.size(), buffer );
		buffer += str.size();
	}

	template <class... ARGS>
	inline static void _write_to_buffer( char* buffer, const ARGS&... args )
	{
	#ifdef _MSC_VER
		int ignore[] = { (_addTo(buffer, args),0) ... };
	#else
		( _addTo( buffer, args ), ... );
	#endif // _MVC
	}

	template <class... ARGS>
	constexpr inline static size_t _total_size(const ARGS& ... args)
	{
	#ifdef _MSC_VER
		const size_t sizes[] = { args.size() ... };
		size_t accum = 0;
		for (auto i : sizes) {
			accum += i;
		}
		return accum;
	#else
		return (0 + ... + args.size());
	#endif // _MVC_VER
	}

	template <class... ARGS>
	inline static const_zstring _concat_impl( const ARGS&... args )
	{
		const size_t newSize = _total_size(args...);//( 0 + ... + args.size() );
		auto		 data	 = _allocate_null_terminated_char_buffer( newSize );
		_write_to_buffer( data.get(), args... );
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
 * Function that can concatenate an arbitrary number of objects from which a mart::string_view can be constructed
 * returned constStr will always be zero terminated
 */
template <class... ARGS>
const_zstring concat( const ARGS&... args )
{
	return const_zstring::_concat_impl( std::string_view( args )... );
}

inline const const_string& getEmptyConstString()
{
	const static const_string str{};
	return str;
}

#define CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(NAME , CMP)								\
template<class T1, class T2,																\
	class = std::enable_if_t<std::is_convertible<T1, std::string_view>::value>,				\
	class = std::enable_if_t<std::is_convertible<T2, std::string_view>::value>>				\
constexpr bool NAME (T1&& lhs, T2&& rhs) noexcept									\
{																							\
	return static_cast<std::string_view>( lhs ) CMP static_cast<std::string_view>( rhs );	\
}																							\

CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(operator== ,== )
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(operator!= ,!= )
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(operator<  ,<  )
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(operator<= ,<= )
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(operator>  ,>  )
CONST_STRING_DEFINE_CONST_STRING_COMPARATOR(operator>= ,>= )

#undef CONST_STRING_DEFINE_CONST_STRING_COMPARATOR

#endif