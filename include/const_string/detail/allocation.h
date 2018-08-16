#pragma once

#include "tree.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>


namespace mba::detail {

constexpr int get_index_of_highest_set_bit( int val )
{
	int cnt = 0;
	while( val ) {
		val >>= 1;
		cnt++;
	}
	return cnt - 1;
}

template<class T>
static constexpr auto to_uType( T v )
{
	return static_cast<std::underlying_type_t<T>>( v );
}

struct Block {
	static constexpr int block_size    = 512;
	static constexpr int local_bit_cnt = get_index_of_highest_set_bit( block_size );

	enum class block_id_t : std::int32_t {};
	enum class local_id_t : std::int32_t {};
	static constexpr block_id_t invalid_block_id{0};
	static constexpr local_id_t invalid_local_id{block_size};

private:
	// data ordered according to decreasing frequency of change (separates frequently and rarely changing data into
	// separate cache lines)
	std::atomic_int                         _free_cnt{block_size};
	std::array<std::atomic_int, block_size> _cnt{0};
	std::array<const char*, block_size>           _data{};

	std::atomic<Block*> _children[2]{nullptr, nullptr};
	const block_id_t    _blockId;

public:
	Block( const block_id_t id ) noexcept
		: _blockId( id )
	{
		assert( _free_cnt == block_size );
	}

	block_id_t id() const { return _blockId; }

	const char* data( local_id_t id ) const { return _data[to_uType( id )]; }
	void        set_data( local_id_t id, const char* data ) { _data[to_uType( id )] = data; }

	int cnt( local_id_t id ) const { return _cnt[to_uType( id )].load( std::memory_order_relaxed ); }
	int free_cnt() const { return _free_cnt.load( std::memory_order_relaxed ); }

	std::atomic<Block*>& child( int idx )
	{
		assert( idx == 0 || idx == 1 );
		return _children[idx];
	}
	std::atomic<Block*>& left_child() { return _children[0]; }
	std::atomic<Block*>& right_child() { return _children[1]; }

	int _try_reserve( int start, int end )
	{
		int pos          = start;
		int expected_cnt = 0;
		while( pos != end && !_cnt[pos].compare_exchange_strong( expected_cnt, 1 ) ) {
			const auto pos_large = std::find( _cnt.begin() + pos, _cnt.begin() + end, 0 ) - _cnt.begin();
			pos                  = static_cast<int>( pos_large );
			expected_cnt         = 0;
		}
		if( pos != end ) {
			_free_cnt--;
		}
		return pos;
	}

	local_id_t reserve_first_free_slot( int hint = 0 )
	{
		if( _free_cnt.load( std::memory_order_relaxed ) == 0 ) {
			return local_id_t{block_size};
		}

		auto pos = _try_reserve( hint, block_size );
		if( pos != block_size ) {
			return local_id_t{pos};
		}

		pos = _try_reserve( 0, hint );
		if( pos != hint ) {
			return local_id_t{pos};
		}

		return local_id_t{block_size};
	}

	void inc_ref( local_id_t id ) { _cnt[to_uType( id )].fetch_add( 1, std::memory_order_relaxed ); }
	void inc_ref( local_id_t id, int cnt ) { _cnt[to_uType( id )].fetch_add( cnt, std::memory_order_relaxed ); }

	enum class LastRef { False, True };

	LastRef dec_ref( local_id_t id )
	{
		int prev_cnt = _cnt[to_uType( id )].fetch_sub( 1 );
		if( prev_cnt == 1 ) {
			_free_cnt++;
			return LastRef::True;
		}
		return LastRef::False;
	}
};

constexpr Block::block_id_t left_child_id( Block::block_id_t parent_id )
{
	return Block::block_id_t{to_uType( parent_id ) << 1};
}
constexpr Block::block_id_t right_child_id( Block::block_id_t parent_id )
{
	return Block::block_id_t{( to_uType( parent_id ) << 1 ) + 1};
}

struct BlockData {
	static constexpr int block_size    = 512;
	static constexpr int local_bit_cnt = get_index_of_highest_set_bit( block_size );

	enum class local_id_t : std::int32_t {};
	static constexpr local_id_t invalid_local_id{block_size};

private:
	// data ordered according to decreasing frequency of change (separates frequently and rarely changing data into
	// separate cache lines)
	std::atomic_int                         _free_cnt{block_size};
	std::array<std::atomic_int, block_size> _cnt{0};
	std::array<const char*, block_size>     _data{};

public:

	const char* data( local_id_t id ) const { return _data[to_uType( id )]; }
	void        set_data( local_id_t id, const char* data ) { _data[to_uType( id )] = data; }

	int cnt( local_id_t id ) const { return _cnt[to_uType( id )].load( std::memory_order_relaxed ); }
	int free_cnt() const { return _free_cnt.load( std::memory_order_relaxed ); }

	int _try_reserve( int start, int end )
	{
		int pos          = start;
		int expected_cnt = 0;
		while( pos != end && !_cnt[pos].compare_exchange_strong( expected_cnt, 1 ) ) {
			const auto pos_large = std::find( _cnt.begin() + pos, _cnt.begin() + end, 0 ) - _cnt.begin();
			pos                  = static_cast<int>( pos_large );
			expected_cnt         = 0;
		}
		if( pos != end ) {
			_free_cnt--;
		}
		return pos;
	}

