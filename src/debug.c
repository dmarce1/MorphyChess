
#define IN_DEBUG_C_FILE

#include <stdlib.h>
#include "bits.h"
#include "board.h"
#include "debug.h"
#include "zobrist.h"

#ifndef NDEBUG
int debug_mode = true;
#else
int debug_mode = false;
#endif

int have_extra_message = false;
char extra_message[ 128 ];
board_t* have_board = NULL;

void __debug_register_board( board_t* b )
{
	have_board = b;
}

void __assert( int condition, const char* file_name, int line_number )
{
	if( !condition )
	{
		printf( "\n\n************************ERROR****************************************\n" );
		printf(     "                   ASSERTION FAILED\n" );
		printf(     "  File Name  : %s\n", file_name );
		printf(     "  Line Number: %i\n", line_number );
		printf(     "*********************************************************************\n\n" );
		if( have_board != NULL )
		{
			board_to_stream( have_board, stdout );
		}
		if( have_extra_message == true )
		{
			printf( "%s\n", extra_message );
			have_extra_message = false;
		}
		abort();
	}
}


void __assert_board_consistency( board_t* b, const char* file, int line )
{
	int i, piece;
	int piece_counts[ 2 ][ 7 ];
	uqword piece_boards[ 2 ][ 7 ];
	uqword occupied_boards;
	have_extra_message = true;
	assert( b != NULL );
	sprintf( extra_message, "Board Consistency Check called from line %i in %s", line, file );
	
	occupied_boards = 0LL;
	for( i = 0; i < 7; i++ )
	{
		piece_counts[ white ][ i ] = 
		piece_counts[ black ][ i ] = 0;
		piece_boards[ white ][ i ] = 
		piece_boards[ black ][ i ] = 0LL;
	}
	occupied_boards = 0x0LL;
	for( i = 0; i < 64; i++ )
	{
		piece = b->piece_values[ i ];
		if( piece > 0 )
		{
			assert( piece <= king );
			piece_counts[ white ][ 0 ]++;
			piece_counts[ white ][ piece ]++;
			bits_set( &piece_boards[ white ][ 0 ], i );
			bits_set( &piece_boards[ white ][ piece ], i );
			bits_set( &occupied_boards, i );
		}
		else if( piece < 0 )
		{
			piece = -piece;
			assert( piece >= -king );
			piece_counts[ black ][ 0 ]++;
			piece_counts[ black ][ piece ]++;
			bits_set( &piece_boards[ black ][ 0 ], i );
			bits_set( &piece_boards[ black ][ piece ], i );
			bits_set( &occupied_boards, i );
		}
	}
	assert_range( 1, piece_counts[ white ][ 0 ], 16 );	
	assert_range( 1, piece_counts[ black ][ 0 ], 16 );	
	assert( piece_counts[ white ][ king ] == 1 );	
	assert( piece_counts[ black ][ king ] == 1 );	
	assert( piece_counts[ white ][ pawn ] <= 8 );	
	assert( piece_counts[ black ][ pawn ] <= 8 );	
	for( i = knight; i <= rook; i++ )
	{
		assert( piece_counts[ white ][ i ] <= 10 );	
		assert( piece_counts[ black ][ i ] <= 10 );	
	}
	assert( piece_counts[ white ][ queen ] <= 9 );	
	assert( piece_counts[ black ][ queen ] <= 9 );	
	assert( b->occupied_squares[ 0 ] == occupied_boards );
	for( i = 0; i < 7; i++ )
	{
		assert( piece_boards[ white ][ i ] == b->piece_boards[ white ][ i ] );
		assert( piece_boards[ black ][ i ] == b->piece_boards[ black ][ i ] );
		assert( piece_counts[ white ][ i ] == b->piece_counts[ white ][ i ] );
		assert( piece_counts[ black ][ i ] == b->piece_counts[ black ][ i ] );
	}
	assert( b->_50_move_rule <= 100 );
	if( b->flags.side_to_move == white )
	{
		assert( (b->flags.ep_capture_square / 8 == 4) || (b->flags.ep_capture_square == 0) );
		if( b->piece_values[ E1 ] != king )
		{
			assert( b->flags.white_king_side == 0 );
			assert( b->flags.white_queen_side == 0 );
		}
		else
		{
			if( b->piece_values[ A1 ] != rook )
			{
				assert( b->flags.white_queen_side == 0 );
			}
			if( b->piece_values[ H1 ] != rook )
			{
				assert( b->flags.white_king_side == 0 );
			}
		}
	}
	else if( b->flags.side_to_move == black )
	{
		assert( b->flags.most_recent_capture <= king );
		assert( (b->flags.ep_capture_square / 8 == 3) || (b->flags.ep_capture_square == 0) );
		if( b->piece_values[ E8 ] != -king )
		{
			assert( b->flags.black_king_side == 0 );
			assert( b->flags.black_queen_side == 0 );
		}
		else
		{
			if( b->piece_values[ A8 ] != -rook )
			{
				assert( b->flags.black_queen_side == 0 );
			}
			if( b->piece_values[ H8 ] != -rook )
			{
				assert( b->flags.black_king_side == 0 );
			}
		}
	}
	else
	{
		assert( false );
	}
	__assert_transposition_key_consistency( b, file, line );
	have_extra_message = false;
}

void __assert_transposition_key_consistency( board_t* board, const char* file, int line )
{
	uqword test_key;
	test_key = compute_position_key( board );
	if( test_key != board->position_key )
	{
		printf( "0x%llx 0x%llx\n", test_key, board->position_key );
		sprintf( extra_message, "Transposition Key Consistency Check called from line %i in %s", line, file );
		assert( false );
	}
	test_key = compute_pawn_key( board );
	if( test_key != board->pawn_key )
	{
		printf( "0x%llx 0x%llx\n", test_key, board->pawn_key );
		sprintf( extra_message, "Pawn Key Consistency Check called from line %i in %s", line, file );
		assert( false );
	}
}

