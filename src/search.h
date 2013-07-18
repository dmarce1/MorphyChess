
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
#ifndef SEARCH_H_
#define SEARCH_H_


#include "board.h"
#include "history.h"
#include "static_evaluator.h"
#include <pthread.h>
#include <setjmp.h>

typedef struct
{
	uqword key;
	sdword score;
	move_t move;
	sbyte depth;
	sbyte null_moves_verified : 4;
	sbyte touched             : 4;
} memory_entry_t;

typedef struct
{
	memory_entry_t* upper_bound_local_store[ 2 ];
	memory_entry_t* lower_bound_local_store[ 2 ];
	memory_entry_t* upper_bound_global_store[ 2 ];
	memory_entry_t* lower_bound_global_store[ 2 ];
	pthread_mutex_t* locks;
	int entry_count;
	int lock_count;
	int thread_count;
	int local_entry_count;
} memory_table_t;


typedef struct
{
	board_t* board;
	static_evaluator_t* evaluator;
	history_t* history;
	memory_table_t* table;
	bool verify_null_move;
	bool kill;
	move_t best_move;
	int node_count;
	int current_ply;
	int grain_size;
	int table_hits;
	jmp_buf jump_buffer;
	pthread_mutex_t kill_mutex;
	int poll_frequency;
	int thread_number;
	int kill_time;
	int null_hits;
	int late_move_reductions;
	int cutoff_at_move_number[ max_moves ];
	int contempt_factor;
	int computer_color;
	bool null_verification;
} search_t;

bool search_quiescent_test( search_t* search, int test_value );
bool search_test( search_t*, int test_value, int depth );
void search_create( search_t*, board_t*, static_evaluator_t*, memory_table_t* );
void search_destroy( search_t* );
void search_reset_counters( search_t* );
int search_serial_test_driver( search_t* search, move_t* move, int depth, int initial_guess );
void search_reconstruct_principal_variation( search_t*, board_t*, move_t*, int );
void search_kill( search_t* );
void search_set_board( search_t*, board_t* );
void search_set_kill_time( search_t*, int );
void memory_table_create( memory_table_t*, int, int, int );
void memory_table_destroy( memory_table_t* );
void memory_table_write( search_t*, move_t, bool, int, int );
bool memory_table_read( search_t*, move_t*, int*, int*, int, int );
void memory_table_reset( memory_table_t* table );
void memory_table_reset_touched_flags( memory_table_t* );
void memory_table_kill_untouched_entries( memory_table_t* );
int search_quiescent_value( search_t* search, int guess );
int search_moves_to_checkmate( search_t* search, int depth );
void search_set_contempt_factor( search_t*, int );
void search_set_computer_color( search_t*, int );


#endif /*SEARCH_H_*/
