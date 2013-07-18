
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
#include <math.h>
#include <stdlib.h>
#include "board.h"
#include "history.h"
#include "memory.h"
#include "params.h"
#include "debug.h"

static void reset_move_history_table( history_t* );


void history_create( history_t* history, double branch_factor )
{
	int i;
	reset_move_history_table( history );
	history->branch_factor = branch_factor;
	for( i = 0; i < MAX_DEPTH; i++ )
		history->update_increments[ i ] = (int) pow( history->branch_factor, (i - 1) );
}


static void half_move_history_table( history_t* history )
{
	int i, j;
	for( i = 0; i < 64; i++ )
		for( j = 0; j < 64; j++ )
			history->move_history[ i ][ j ] >>= 1;
}


void history_update( history_t* history, move_t move, bool test, int depth, int ply, bool is_non_cap )
{
	int new_score, old_score;
	assert( depth >= 1 );
	if( test && !move_is_empty( move ) && is_non_cap )
	{
		old_score = history->move_history[ move.from ][ move.to ];
		new_score = old_score + history->update_increments[ depth ];
		if( old_score > new_score )
			reset_move_history_table( history );
		else
			history->move_history[ move.from ][ move.to ] = new_score;
		if( !move_compare( move, history->killer1[ ply ]) )
		{
			history->killer2[ ply ] = history->killer1[ ply ];
			history->killer1[ ply ] = move;
		}
	}
}

move_t history_killer1( history_t* history, int ply )
{
	return history->killer1[ ply ];
}

move_t history_killer2( history_t* history, int ply )
{
	return history->killer2[ ply ];
}

int history_move_history( history_t* history, move_t move, int ply )
{
	int score;
	score = history->move_history[ move.from ][ move.to ];
	return score;
}


static void reset_move_history_table( history_t* history )
{
	int i, j;
	for( i = 0; i < 64; i++ )
	{
		for( j= 0; j < 64; j++ )
			history->move_history[ i ][ j ] = 0;
	}
	for( i = 0; i < MAX_DEPTH; i++ )
	{
		move_set_empty( history->killer1 + i );
		move_set_empty( history->killer2 + i );
	}
}

void history_destroy( history_t* h )
{

}
