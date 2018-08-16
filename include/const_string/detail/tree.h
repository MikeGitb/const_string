#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>

namespace mba::const_string::detail {

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
constexpr auto to_uType( T v ) noexcept
{
	return static_cast<std::underlying_type_t<T>>( v );
}

enum class node_id_t : std::uint32_t { Invalid = 0 };

constexpr node_id_t left_child_id( node_id_t parent_id ) noexcept
{
	return node_id_t{to_uType( parent_id ) << 1};
}

constexpr node_id_t right_child_id( node_id_t parent_id ) noexcept
{
	return node_id_t{( to_uType( parent_id ) << 1 ) + 1};
}

template<class T>
struct Node {
	using child_ptr_t = std::atomic<Node<T>*>;

	// data ordered according to decreasing frequency of change (separates frequently and rarely changing data into
	// separate cache lines)

	T                          _data{};
	std::array<child_ptr_t, 2> _children{nullptr, nullptr};
	const node_id_t            _blockId;

	constexpr Node( const node_id_t id ) noexcept
		: _blockId( id )
	{
	}

	constexpr Node( const node_id_t id, T data ) noexcept
		: _data( std::move( data ) )
		, _blockId( id )
	{
	}

	constexpr node_id_t id() const noexcept { return _blockId; }

	child_ptr_t& left_child() noexcept { return _children[0]; }
	child_ptr_t& right_child() noexcept { return _children[1]; }
};

constexpr int get_number_of_leading_zeros( node_id_t val )
{
	// TODO use indriniscs like __builtin_clz
	constexpr auto bit_cnt = sizeof( val ) * CHAR_BIT;
	const auto     lid     = to_uType( val );
	assert( lid != 0 );

	std::underlying_type_t<node_id_t> mask = 0x1 << ( bit_cnt - 1 );
	for( int i = 0; i < bit_cnt; i++ ) {
		if( lid & mask ) {
			return i;
		}
		assert( mask );
		mask >>= 1;
	}
	return -1;
}

template<class T>
Node<T>& get_node( Node<T>& root, const node_id_t id )
{
	const int remaining_bit_cnt = get_number_of_leading_zeros( root.id() ) - get_number_of_leading_zeros( id );
	assert( remaining_bit_cnt >= 0 );

	auto cblock = &root;
	for( int i = remaining_bit_cnt; i > 0; --i ) {
		assert( cblock != nullptr );
		// scan id from highest to lowest
		cblock = cblock->_children[( to_uType( id ) >> (i-1) ) & 0x1];
	}
	return *cblock;
}

template<class T>
class Tree {
	using node_type = Node<T>;
	node_type root{node_id_t{1}};

	std::atomic_int _height{1};

	node_type& getNode( node_id_t id ) { return get_node( root, id ); }

	void grow_to_height( int target_height )
	{
		if( target_height <= _height.load() ) {
			return;
		}

		alloc_nodes_recursive( root, target_height - 1 );

		auto start_height = _height.load();
		while( !_height.compare_exchange_weak( start_height, std::max( start_height, target_height ) ) ) {
			;
		}
	}

private:
	void alloc_nodes_recursive( node_type& b, int levels )
	{
		if( levels == 0 ) {
			return;
		}
		alloc_child_if_neccessary( b.left_child(), left_child_id( b.id() ) );
		alloc_child_if_neccessary( b.right_child(), right_child_id( b.id() ) );

		alloc_nodes_recursive( *b.left_child(), levels - 1 );
		alloc_nodes_recursive( *b.right_child(), levels - 1 );
	}

	void alloc_child_if_neccessary( std::atomic<node_type*>& child, node_id_t expected_id )
	{
		if( !child ) {
			node_type* expected = nullptr;
			auto       newBlock = new node_type( expected_id );
			if( !child.compare_exchange_strong( expected, newBlock ) ) {
				// someone else was faster
				delete( newBlock );
			}
		}
	}
};

} // namespace mba::const_string::detail
