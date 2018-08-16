#include <const_string/detail/allocation.h>

#include <catch2/catch.hpp>

#include <iostream>


using namespace mba::const_string::detail;

TEST_CASE("Bit twiddeling") {


	CHECK( get_index_of_highest_set_bit( 0 ) == -1 );
	CHECK( get_index_of_highest_set_bit( 1 ) == 0 );
	CHECK( get_index_of_highest_set_bit( 2 ) == 1 );
	CHECK( get_index_of_highest_set_bit( 3 ) == 1 );
	CHECK( get_index_of_highest_set_bit( 4 ) == 2 );
	CHECK( get_index_of_highest_set_bit( 5 ) == 2 );
	CHECK( get_index_of_highest_set_bit( 6 ) == 2 );
	CHECK( get_index_of_highest_set_bit( 7 ) == 2 );
	CHECK( get_index_of_highest_set_bit( 8 ) == 3 );
	CHECK( get_index_of_highest_set_bit( 9 ) == 3 );
	CHECK( get_index_of_highest_set_bit( 0b110010101 ) == 8 );

}

TEST_CASE( "Block_ids" )
{
	std::vector < std::unique_ptr<Node<int>>> blocks(7);

	for( unsigned int i = 0; i < 7; ++i ) {
		blocks[i] = std::make_unique<Node<int>>( node_id_t{i + 1} );
	}

	blocks[0]->left_child() = blocks[1].get();
	blocks[0]->right_child() = blocks[2].get();

	blocks[1]->left_child()  = blocks[3].get();
	blocks[1]->right_child() = blocks[4].get();

	blocks[2]->left_child()  = blocks[5].get();
	blocks[2]->right_child() = blocks[6].get();

	for( auto& b : blocks ) {
		if( b->left_child() ) {
			CHECK( left_child_id( b->id() ) == b->left_child().load()->id() );
		}
		if( b->right_child() ) {
			CHECK( right_child_id( b->id() ) == b->right_child().load()->id() );
		}
		CHECK( &get_node( *blocks[0], b->id() ) == b.get() );
	}

}

TEST_CASE( "DataHandle" )
{
	DataHandle h{};

	CHECK( !h );

	for( unsigned int nid = 1; nid < 100; ++nid ) {
		auto node_id = node_id_t{nid};
		for( int lid = 0; lid < AllocSlots::block_size; ++lid ) {
			auto       local_id = AllocSlots::local_id_t{lid};
			DataHandle h1(node_id , local_id );
			CHECK( h1.localId() == local_id );
			CHECK( h1.blockId() == node_id );
			CHECK( (bool)h1 );
		}
	}
}