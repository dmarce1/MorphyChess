
/*********************************************************************************
MorphyChess 1.0.5 Copyright (C) 2008 Dominic C. Marcello   (dmarcello@phys.lsu.edu) 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see [http://www.gnu.org/licenses/].
**********************************************************************************/
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

