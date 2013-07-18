
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
#include <stdlib.h>
#include "search.h"
#include "node.h"
#include "debug.h"

static bool search_test_child( search_t* search, int test_value, int depth );
static void check_kill( search_t* search );
 
void search_create( search_t* search, board_t* board, static_evaluator_t* evaluator, memory_table_t* table )
{
	int i;
	for( i = 0; i < max_moves; i++ )
		search->cutoff_at_move_number[ i ] = 0;
	search->null_verification = false;
	search->board = board;
	search->computer_color = white;
	search->contempt_factor = 0;
	search->evaluator = evaluator;
	search->table = table;
	search->grain_size = search->evaluator->grain_size;
	search->verify_null_move = true;
	search->current_ply = 0;
	search->poll_frequency = DEFAULT_POLL_FREQUENCY;
	search->history = (history_t*) malloc( sizeof( history_t ) );
	if( search->history == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	history_create( search->history, DEFAULT_HISTORY_BRANCH_FACTOR );
	move_set_empty( &search->best_move );
	pthread_mutex_init( &search->kill_mutex, NULL );
	search->kill = false;
	search->thread_number = 0;
	search->kill_time = 0;
	search_reset_counters( search );
}

void search_set_kill_time( search_t* search, int kill_time )
{
	search->kill_time = kill_time;
}

void search_destroy( search_t* search )
{
	pthread_mutex_destroy( &search->kill_mutex );
	history_destroy( search->history );
	free( search->history );
}

void search_reconstruct_principal_variation( search_t* search, board_t* leaf_node, move_t* pv, int depth )
{
	int ply, upper, lower, guess;
	char move_str[ 64 ];
	bool in_table;

	for( ply = 0; depth > 0; depth-- )
	{
		in_table = memory_table_read( search, pv + ply, &lower, &upper, depth, -INFTY );
		if( !in_table )
			break;
		if( !in_table || move_is_empty( pv[ ply ] ) )
		{
			if( (search->kill_time != 0) && (search->kill_time <= time( NULL )) )
				break;
			guess = upper + lower;
			guess /= 2;
			guess /= search->grain_size;
			guess *= search->grain_size;
			search_serial_test_driver( search, pv + ply, depth, guess );
		}
		if( move_is_empty( pv[ ply ] ) )
			break;
		board_move_to_str( move_str, search->board, pv[ ply ] );
		printf( "%8s ", move_str );
		fflush( stdout );
		board_make_move( search->board, pv[ ply ] );
		ply++;
		if( board_king_in_check( search->board ) )
			depth++;
		if( board_result( search->board ) != IN_PROGRESS )
			break;
	}
	move_set_empty( pv + ply );
	if( leaf_node != NULL )
		board_copy( leaf_node, search->board );
	while( ply > 0 )
	{
		board_undo_move( search->board );
		ply--;
	}
}

void search_reset_counters( search_t* search )
{
	search->node_count = 0;
	search->table_hits = 0;
	search->null_hits = 0;
	search->late_move_reductions = 0;
}

int search_serial_test_driver( search_t* search, move_t* move, int depth, int initial_guess )
{
	bool test_result;
	int upper_bound;
	int lower_bound;
	int test_value;
	int epsilon;
	bool timesup;
	
	upper_bound = +INFTY;
	lower_bound = -INFTY;
	test_value = initial_guess;
	epsilon = search->grain_size / 2;

	move_set_empty( &search->best_move );
	timesup = false;
	
	while( (upper_bound > lower_bound) && !timesup )
	{
		test_result = search_test( search, test_value - epsilon, depth );
		if( (search->kill_time != 0) && (search->kill_time <= time( NULL )) )
		{
			move_set_empty( move );
			timesup = true;
		}
		else if( test_result == true )
		{
			*move = search->best_move;
			lower_bound = test_value;
			test_value += search->grain_size;
		}
		else
		{
			test_value -= search->grain_size;
			upper_bound = test_value;
		}
	}
	
	return lower_bound;
}

static bool null_test( search_t* search, int test_value, int depth )
{
	move_t null_move;
	bool test_result;
	int stm, total_pawns;
	if( (depth <= 1) || board_king_in_check( search->board ) || board_position_is_repeat( search->board ) || search->null_verification )
	{
		test_result = false;
	}
	else
	{
		stm = search->board->flags.side_to_move;
		total_pawns = 1 * search->board->piece_counts[ stm ][ pawn ];
		total_pawns += 3 * search->board->piece_counts[ stm ][ knight ];
		total_pawns += 3 * search->board->piece_counts[ stm ][ bishop ];
		total_pawns += 5 * search->board->piece_counts[ stm ][ rook ];
		total_pawns += 9 * search->board->piece_counts[ stm ][ queen ];
		if( total_pawns >= 12 )
		{
			move_set_empty( &null_move );
			board_make_move( search->board, null_move );
			search->current_ply++;
			test_result = !search_test_child( search, -test_value, depth - 1 - NULL_REDUCTION );
			search->current_ply--;
			board_undo_move( search->board );
		}
		else
		{
			test_result = false;
		}
	}
	if( (test_result == true) && (search->verify_null_move == true) )
	{
		search->verify_null_move = false;
		search->null_verification = true;
		test_result = search_test_child( search, test_value, depth - 1 );
		search->verify_null_move = true;
	}
	search->null_verification = false;
	return test_result;
}


static bool extension_needed( search_t* search )
{
	board_t* b;
	bool rc;
	move_t move;
	int moving_man;
	b = search->board;
	move = b->undo_info[ b->flags.half_move_count - 1 ].move;
	moving_man = b->piece_values[ move.to ];
	if( board_king_in_check( b ) )
		rc = true;
	else if( move.promo > pawn )
		rc = true;
	else if( (moving_man == +pawn) && (move.to / 8 == 6) )
		rc = true;
	else if( (moving_man == -pawn) && (move.to / 8 == 1) )
		rc = true;
	else 
		rc = false;
	return rc;
}

static bool search_test_child( search_t* search, int test_value, int depth )
{
	bool test_result;
	bool extension;
	node_t node;
	move_t current_move;
	move_t best_move;
	int branch_count;
	int node_status;
	int lower_bound, upper_bound, draw_score;
	
	draw_score = search->contempt_factor;
	if( search->board->flags.side_to_move != search->computer_color )
		draw_score = -draw_score;
	if( board_is_draw( search->board ) )
	{
		test_result = (bool) (test_value < draw_score);
		search->node_count++;
	}
	else
	{
		extension = extension_needed( search );
		if( extension )
			depth++;
		if( (depth <= 0) || (search->current_ply >= MAX_DEPTH-1) )
		{
			test_result = search_quiescent_test( search, test_value );
		}
		else
		{
			memory_table_read( search, &best_move, &lower_bound, &upper_bound, depth, test_value );
			if( upper_bound < test_value )
			{
				test_result = false;
				search->table_hits++;
				search->node_count++;
			}
			else if( lower_bound > test_value )
			{
				test_result = true;
				search->table_hits++;
				search->node_count++;
			}
			else
			{
				test_result = null_test( search, test_value, depth );
				if( test_result == true )
				{
					search->node_count++;
					search->null_hits++;
				}
				else
				{
					node_create( &node, search, best_move );
					node_status = node_do_next_move( &node, &current_move );
					if( node_status == NODE_MOVES_AVAILABLE )
					{
						branch_count = 0;
						do
						{
							if( extension || !node_ok_to_reduce( &node, depth ) )
							{
								test_result = !search_test_child( search, -test_value, depth - 1 );
							}
							else
							{
								search->late_move_reductions++;
								test_result = !search_test_child( search, -test_value, depth - 2 );
								if( test_result == true )
									test_result = !search_test_child( search, -test_value, depth - 1 );
							}
							node_restore( &node );
							if( test_result == true )
							{
								best_move = current_move;
								break;
							}
							branch_count++;
							node_status = node_do_next_move( &node, &current_move );
						} 
						while( node_status == NODE_MOVES_AVAILABLE );
					}
					else
					{
						assert( node_status == NODE_TERMINAL );
						if( board_king_in_check( search->board ) )
							if( test_value < -static_evaluator_checkmate_value( search->evaluator, search->current_ply ) )
								test_result = true;
							else
								test_result = false;
						else if( test_value > draw_score )
							test_result = false;
						else
							test_result = true;
					}
					history_update( search->history, best_move, test_result, depth, search->current_ply, (search->board->piece_values[ best_move.to ] == 0) ? true : false );
					node_destroy( &node );
					memory_table_write( search, best_move, test_result, test_value, depth );
				}
			}
		}
	}
	check_kill( search );
	return test_result;
}

bool search_test( search_t* search, int test_value, int depth )
{
	bool test_result;
	node_t node;
	move_t current_move;
	int branch_count;
	int node_status;
	int lower_bound, upper_bound;

	assert( depth >= 1 );
	/***************************************/
	pthread_mutex_lock( &search->kill_mutex );
	/***************************************/
	search->kill = false;
	/***************************************/
	pthread_mutex_unlock( &search->kill_mutex );
	/***************************************/
	search->verify_null_move = true;
	search->current_ply = 0;
	move_set_empty( &search->best_move );
	test_result = false;
	if( setjmp( search->jump_buffer ) == 0 )
	{
		memory_table_read( search, &search->best_move, &lower_bound, &upper_bound, depth, test_value );
		if( (upper_bound < test_value) && !move_is_empty( search->best_move ) )
		{
			search->node_count++;
			search->table_hits++;
		}
		else if( (lower_bound > test_value)  && !move_is_empty( search->best_move ) )
		{
			test_result = true;
			search->node_count++;
			search->table_hits++;
		}
		else
		{
			node_create( &node, search, search->best_move );
			node_status = node_do_next_move( &node, &current_move );
			assert_board_consistency( search->board );
			assert( node_status == NODE_MOVES_AVAILABLE );
			branch_count = 0;
			do
			{
				if( !node_ok_to_reduce( &node, depth ) )
				{
					test_result = !search_test_child( search, -test_value, depth - 1 );
				}
				else
				{
					search->late_move_reductions++;
					test_result = !search_test_child( search, -test_value, depth - 2 );
					if( test_result == true )
						test_result = !search_test_child( search, -test_value, depth - 1 );
				}
				node_restore( &node );
				if( test_result == true )
				{
					search->cutoff_at_move_number[ branch_count ]++;
					search->best_move = current_move;
					break;
				}
				branch_count++;
				node_status = node_do_next_move( &node, &current_move );
			} 
			while( node_status == NODE_MOVES_AVAILABLE );
			node_destroy( &node );
			history_update( search->history, search->best_move, test_result, depth, search->current_ply, (search->board->piece_values[ search->best_move.to ] == 0) ? true : false );
			memory_table_write( search, search->best_move, test_result, test_value, depth );
		}
	}
	else
	{
		while( search->current_ply > 0 )
		{
			board_undo_move( search->board );
			search->current_ply--;
		}
	}
	return test_result;
}

static void check_kill( search_t* search )
{
	bool kill_thread;
	if( (search->node_count % search->poll_frequency == 0) && (search->current_ply > 4) )
	{
		/**************************************/
		pthread_mutex_lock( &search->kill_mutex );
		/**************************************/
		if( search->kill )
		{
			kill_thread = true;
			search->kill = false;
		}
		else if( search->kill_time != 0 )
		{
			if( search->kill_time <= time( NULL ) )
				kill_thread = true;
			else 
				kill_thread = false;
		}
		else
		{
			kill_thread = false;
		}
		/**************************************/
		pthread_mutex_unlock( &search->kill_mutex );
		/**************************************/
		if( kill_thread == true )
			longjmp( search->jump_buffer, 1 );
	}
}

void search_kill( search_t* search )
{
	/**************************************/
	pthread_mutex_lock( &search->kill_mutex );
	/**************************************/
	search->kill = true;
	/**************************************/
	pthread_mutex_unlock( &search->kill_mutex );
	/**************************************/
}

void search_set_board( search_t* engine, board_t* board )
{
	engine->board = board;
}


void search_set_contempt_factor( search_t*s , int c )
{
	s->contempt_factor = (c / s->grain_size) * s->grain_size;;
}


void search_set_computer_color( search_t* s, int c )
{
	s->computer_color = c;
}