	local_id_t reserve_first_free_slot( int hint = 0 )
	{
		if( _free_cnt.load( std::memory_order_relaxed ) == 0 ) {
			return local_id_t{block_size};
		}

		auto pos = _try_reserve( hint, block_size );
		if( pos != block_size ) {
			return local_id_t{pos};
		}

		pos = _try_reserve( 0, hint );
		if( pos != hint ) {
			return local_id_t{pos};
		}

		return local_id_t{block_size};
	}

	void inc_ref( local_id_t id ) { _cnt[to_uType( id )].fetch_add( 1, std::memory_order_relaxed ); }
	void inc_ref( local_id_t id, int cnt ) { _cnt[to_uType( id )].fetch_add( cnt, std::memory_order_relaxed ); }

	enum class LastRef { False, True };

	LastRef dec_ref( local_id_t id )
	{
		int prev_cnt = _cnt[to_uType( id )].fetch_sub( 1 );
		if( prev_cnt == 1 ) {
			_free_cnt++;
			return LastRef::True;
		}
		return LastRef::False;
	}
};



class DataHandle {

public:
	constexpr DataHandle() noexcept = default;
	constexpr DataHandle( nullptr_t ) noexcept {};
	constexpr DataHandle( Block::block_id_t block_id, Block::local_id_t local_id ) noexcept
		: _id( to_uType( block_id ) << Block::local_bit_cnt || to_uType( local_id ) )
	{
		assert( block_id != Block::invalid_block_id );
		assert( local_id != Block::invalid_local_id );
		assert( _id >= 0 );
	};

	constexpr auto blockId() const { return Block::block_id_t{_id >> Block::local_bit_cnt}; }
	constexpr auto localId() const { return Block::local_id_t{_id & Block::block_size - 1}; }

	void  inc_ref() const;
	void  inc_ref( int cnt ) const;
	void  dec_ref() const;
	const char* get_data() const;

	constexpr explicit operator bool() const { return _id >= 0; }

private:
	Block& get_block() const;

	int _id{-1};
};

inline Block& get_block( Block& root, const Block::block_id_t id )
{
	if( id == Block::invalid_block_id ) {
		return root;
	}
	const int depth  = get_index_of_highest_set_bit( to_uType( id ) );
	auto      cblock = &root;
	for( int i = depth - 1; i >= 0; --i ) {
		// scan id from highest to lowest
		cblock = cblock->child( ( to_uType( id ) >> i ) & 0x1 );
	}
	return *cblock;
}

class Allocator {
public:
	constexpr Allocator() noexcept = default;
	Block& head() noexcept { return head_block; }

	struct AllocResult {
		char*      data;
		DataHandle handle;
	};

	AllocResult alloc( size_t size )
	{
		auto buffer = new char[size + 1];

		auto       depth  = _depth.load();
		DataHandle handle = try_place_recursive( head_block, buffer );

		while( !handle ) {
			// we failed to find a free slot, so we have to allocate another level,
			// but only if no one else is currently doing that
			if( _depth.compare_exchange_strong( depth, depth + 1 ) ) {
				alloc_blocks_recursive( head_block, _depth );
			}
			handle = try_place_recursive( head_block, buffer );
		}

		return {buffer, handle};
	}

private:
	Block head_block{Block::block_id_t{1}};

	std::atomic_int _depth{0};

	void alloc_blocks_recursive( Block& b, int depth )
	{
		if( depth == 0 ) {
			return;
		}
		alloc_child_if_neccessary( b.left_child(), left_child_id( b.id() ) );
		alloc_child_if_neccessary( b.right_child(), right_child_id( b.id() ) );

		alloc_blocks_recursive( *b.left_child(), depth - 1 );
		alloc_blocks_recursive( *b.right_child(), depth - 1 );
	}


	void alloc_child_if_neccessary( std::atomic<Block*>& child, Block::block_id_t expected_id )
	{
		if( !child ) {
			Block* expected = nullptr;
			auto   newBlock = new Block( expected_id );
			if( !child.compare_exchange_strong( expected, newBlock ) ) {
				// someone else was faster
				delete( newBlock );
			}
		}
	}



	DataHandle try_place_at( Block& b, const char* data )
	{
		const Block::local_id_t pos = b.reserve_first_free_slot();

		if( pos == Block::invalid_local_id ) {
			return {};
		}

		delete[]( b.data( pos ) );
		b.set_data( pos, data );
		return DataHandle( b.id(), pos );
	}

	DataHandle try_place_recursive( Block& b, const char* data )
	{
		DataHandle res = try_place_at( b, data );
		if( !res && b.left_child() ) {
			res = try_place_recursive( *b.left_child(), data );
			if( !res ) {
				res = try_place_recursive( *b.right_child(), data );
			}
		}
		return res;
	}
};

inline Allocator alloc{};

inline Allocator& getAllocator()
{
	return alloc;
}

inline Block& DataHandle::get_block() const
{
	return ::mba::detail::get_block( getAllocator().head(), blockId() );
}

// clang-format off
inline void  DataHandle::inc_ref()			const { get_block().inc_ref( localId()      ); }
inline void  DataHandle::inc_ref( int cnt )	const {	get_block().inc_ref( localId(), cnt ); }
inline void  DataHandle::dec_ref()			const {	get_block().dec_ref( localId()      ); }
inline const char* DataHandle::get_data()	const {	return get_block().data( localId() );  }
// clang-format on

inline Allocator::AllocResult allocate( std::size_t s )
{
	return getAllocator().alloc( s );
}
}; // namespace mba::detail
