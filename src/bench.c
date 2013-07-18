
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

#include "math.h"
#include "debug.h"
#include "parallel_search.h"
#include "search.h"


static const char* benchmark_positions[ NUM_BENCHMARKS ] = 
{ 
	initial_position,
	"8/1b3Rk1/p2rB1pp/8/p2p4/8/1PP3PP/6K1 b - - 0 33",
	"4r2k/1q4p1/pp4p1/2b1B1Nb/PPQ5/8/2P3PP/5R1K b - b3 0 32",
	"r1bq1rk1/pp2bppp/2nppn2/2p5/4P3/2NP1NP1/PPP2PBP/R1BQ1RK1 w - - 0 8",
	"rnbqkb1r/p1p2ppp/1p2p3/4P3/3Pp3/4B3/PPP1NPPP/R2QKB1R b KQkq - 0 7",
	"r1b1kb1r/pppnqppp/3p1n2/6B1/8/3P1N2/PPP1QPPP/RN2KB1R w KQkq - 0 8",
	"r3k2r/4qppp/p3p3/1p2n3/3QN3/8/PPP3PP/2KR3R b kq - 0 17",
	"8/5k2/p1r1pp2/6pp/6r1/2N3P1/RPP1K2P/2N5 w - h6 0 32",
	"r3rbk1/3n1pp1/p3pn1p/3b4/1PBN4/P4N2/1B3PPP/3R1RK1 w - - 0 25",
	"3k4/8/3q4/8/8/4KB2/8/8 b - - 0 32"

};

#ifdef __ICC
#pragma optimization_level 1
#endif
int interface_run_benchmark( void )
{
	board_t board;
	move_t best_move;
	parallel_search_t search;
	static_evaluator_t eval;
	int time_start;
	int node_count;
	int table_hits;
	int null_hits;
	int late_move_reductions;
	double time_taken;
	double branch_factor;
	int i,j;
	int move_counts[ max_moves ];
	memory_table_t memory_table;

	for( i = 0; i < max_moves; i++ )
		move_counts[ i ] = 0;
	static_evaluator_create( &eval, default_evaluator_parameters );
	memory_table_create( &memory_table, TABLE_SIZE, TABLE_LOCKS_PER_THREAD*THREAD_COUNT, THREAD_COUNT  );
	printf( "Starting benchmarks...\n" );
	time_start = time( NULL );
	board_create( &board );
	debug_register_board( &board );
	while( time( NULL ) == time_start )
		;
	node_count = 0;
	null_hits = 0;
	table_hits = 0;
	late_move_reductions = 0;
	time_start++;
	parallel_search_create( &search, &board, &eval, &memory_table, THREAD_COUNT );
	parallel_search_set_posting( &search, true );
	parallel_search_set_learning( &search, false );
	parallel_search_set_book( &search, false );
	branch_factor = 0.0;
	for( i = 0; i < NUM_BENCHMARKS; i++ )
	{	
		printf( "--------------------------------------------------------------------------------\n" );
		printf( "\nPosition #%i of %i\n", i + 1, NUM_BENCHMARKS  );
		board_load_FEN( &board, benchmark_positions[ i ] );
		board_to_stream( &board, stdout );
		parallel_search_reset_memory( &search );
		parallel_search_set_board( &search, &board );
		parallel_search_set_kill_time( &search, (BENCH_TIME/NUM_BENCHMARKS) + time(NULL) );
		parallel_search_set_use_max_time( &search, true );
		parallel_search_test_driver( &search, &best_move, MAX_DEPTH, 0 );
		time_taken = (double) (time( NULL ) - time_start);
		node_count += search.node_count;
		table_hits += search.table_hits;
		null_hits += search.null_hits;
		late_move_reductions += search.late_move_reductions;
		if( time_taken == 0 )
			time_taken = 1;
		printf( "time             = %i s\n", (int)time_taken );
		printf( "Total KNodes     = %i\n", node_count/1024 );
		printf( "Total KNodes/S   = %i\n", node_count/1024/(int)time_taken );
		printf( "Total KHits      = %i\n", table_hits/1024 );
		printf( "Total KNulls     = %i\n", null_hits/1024 );
		printf( "Total KReductions= %i\n", late_move_reductions/1024 );
		branch_factor = (i * branch_factor + pow( search.node_count,  1.0 / search.max_depth_complete )) / (double) (i+1);
		printf( "Branch Factor    = %f\n", branch_factor );
		for( j = 0; j < max_moves; j++ )
			move_counts[ j ] += search.cutoff_at_move_number[ j ];
	}
	memory_table_destroy( &memory_table );
	static_evaluator_destroy( &eval );
//	for( i = 0; i < 100; i++ )
//		printf( "%i %i\n", i, move_counts[ i ] );

	return 0;
}



