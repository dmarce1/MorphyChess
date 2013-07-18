#include "board.h"
#include <stdlib.h>

int material_score[ 7 ] = { 0, 100, 300, 300, 500, 900, 1000 };


int SEE( board_t* board, move_t move, int score )
{
	move_t reply;
	int reply_score;
	int capture;

	capture = abs( board->piece_values[ move.to ] );
	if( board_make_move( board, move ) )
	{
		score += material_score[ capture ];
		if( board_gen_LVA_attack_to_square( board, &reply, move.to ) )
		{
			reply_score = -SEE( board, reply, -score );
			if( reply_score < score )
				score = reply_score;
		}
		board_undo_move( board );
	}
	return score;
}

