
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "see.h"
#include "history.h"
#include "node.h"
#include "integers.h"
#include "memory.h"
#include "params.h"

static int capture_move_value( board_t*, move_t );



int node_total_moves( node_t* node )
{
	return node->total_moves_generated;
}

void node_create( node_t* node, search_t* search, move_t move )
{
	node->first_noncap = false;
	node->mg_stage = START;
	node->search = search;
	node->total_moves_generated = 0;
	node->search->node_count++;
	node->table_move = move;
	node->moves_in_queue = 0;
	move_set_empty( &node->killer );
}


int node_do_next_move( node_t* node, move_t* branch )
{
	move_t move;
	int i, rc, best_score, score, best_i, captures_left;

	assert( node->total_moves_generated < max_moves );
	best_i = 0;
	rc = NODE_MOVES_AVAILABLE;
	if( node->mg_stage == NONCAPTURES )
		node->first_noncap = false;
TRY_AGAIN:
	switch( node->mg_stage )
	{
	case START:
        if( !move_is_empty( node->table_move) )
        {
        	move = node->table_move;
        	node->mg_stage = TABLE;
        	break;
        }
	case TABLE:
		node->mg_stage = PROMOTIONS;
		node->moves_in_queue = board_gen_pseudolegal_promotions( node->search->board, node->list );
	case PROMOTIONS:
		if( node->moves_in_queue > 0 )
		{
			node->moves_in_queue--;
			move = node->list[ node->moves_in_queue ];
			break;
		}
		node->moves_in_queue = board_gen_pseudolegal_captures( node->search->board, node->list );
		node->mg_stage = CAPTURES;
		for( i = 0; i < node->moves_in_queue; i++ )
			node->scores[ i ] = capture_move_value( node->search->board, node->list[ i ] );
	case CAPTURES:
		if( node->moves_in_queue > 0 )
		{
			best_score = -INFTY;
			for( i = 0; i < node->moves_in_queue; i++ )
			{
				score = node->scores[ i ];
				assert( score > -INFTY );
				if( score > best_score )
				{
					best_score = score;
					best_i = i;
				}
			}
			if( best_score >= 0 )
			{
				node->moves_in_queue--;
				move = node->list[ best_i ];
				node->list[ best_i ] = node->list[ node->moves_in_queue ];
				node->scores[ best_i ] = node->scores[ node->moves_in_queue ];
				break;
			}
		}
		node->mg_stage = LOSING_CAPTURES;
		move = history_killer1( node->search->history, node->search->current_ply );
		if( (move.promo == 0) )
			if( !move_compare( move, node->table_move ) )
				if( !board_move_is_capture( node->search->board, move ) )
					if( board_is_pseudo_legal_move( node->search->board, move ) )
					{
						node->killer = move;
						break;
					}
		move = history_killer2( node->search->history, node->search->current_ply );
		if( (move.promo == 0) )
			if( !move_compare( move, node->table_move ) )
				if( !board_move_is_capture( node->search->board, move ) )
					if( board_is_pseudo_legal_move( node->search->board, move ) )
					{
						node->killer = move;
						break;
					}
	case LOSING_CAPTURES:
		if( node->moves_in_queue > 0 )
		{
			best_score = -INFTY;
			for( i = 0; i < node->moves_in_queue; i++ )
			{
				score = node->scores[ i ];
				assert( score > -INFTY );
				if( score > best_score )
				{
					best_score = score;
					best_i = i;
				}
			}
			node->moves_in_queue--;
			move = node->list[ best_i ];
			node->list[ best_i ] = node->list[ node->moves_in_queue ];
			node->scores[ best_i ] = node->scores[ node->moves_in_queue ];
			break;
		}
		node->moves_in_queue = board_gen_pseudolegal_noncaptures( node->search->board, node->list );
		node->mg_stage = NONCAPTURES;
		node->first_noncap = true;
	case NONCAPTURES:
		if( node->moves_in_queue > 0 )
		{
			best_score = -INFTY;
			for( i = 0; i < node->moves_in_queue; i++ )
			{
				score = history_move_history( node->search->history, node->list[ i ], node->search->current_ply );
				assert( score > -INFTY );
				if( score > best_score )
				{
					if( !move_compare( node->killer, node->list[ i ] ) )
					{
						best_score = score;
						best_i = i;
					}
				}
			}
			if( best_score > -INFTY )
			{
				node->moves_in_queue--;
				move = node->list[ best_i ];
				node->list[ best_i ] = node->list[ node->moves_in_queue ];
				break;
			}
		}
		else
		{
			node->mg_stage = END;
		}
	case END:
		move_set_empty( &move );
		rc = NODE_NO_MORE_MOVES;
		break;
	}
	*branch = move;
	if( rc == NODE_MOVES_AVAILABLE )
	{
		if( (move_compare( move, node->table_move )) && (node->mg_stage != TABLE) )
		{
			goto TRY_AGAIN;
		}
		else if( !board_make_move( node->search->board, move ) )
		{
			goto TRY_AGAIN;
		}
		else
		{
			node->search->current_ply++;
			node->total_moves_generated++;
		}
	}
	else if( node->total_moves_generated == 0 )
	{
		rc = NODE_TERMINAL;
	}
    return rc;
}


