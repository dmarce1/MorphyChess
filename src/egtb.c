#include "egtb.h"
#include "board.h"
#include "bits.h"

extern bool check_for_draw( board_t* );
extern bool king_in_check( const board_t*, int );

const int flip_lr[ 64 ] = 
{
 	 7, 6, 5, 4, 3,  2,  1,  0,
	15,14,13,12,11, 10,  9,  8,
	23,22,21,20,19, 18, 17, 16,
	31,30,29,28,27, 26, 25, 24, 
	39,38,37,36,35, 34, 33, 32,
	47,46,45,44,43, 42, 41, 40,
	55,54,53,52,51, 50, 49, 48,
	63,62,61,60,59, 58, 57, 56
};


const int flip_ud[ 64 ] = 
{
	56,57,58,59,60,61,62,63,
	48,49,50,51,52,53,54,55,
	40,41,42,43,44,45,46,47,
	32,33,34,35,36,37,38,39,
	24,25,26,27,28,29,30,31,
	16,17,18,19,20,21,22,23,
	 8, 9,10,11,12,13,14,15,
	 0, 1, 2, 3, 4, 5, 6, 7

};

const int transpose[ 64 ] = 
{
	 0, 8,16,24,32,40,48,56,
	 1, 9,17,25,33,41,49,57,
	 2,10,18,26,34,42,50,58,
	 3,11,19,27,35,43,51,59,
	 4,12,20,28,36,44,52,60,
	 5,13,21,29,37,45,53,61,
	 6,14,22,30,38,46,54,62,
	 7,15,23,31,39,47,55,63
};

const int lower_octant_to_index[ 64 ] = 
{
		 0, 1, 2, 3,-1,-1,-1,-1,
		-1, 4, 5, 6,-1,-1,-1,-1,
		-1,-1, 7, 8,-1,-1,-1,-1,
		-1,-1,-1, 9,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1
};

const int index_to_lower_octant[ 10 ] = 
{
		0, 1, 2, 3, 9, 10, 11, 18, 19, 27
};

const bool needs_flip_ud[ 64 ] =
{
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1
};

const bool needs_flip_lr[ 64 ] =
{
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1,
		0,0,0,0,1,1,1,1
};

const bool needs_transpose[ 64 ] =
{
		0,0,0,0,0,0,0,0,
		1,0,0,0,0,0,0,0,
		1,1,0,0,0,0,0,0,
		1,1,1,0,0,0,0,0,
		1,1,1,1,0,0,0,0,
		1,1,1,1,1,0,0,0,
		1,1,1,1,1,1,0,0,
		1,1,1,1,1,1,1,0
};

bool egtb_index_to_board( board_t* board, int third_man, int index, int stm )
{
	int wk;
	int bk;
	int w3;
	int i;
	bool legal;
	bool do_flip_ud, do_flip_lr, do_transpose;

	w3 = index % 62;
	index /= 62;
	bk = index % 63;
	index /= 63;
	wk = index;
	
	if( w3 >= wk )
		w3++;
	if( w3 >= bk )
		w3++;
	if( bk >= wk )
		bk++;
	if( third_man != pawn )
	{
		wk = index_to_lower_octant[ wk ];
	}
	for( i = 0; i < 64; i++ )
		board->piece_values[ i ] = 0;
	for( i = 0; i <= 6; i++ )
	{
		board->piece_boards[ white ][ i ] = 0x0LL;
		board->piece_boards[ black ][ i ] = 0x0LL;
	}
	rotated_bits_zero_out( board->occupied_squares );
	bits_set( &board->piece_boards[ white ][ king ], wk );
	bits_set( &board->piece_boards[ white ][ king ], bk );
	bits_set( &board->piece_boards[ white ][ third_man ], w3 );
	bits_set( &board->piece_boards[ 0 ][ king ], wk );
	bits_set( &board->piece_boards[ 0 ][ king ], bk );
	bits_set( &board->piece_boards[ 0 ][ third_man ], w3 );
	rotated_bits_set( board->occupied_squares, wk );
	rotated_bits_set( board->occupied_squares, bk );
	rotated_bits_set( board->occupied_squares, w3 );
	board->piece_values[ wk ] = +king; 
	board->piece_values[ bk ] = -king;
	board->piece_values[ w3 ] = +third_man;
	legal = (bool) ((board->piece_counts[ white ][ king ] == 1) && (board->piece_counts[ black ][ king ] == 1));
	if( king_in_check( board, board->flags.side_to_move ) )
		board->flags.check = 1;
	else
		board->flags.check = 0;
	if( check_for_draw( board ) )
		board->flags.draw = 1;
	else
		board->flags.draw = 0;
	board->flags.side_to_move = stm;
	board->flags.black_king_side = 
	board->flags.black_queen_side = 
	board->flags.ep_capture_square = 
	board->flags.half_move_count = 
	board->flags.most_recent_capture =
	board->flags.repeat_count = 
	board->flags.white_king_side = 
	board->flags.white_queen_side = 0;
	return legal;
}


int board_3man_egtb_index( board_t* board, int third_man, int wc )
{
	int wk;
	int bk;
	int w3;
	int bc;
	int index;
	bool do_flip_ud, do_flip_lr, do_transpose;
	
	bc = color_flip( wc );
	bk = bits_first_one( board->piece_boards[ bc ][ king ] );
	wk = bits_first_one( board->piece_boards[ wc ][ king ] );
	w3 = bits_first_one( board->piece_boards[ wc ][ third_man ] );
	if( third_man != pawn )
	{
		do_flip_ud = needs_flip_ud[ wk ];
		do_flip_lr = needs_flip_lr[ wk ];
		do_transpose = needs_transpose[ wk ];
		if( do_flip_ud )
		{
			wk = flip_ud[ wk ];
			bk = flip_ud[ bk ];
			w3 = flip_ud[ w3 ];
		}
		if( do_flip_lr )
		{
			wk = flip_ud[ wk ];
			bk = flip_ud[ bk ];
			w3 = flip_ud[ w3 ];
		}
		if( do_transpose )
		{
			wk = flip_ud[ wk ];
			bk = flip_ud[ bk ];
			w3 = flip_ud[ w3 ];
		}
		if( w3 > wk )
			w3--;
		if( w3 > bk )
			w3--;
		if( bk > wk )
			bk--;
		wk = lower_octant_to_index[ wk ];
	}
	else
	{
		do_flip_lr = needs_flip_ud[ wk ];
		if( bc == white )
		{
			wk = flip_ud[ wk ];
			bk = flip_ud[ bk ];
			w3 = flip_ud[ w3 ];
		}
		if( do_flip_ud )
		{
			wk = flip_lr[ wk ];
			bk = flip_lr[ bk ];
			w3 = flip_lr[ w3 ];
		}
		if( w3 > wk )
			w3--;
		if( w3 > bk )
			w3--;
		if( bk > wk )
			bk--;
		wk = lower_octant_to_index[ wk ];
	}
	index = wk * (63*62) + bk * (62) + w3; 
	return index;
}


