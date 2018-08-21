#ifndef CONST_STRING_DETAIL_REF_CNT_BUF_H
#define CONST_STRING_DETAIL_REF_CNT_BUF_H

#include "allocation.h"
//#include "handle.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <new>
#include <string_view>
#include <utility>

namespace mba::const_string::detail {

#ifdef CONST_STRING_DEBUG_HOOKS
struct Stats {
	std::atomic_uint64_t total_cnt_accesses{0};
	std::atomic_uint64_t total_allocs{0};
	std::atomic_uint64_t current_allocs{0};
	std::atomic_uint64_t inc_ref_cnt{0};
	std::atomic_uint64_t dec_ref_cnt{0};

	void inc_ref()
	{
		total_cnt_accesses.fetch_add( 1, std::memory_order_relaxed );
		inc_ref_cnt.fetch_add( 1, std::memory_order_relaxed );
	}

	void dec_ref()
	{
		total_cnt_accesses.fetch_add( 1, std::memory_order_relaxed );
		dec_ref_cnt.fetch_add( 1, std::memory_order_relaxed );
	}

	void alloc()
	{
		total_allocs.fetch_add( 1, std::memory_order_relaxed );
		current_allocs.fetch_add( 1, std::memory_order_relaxed );
	}

	void dealloc() { current_allocs.fetch_sub( 1, std::memory_order_relaxed ); }

	std::uint64_t get_total_cnt_accesses() const { return total_cnt_accesses.load( std::memory_order_relaxed ); };
	std::uint64_t get_total_allocs() const { return total_allocs.load( std::memory_order_relaxed ); };
	std::uint64_t get_current_allocs() const { return current_allocs.load( std::memory_order_relaxed ); };
	std::uint64_t get_inc_ref_cnt() const { return inc_ref_cnt.load( std::memory_order_relaxed ); };
	std::uint64_t get_dec_ref_cnt() const { return dec_ref_cnt.load( std::memory_order_relaxed ); };

	constexpr Stats() noexcept = default;
	Stats( const Stats& other )
		: total_cnt_accesses( other.total_cnt_accesses.load( std::memory_order_relaxed ) )
		, total_allocs( other.total_allocs.load( std::memory_order_relaxed ) )
		, current_allocs( other.current_allocs.load( std::memory_order_relaxed ) )
		, inc_ref_cnt( other.inc_ref_cnt.load( std::memory_order_relaxed ) )
		, dec_ref_cnt( other.dec_ref_cnt.load( std::memory_order_relaxed ) )
	{
	}
};
#else
struct Stats {
	constexpr Stats() noexcept = default;

	constexpr void inc_ref() noexcept {}
	constexpr void dec_ref() noexcept {}
	constexpr void alloc() noexcept {}
	constexpr void dealloc() noexcept {}

	constexpr std::uint64_t get_total_cnt_accesses() const noexcept { return 0; };
	constexpr std::uint64_t get_total_allocs() const noexcept { return 0; };
	constexpr std::uint64_t get_current_allocs() const noexcept { return 0; };
	constexpr std::uint64_t get_inc_ref_cnt() const noexcept { return 0; };
	constexpr std::uint64_t get_dec_ref_cnt() const noexcept { return 0; };
};
#endif

static Stats& stats()
{
	static Stats stats{};
	return stats;
}

class defer_ref_cnt_tag_t {
};

class atomic_ref_cnt_buffer {
	using Cnt_t = std::atomic_int;

	static constexpr int required_space = (int)sizeof( Cnt_t );

public:
	constexpr atomic_ref_cnt_buffer() noexcept = default;

	constexpr atomic_ref_cnt_buffer(DataHandle&& handle)
		: _handle{std::move(handle)}
	{
	}

	constexpr atomic_ref_cnt_buffer( const atomic_ref_cnt_buffer& other, defer_ref_cnt_tag_t ) noexcept
		: _handle{other._handle}
	{
	}

	atomic_ref_cnt_buffer( const atomic_ref_cnt_buffer& other ) noexcept
		: _handle{other._handle}
	{
		_incref();
	}

	atomic_ref_cnt_buffer( atomic_ref_cnt_buffer&& other ) noexcept
		: _handle{std::exchange( other._handle, nullptr )}
	{
	}

	atomic_ref_cnt_buffer& operator=( const atomic_ref_cnt_buffer& other ) noexcept
	{
		// inc before dec to protect against dropping in self assignment
		other._incref();
		_decref();
		_handle = other._handle;

		return *this;
	}

	atomic_ref_cnt_buffer& operator=( atomic_ref_cnt_buffer&& other ) noexcept
	{
		assert( this != &other && "Move assignment to self is not allowed" );
		_decref();
		_handle = std::exchange( other._handle, nullptr );
		return *this;
	}

	~atomic_ref_cnt_buffer() { _decref(); }

	const char* get() noexcept { return _handle.get_data(); }

	friend void swap( atomic_ref_cnt_buffer& l, atomic_ref_cnt_buffer& r ) noexcept { std::swap( l._handle, r._handle ); }

	void add_ref_cnt( int cnt ) const
	{
		if( !_handle ) {
			return ;
		}
		stats().inc_ref();
		_handle.inc_ref(cnt);
	}

private:
	void _decref() const noexcept
	{
		if( !_handle ) {
			return ;
		}
		stats().dec_ref();
		_handle.dec_ref();
	}

	void _incref() const noexcept
	{
		if( !_handle ) {
			return ;
		}
		stats().inc_ref();
		_handle.inc_ref();
	}

	DataHandle _handle {};
};


struct AllocResult {
	char*                 data;
	atomic_ref_cnt_buffer handle;
};

inline AllocResult allocate_null_terminated_char_buffer( int size )
{
	stats().alloc();
	auto ret = alloc.alloc( size + 1 );

	atomic_ref_cnt_buffer handle( std::move(ret.handle) );

	auto data = ret.data;

	data[size] = '\0'; // zero terminate
	return {data, std::move( handle )};
}

inline constexpr std::string_view getEmptyZeroTerminatedStringView()
{
	return std::string_view{""};
}

} // namespace detail

#endif
