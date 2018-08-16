#include <const_string/detail/tree.h>

#include <catch2/catch.hpp>

#include <iostream>

TEST_CASE( "Bit-twiddeling-2" )
{
	using namespace mba::const_string::detail;

	CHECK( mba::const_string::detail::get_index_of_highest_set_bit( 0 ) == -1 );
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

	CHECK( get_number_of_leading_zeros( node_id_t{1} ) == 31 );
	CHECK( get_number_of_leading_zeros( node_id_t{2} ) == 30 );
	CHECK( get_number_of_leading_zeros( node_id_t{3} ) == 30 );
	CHECK( get_number_of_leading_zeros( node_id_t{4} ) == 29 );
	CHECK( get_number_of_leading_zeros( node_id_t{5} ) == 29 );
	CHECK( get_number_of_leading_zeros( node_id_t{6} ) == 29 );
	CHECK( get_number_of_leading_zeros( node_id_t{7} ) == 29 );
	CHECK( get_number_of_leading_zeros( node_id_t{8} ) == 28 );
	CHECK( get_number_of_leading_zeros( node_id_t{9} ) == 28 );
	CHECK( get_number_of_leading_zeros( node_id_t{0b110010101} ) == 23 );
}

TEST_CASE( "Node_ids" )
{
	using namespace mba::const_string::detail;

	std::vector<std::unique_ptr<Node<int>>> blocks( 7 );

	for( unsigned int i = 0; i < 7; ++i ) {
		blocks[i] = std::make_unique<Node<int>>( node_id_t{i + 1} );
	}

	blocks[0]->left_child()  = blocks[1].get();
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
		CHECK( get_node( *blocks[0], b->id() ).id() == b->id() );
		CHECK( &get_node( *blocks[0], b->id() ) == b.get() );
	}
}