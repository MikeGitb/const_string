#include <const_string/detail/allocation.h>

#include <catch2/catch.hpp>

#include <iostream>


TEST_CASE("Bit twiddeling") {

	CHECK( mba::detail::get_index_of_highest_set_bit( 0 ) == -1 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 1 ) == 0 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 2 ) == 1 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 3 ) == 1 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 4 ) == 2 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 5 ) == 2 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 6 ) == 2 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 7 ) == 2 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 8 ) == 3 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 9 ) == 3 );
	CHECK( mba::detail::get_index_of_highest_set_bit( 0b110010101 ) == 8 );

}

TEST_CASE( "Block_ids" )
{
	using mba::detail::Block;

	std::vector < std::unique_ptr<Block>> blocks(7);

	for( int i = 0; i < 7; ++i ) {
		blocks[i] = std::make_unique<Block>( Block::block_id_t{i+1} );
	}

	blocks[0]->left_child() = blocks[1].get();
	blocks[0]->right_child() = blocks[2].get();

	blocks[1]->left_child()  = blocks[3].get();
	blocks[1]->right_child() = blocks[4].get();

	blocks[2]->left_child()  = blocks[5].get();
	blocks[2]->right_child() = blocks[6].get();

	for( auto& b : blocks ) {
		if( b->left_child() ) {
			CHECK( mba::detail::left_child_id( b->id() ) == b->left_child().load()->id() );
		}
		if( b->right_child() ) {
			CHECK( mba::detail::right_child_id( b->id() ) == b->right_child().load()->id() );
		}
		CHECK( &mba::detail::get_block( *blocks[0], b->id() ) == b.get() );
	}

}

TEST_CASE( "DataHandle" )
{
	mba::detail::DataHandle h{};

	CHECK( !h );
}