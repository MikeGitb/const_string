#ifndef CONST_STRING_DETAIL_REF_CNT_BUF_H
#define CONST_STRING_DETAIL_REF_CNT_BUF_H

#include <atomic>
#include <cassert>
#include <cstdint>
#include <new>
#include <string_view>
#include <utility>

namespace detail {

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

	constexpr std::uint64_t get_total_cnt_accesses() noexcept const { return 0; };
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

	constexpr atomic_ref_cnt_buffer( const atomic_ref_cnt_buffer& other, defer_ref_cnt_tag_t ) noexcept
		: _cnt{other._cnt}
	{
	}

	explicit atomic_ref_cnt_buffer( int buffer_size )
	{
		stats().alloc();
		auto data = new char[buffer_size + required_space];
		_cnt      = new( data ) Cnt_t{1};

		// TODO: Is this guaranteed by the standard?
		assert( reinterpret_cast<char*>( _cnt ) == data );
	}

	atomic_ref_cnt_buffer( const atomic_ref_cnt_buffer& other ) noexcept
		: _cnt{other._cnt}
	{
		_incref();
	}

	atomic_ref_cnt_buffer( atomic_ref_cnt_buffer&& other ) noexcept
		: _cnt{std::exchange( other._cnt, nullptr )}
	{
	}

	atomic_ref_cnt_buffer& operator=( const atomic_ref_cnt_buffer& other ) noexcept
	{
		// inc before dec to protect against dropping in self assignment
		other._incref();
		_decref();
		_cnt = other._cnt;

		return *this;
	}

	atomic_ref_cnt_buffer& operator=( atomic_ref_cnt_buffer&& other ) noexcept
	{
		assert( this != &other && "Move assignment to self is not allowed" );
		_decref();
		_cnt = std::exchange( other._cnt, nullptr );
		return *this;
	}

	~atomic_ref_cnt_buffer() { _decref(); }

	char* get() noexcept { return reinterpret_cast<char*>( _cnt ) + sizeof( Cnt_t ); }

	friend void swap( atomic_ref_cnt_buffer& l, atomic_ref_cnt_buffer& r ) noexcept { std::swap( l._cnt, r._cnt ); }

	int get_ref_cnt() const
	{
		if( !_cnt ) {
			return 0;
		}
		return _cnt->load( std::memory_order_acquire );
	}

	int add_ref_cnt( int cnt ) const
	{
		if( !_cnt ) {
			return 0;
		}
		stats().inc_ref();
		return _cnt->fetch_add( cnt, std::memory_order_relaxed ) + cnt;
	}

private:
	void _decref() const noexcept
	{
		if( _cnt ) {
			stats().dec_ref();
			if( _cnt->fetch_sub( 1 ) == 1 ) {
				stats().dealloc();
				_cnt->~Cnt_t();
				delete[]( reinterpret_cast<char*>( _cnt ) );
			}
		}
	}

	void _incref() const noexcept
	{
		if( _cnt ) {
			stats().inc_ref();
			_cnt->fetch_add( 1, std::memory_order_relaxed );
		}
	}

	Cnt_t* _cnt = nullptr;
};

struct AllocResult {
	char*                 data;
	atomic_ref_cnt_buffer handle;
};

inline AllocResult allocate_null_terminated_char_buffer( int size )
{
	atomic_ref_cnt_buffer handle( size + 1 );
	auto                  data = handle.get();

	data[size] = '\0'; // zero terminate
	return {data,std::move(handle)};
}

inline constexpr std::string_view getEmptyZeroTerminatedStringView()
{
	return std::string_view{""};
}

} // namespace detail

#endif
