#ifndef CONST_STRING_DETAIL_REF_CNT_BUF_H
#define CONST_STRING_DETAIL_REF_CNT_BUF_H

#include <atomic>
#include <cassert>
#include <cstdint>
#include <new>
#include <string_view>

namespace detail {

class atomic_ref_cnt_buffer {
	using Cnt_t = std::atomic_int;

	static constexpr size_t required_space = sizeof( Cnt_t );

public:
	atomic_ref_cnt_buffer() = default;

	explicit atomic_ref_cnt_buffer( std::size_t buffer_size )
	{
		auto data = new char[buffer_size + required_space];
		_cnt	  = new( data ) Cnt_t{1};

		// TODO: Is this guaranteed by the standard?
		assert( reinterpret_cast<char*>( _cnt ) == data );
	}

	atomic_ref_cnt_buffer( const atomic_ref_cnt_buffer& other ) noexcept
		: _cnt{other._cnt}
	{
		_incref();
	}

	atomic_ref_cnt_buffer( atomic_ref_cnt_buffer&& other ) noexcept
		: _cnt{other._cnt}
	{
		other._cnt = nullptr;
	}

	atomic_ref_cnt_buffer& operator=( const atomic_ref_cnt_buffer& other ) noexcept
	{
		// inc before dec to protect against self assignment
		other._incref();
		_decref();
		_cnt = other._cnt;

		return *this;
	}

	atomic_ref_cnt_buffer& operator=( atomic_ref_cnt_buffer&& other ) noexcept
	{
		_decref();
		_cnt	   = other._cnt;
		other._cnt = nullptr;
		return *this;
	}

	~atomic_ref_cnt_buffer() { _decref(); }

	char* get() noexcept { return reinterpret_cast<char*>( _cnt ) + sizeof( Cnt_t ); }

	friend void swap( atomic_ref_cnt_buffer& l, atomic_ref_cnt_buffer& r ) noexcept { std::swap( l._cnt, r._cnt ); }

private:
	void _decref() const noexcept
	{
		if( _cnt ) {
			if( _cnt->fetch_sub( 1 ) == 1 ) {
				_cnt->~Cnt_t();
				delete[]( reinterpret_cast<char*>( _cnt ) );
			}
		}
	}

	void _incref() const noexcept
	{
		if( _cnt ) {
			_cnt->fetch_add( 1, std::memory_order_relaxed );
		}
	}

	Cnt_t* _cnt = nullptr;
};

inline constexpr std::string_view getEmptyZeroTerminatedStringView()
{
	return std::string_view{""};
}

}

#endif