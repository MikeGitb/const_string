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

namespace mba::const_string::detail {

struct AllocSlots {
	static constexpr std::int32_t block_size    = 512;
	static constexpr std::int32_t local_bit_cnt = get_index_of_highest_set_bit( block_size );

	enum class local_id_t : std::int32_t { Invalid = block_size };
	static constexpr local_id_t invalid_local_id{block_size};

private:
	// data ordered according to decreasing frequency of change (separates frequently and rarely changing data into
	// separate cache lines)
	std::atomic_int                         _free_cnt{block_size};
	std::array<std::atomic_int, block_size> _cnt{0};
	std::array<const char*, block_size>     _data{};

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

public:
	const char* data( local_id_t id ) const { return _data[to_uType( id )]; }
	void        set_data( local_id_t id, const char* data ) { _data[to_uType( id )] = data; }

	int cnt( local_id_t id ) const { return _cnt[to_uType( id )].load( std::memory_order_relaxed ); }
	int free_cnt() const { return _free_cnt.load( std::memory_order_relaxed ); }

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
	constexpr DataHandle( node_id_t block_id, AllocSlots::local_id_t local_id ) noexcept
		: _id{fuse( block_id, local_id )}
	{
		assert( block_id != node_id_t::Invalid );
		assert( local_id != AllocSlots::local_id_t::Invalid );
		assert( _id > 0 );
		assert( blockId() == block_id );
		assert( localId() == local_id );
	};

	constexpr node_id_t              blockId() const { return node_id_t{_id >> AllocSlots::local_bit_cnt}; }
	constexpr AllocSlots::local_id_t localId() const
	{
		return AllocSlots::local_id_t{_id & AllocSlots::block_size - 1};
	}

	void        inc_ref() const;
	void        inc_ref( int cnt ) const;
	void        dec_ref() const;
	const char* get_data() const;

	constexpr explicit operator bool() const { return _id != Invalid_id; }

private:
	AllocSlots& get_slot_block() const;

	using id_t                       = unsigned int;
	static constexpr id_t Invalid_id = -1;	//TODO: switch to 0

	id_t _id = Invalid_id;

	constexpr static id_t fuse( node_id_t block_id, AllocSlots::local_id_t local_id ) noexcept
	{
		return ( to_uType( block_id ) << AllocSlots::local_bit_cnt ) | to_uType( local_id );
	}
};

class Allocator {
public:
	constexpr Allocator() noexcept = default;

	struct AllocResult {
		char*      data;
		DataHandle handle;
	};

	AllocResult alloc( size_t size )
	{
		auto buffer = new char[size + 1];

		DataHandle handle = try_place_recursive( _head(), buffer );

		while( !handle ) {
			// we failed to find a free slot, so we have to allocate another level,
			// but only if no one else is currently doing that
			_slots.grow_by_one_level();

			handle = try_place_recursive( _head(), buffer );
		}

		return {buffer, handle};
	}

	Tree<AllocSlots>& slots() noexcept { return _slots; }

private:
	Tree<AllocSlots> _slots;

	Node<AllocSlots>& _head() noexcept { return _slots.root; }

	static void alloc_child_if_neccessary( std::atomic<Node<AllocSlots>*>& child, node_id_t expected_id )
	{
		if( !child ) {
			Node<AllocSlots>* expected = nullptr;
			auto   newBlock = new Node<AllocSlots>( expected_id );
			if( !child.compare_exchange_strong( expected, newBlock ) ) {
				// someone else was faster
				delete( newBlock );
			}
		}
	}

	static DataHandle try_place_at( Node<AllocSlots>& b, const char* data )
	{
		const AllocSlots::local_id_t pos = b.data().reserve_first_free_slot();

		if( pos == AllocSlots::local_id_t::Invalid ) {
			return {};
		}

		delete[]( b.data().data( pos ) );
		b.data().set_data( pos, data );
		return DataHandle( b.id(), pos );
	}

	static DataHandle try_place_recursive( Node<AllocSlots>& b, const char* data )
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

inline AllocSlots& DataHandle::get_slot_block() const
{
	return getAllocator().slots().getNode( blockId() ).data();
}

// clang-format off
inline void  DataHandle::inc_ref()			const { get_slot_block().inc_ref( localId()      ); }
inline void  DataHandle::inc_ref( int cnt )	const {	get_slot_block().inc_ref( localId(), cnt ); }
inline void  DataHandle::dec_ref()			const {	get_slot_block().dec_ref( localId()      ); }
inline const char* DataHandle::get_data()	const {	return get_slot_block().data( localId() );  }
// clang-format on

inline Allocator::AllocResult allocate( std::size_t s )
{
	return getAllocator().alloc( s );
}
}; // namespace mba::const_string::detail
