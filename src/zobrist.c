
#include <stdlib.h>
#include "board.h"
#include "debug.h"
#include "zobrist.h"

static uqword keys[ 2 ][ 6 ][ 64 ];
static uqword ep_keys[ 8 ];
static uqword king_castle_keys[ 2 ];
static uqword queen_castle_keys[ 2 ];


static uqword rand64( void );


void init_zobrist_keys( void )
{
	int i, j, k;
	srand( 0x33333333 );
	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j < 6; j++ )
		{
			for( k = 0; k < 64; k++ )
			{
				keys[ i ][ j ][ k ] = rand64();
			}
		}
	}
	for( i = 0; i < 8; i++ )
	{
		ep_keys[ i ] = rand64();
	}
	for( i = 0; i < 2; i++ )
	{
		king_castle_keys[ i ] = rand64();
		queen_castle_keys[ i ] = rand64();
	}
}


uqword zobrist_key( int piece, int square )
{
	int piece_index, color;
	uqword key;

	if( piece == empty )
	{
		key = 0LL;
	}
	else
	{
		piece_index = abs( piece ) - 1;
		color = (1 - (abs( piece ) / piece)) >> 1;
		key = keys[ color ][ piece_index ][ square ];
	}
	return key;
}

uqword zobrist_king_side_castle_key( int color )
{
	assert_range( 0, color, 1 );
	return king_castle_keys[ color ];
}

uqword zobrist_queen_side_castle_key( int color )
{
	assert_range( 0, color, 1 );
	return queen_castle_keys[ color ];
}

uqword zobrist_ep_key( int square )
{
	assert_range( 0, square, 64 );
	return ep_keys[ square & 0x7 ];
}


uqword compute_position_key( board_t* board )
{
	uqword key;
	int i;

	key = 0LL;
	for( i = 0; i < 64; i++ )
	{
		key ^= zobrist_key( board->piece_values[ i ], i );
	}
	if( board->flags.white_king_side == 0 )
		key ^= zobrist_king_side_castle_key( white );
	if( board->flags.white_queen_side == 0 )
		key ^= zobrist_queen_side_castle_key( white );
	if( board->flags.black_king_side == 0 )
		key ^= zobrist_king_side_castle_key( black );
	if( board->flags.black_queen_side == 0 )
		key ^= zobrist_queen_side_castle_key( black );
	if( board->flags.ep_capture_square != 0 )
		key ^= zobrist_ep_key( board->flags.ep_capture_square );
//	key ^= zobrist_repeat_key( board->flags.repeat_count );

	return key;
}

uqword compute_pawn_key( board_t* board )
{
	uqword key;
	int i;

	key = 0LL;
	for( i = 0; i < 64; i++ )
	{
		if( (abs( board->piece_values[ i ] )== pawn) ||  (abs( board->piece_values[ i ] )== king))
			key ^= zobrist_key( board->piece_values[ i ], i );
	}
	if( board->flags.ep_capture_square != 0 )
		key ^= zobrist_ep_key( board->flags.ep_capture_square );

	return key;
}


static uqword rand64( void )
{
	uqword n;
	int n1, n2, n3, n4;
	n1 = rand() & 0xFFFF;
	n2 = rand() & 0xFFFF;
	n3 = rand() & 0xFFFF;
	n4 = rand() & 0xFFFF;
	n = (uqword) n1 | ((uqword) n2 << 16) | ((uqword) n3 << 32) | ((uqword) n4 << 48);
	return n;
}


uqword zobrist_repeat_key( int repeat_count )
{
	uqword q;
	if( repeat_count != 0 )
		q = (uqword) repeat_count;
	else
		q = 0;
	return q;
}