int node_do_next_nonquiescent_move( node_t* node, move_t* branch )
{
	move_t move;
	int i, rc, best_score, score, best_i;

	assert( node->total_moves_generated < max_moves );
	best_i = 0;
	rc = NODE_MOVES_AVAILABLE;
TRY_AGAIN:
	switch( node->mg_stage )
	{
	case START:
		node->moves_in_queue = board_gen_pseudolegal_promotions( node->search->board, node->list );
		node->mg_stage = PROMOTIONS;
	case PROMOTIONS:
		if( node->moves_in_queue > 0 )
		{
			node->moves_in_queue--;
			move = node->list[ node->moves_in_queue ];
			break;
		}
		node->moves_in_queue = board_gen_pseudolegal_captures( node->search->board, node->list );
		node->mg_stage = CAPTURES;
		for( i = 0; i < node->moves_in_queue; i++ )
			node->scores[ i ] = capture_move_value( node->search->board, node->list[ i ] );
	case CAPTURES:
		if( node->moves_in_queue > 0 )
		{
			best_score = -INFTY;
			for( i = 0; i < node->moves_in_queue; i++ )
			{
				score = node->scores[ i ];
				assert( score > -INFTY );
				if( score > best_score )
				{
					best_score = score;
					best_i = i;
				}
			}
			node->moves_in_queue--;
			move = node->list[ best_i ];
			node->list[ best_i ] = node->list[ node->moves_in_queue ];
			node->scores[ best_i ] = node->scores[ node->moves_in_queue ];
			break;
		}
		node->mg_stage = END;
	case END:
		move_set_empty( &move );
		rc = NODE_NO_MORE_MOVES;
		break;
	}
	*branch = move;
	if( rc == NODE_MOVES_AVAILABLE )
	{
		if( board_make_move( node->search->board, move ) )
		{
			node->search->current_ply++;
			node->total_moves_generated++;
		}
		else
		{
			goto TRY_AGAIN;
		}
	}
    return rc;
}



void node_restore( node_t* node )
{
	node->search->current_ply--;
	board_undo_move( node->search->board );
}


static int capture_move_value( board_t* board, move_t move )
{
	static const int pvals[ 7 ] = { 0, 100, 300, 300, 500, 900, INFTY };
	int target_piece;
	int capturing_piece;
	int value, test_value;
	target_piece = abs( board->piece_values[ move.to ] );
	capturing_piece = abs( board->piece_values[ move.from ] );
	value = pvals[ target_piece ];
	if( board_square_under_attack_by( board, move.to, color_flip( board->flags.side_to_move ) ) )
		value -= pvals[ capturing_piece ];
	else 
		value -= pvals[ capturing_piece ] / 10;
	return value;
}


void node_destroy( node_t* n )
{

}

bool node_no_moves_generated( node_t* n )
{
	return (bool) (n->total_moves_generated == 0);
}

bool node_ok_to_reduce( node_t* node, int depth )
{
	bool rc;
#ifdef LATE_MOVE_REDUCTION_OFF 
	return false;
#endif
	if( node->mg_stage != NONCAPTURES )
		rc = false;
	else if( node->first_noncap )
		rc = false;
	else if( depth < REDUCTION_LIMIT )
		rc = false;
	else if( node->total_moves_generated <= FULL_DEPTH_MOVES )
		rc = false;
	else if( board_king_in_check( node->search->board ) )
		rc = false;
	else
		rc = true;
	return rc;
}


bool node_move_is_futile( node_t* node, int material_balance, int test_value )
{
	bool rc;
	move_t move;
	int move_material_gain;
	int last_cap;
	board_t* b = node->search->board;
	static const int pvals[ 7 ] = { 0, 100, 325, 325, 500, 900, INFTY };

	
	if( board_king_in_check( node->search->board ) )
	{
		rc = false;
	}
	else
	{
		move = b->undo_info[ b->flags.half_move_count - 1 ].move;
		last_cap = b->flags.most_recent_capture;
		move_material_gain = pvals[ last_cap ];
		if( move.promo > pawn )
			move_material_gain += pvals[ move.promo ] - pvals[ pawn ];
		if( material_balance + FUTILITY_MARGIN + move_material_gain >= test_value )
			rc = false;
		else 
			rc = true;
	}

	return rc;
}


