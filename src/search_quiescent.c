
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
#include "search.h"
#include "node.h"
#include "board.h"

static void set_quiescent_flag( search_t* search, bool* first_quiescent_node );
static void reset_quiescent_flag( search_t* search, bool first_quiescent_node );


int search_quiescent_value( search_t* search, int guess )
{
	int score;
	if( search->board->flags.side_to_move == black )
		guess = -guess;
	guess /= search->grain_size;
	guess *= search->grain_size;
	if( board_king_in_checkmate( search->board, white) )
		score = +static_evaluator_checkmate_value( search->evaluator, search->current_ply );
	else if( board_king_in_checkmate( search->board, black) )
		score = -static_evaluator_checkmate_value( search->evaluator, search->current_ply );
	else if( board_king_in_stalemate( search->board, white ) || board_king_in_stalemate( search->board, black ) || board_is_draw( search->board ) )
		score = 0;
	else
	{
		score = guess - search->grain_size / 2;
		if( search_quiescent_test( search, score ) )
		{
			score += search->grain_size;
			while( search_quiescent_test( search, score ) && (score < +INFTY) )
				score += search->grain_size;
			score -= search->grain_size;
		}
		else
		{
			score -= search->grain_size;
			while( !search_quiescent_test( search, score ) && (score > -INFTY) )
				score -= search->grain_size;
		}
		score = score + search->grain_size / 2;
	}
	if( search->board->flags.side_to_move == black )
		score = -score;
	return score;
}

bool search_quiescent_test( search_t* search, int test_value )
{
	bool test_result;
	node_t node;
	move_t current_move;

	move_set_empty( &current_move );
	node_create( &node, search, current_move );

	test_result = static_evaluator_test( search->evaluator, search->board, test_value );
	if( (test_result == false) && (search->current_ply < MAX_DEPTH) )
	{
		while( node_do_next_nonquiescent_move( &node, &current_move ) == NODE_MOVES_AVAILABLE )
		{
			test_result = !search_quiescent_test( search, -test_value );
			node_restore( &node );
			if( test_result == true )
				break;
		}
	}

	node_destroy( &node );
	return test_result;
}
