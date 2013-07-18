#include "parallel_search.h"
#include <stdlib.h>
#include "debug.h"
#include <sched.h>

static void* parallel_thread_handler( void* );
static void set_in_use( parallel_search_t* engine, bool in_use );

void parallel_search_reset_memory( parallel_search_t* engine )
{
	memory_table_reset( engine->searches[ 0 ].table );
	static_evaluator_reset_pawn_table( engine->evaluator );
}

void parallel_search_set_kill_time( parallel_search_t* engine, int kill_time )
{
	engine->kill_time = kill_time;
}

static void* detached_search_driver( void* data )
{
	int rc, restart;
	parallel_search_t* engine;
	engine = (parallel_search_t*) data;
	pthread_mutex_lock( engine->main_lock );
RESTART:
	if( engine->learning_on == true )
	{
		engine->leaf_board_ptr = (board_t*) malloc( sizeof( board_t ) );
		if( engine->leaf_board_ptr == NULL )
		{
			printf( "Allocation error\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
	}
	parallel_search_test_driver( engine, &engine->root_best_move, engine->max_depth, engine->initial_guess );
	if( (engine->learning_on == true) && (engine->action_function != NULL) )
	{
		rc = fwrite( engine->leaf_board_ptr, sizeof( board_t ), 1, engine->learning_file );
		if( rc != 1 )
		{
			printf( "Error writing learning file\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
	}
	if( engine->learning_on == true )
		free( engine->leaf_board_ptr );
	/*********************************/
	pthread_mutex_lock( engine->lock );
	/*********************************/
	if( (engine->action_function != NULL) && (board_result( engine->board ) == IN_PROGRESS) )
		restart = engine->action_function( engine->board, &engine->root_best_move );
	else 
		restart = false;
	/**********************************/
	pthread_mutex_unlock( engine->lock );
	/**********************************/
	if( restart && (board_result( engine->board ) == IN_PROGRESS) )
	{
		engine->kill_time = time( NULL ) + 1000000;
		engine->action_function = NULL;
		goto RESTART;
	}
	engine->detached = false;
	pthread_mutex_unlock( engine->main_lock );
	pthread_exit( NULL );
	return NULL;
}

void parallel_search_detached_test_driver( parallel_search_t* engine, bool (*action_function)( board_t*, move_t*), int max_time )
{
	int rc;
	engine->action_function = action_function;
	engine->initial_guess = 0;
	engine->max_depth = MAX_DEPTH;
	engine->kill_time = time( NULL ) + max_time;
	engine->detached = true;
	if( (rc = pthread_create( &engine->thread_id, NULL, detached_search_driver, engine )) != 0 )
	{
		printf( "pthread_create returned %i\n", rc );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	if( (rc = pthread_detach( engine->thread_id )) != 0 )
	{
		printf( "pthread_create returned %i\n", rc );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
}

void parallel_search_set_posting( parallel_search_t* engine, bool posting )
{
	engine->posting = posting;
}

void parallel_search_set_book( parallel_search_t* engine, bool b )
{
	engine->use_book = b;
}


bool timeb_compare( struct timeb* t1, struct timeb* t2 )
{
	return (t1->time == t2->time) && (t1->millitm == t2->millitm);
}

int timeb_difference( struct timeb* t1, struct timeb* t2 )
{
	return (1000 * (t1->time - t2->time) + t1->millitm - t2->millitm) / 10;
}

int parallel_search_test_driver( parallel_search_t* engine, move_t* move, int depth, int initial_guess )
{
	int i, rc, j;
	struct sched_param sched;
	move_t book_move;
	if( !engine->detached )
		pthread_mutex_lock( engine->main_lock );
	ftime( &engine->start_time );
	engine->max_time = engine->kill_time - time(NULL);
//	printf( "*******************%i\n", engine->max_time );
	set_in_use( engine, true );
	memory_table_reset_touched_flags( engine->searches[ 0 ].table );
	engine->max_depth = depth; 
	engine->initial_guess = initial_guess;
	engine->table_hits = 0;
	engine->node_count = 0;
	engine->late_move_reductions = 0;
	engine->null_hits = 0;
TRY_AGAIN:
    engine->max_depth_complete = 0;
	for( i = 0; i < MAX_DEPTH; i++ )
	{
		engine->upper_bound[ i ] = +INFTY;
		engine->lower_bound[ i ] = -INFTY;
		engine->search_status[ i ] = WAITING;
	}
	for( i = 0; i < engine->thread_count; i++ )
		engine->depths[ i ] = -1;
	engine->search_done = false;
	engine->lower_bound[ 0 ] = initial_guess - engine->grain_size / 2;
	engine->upper_bound[ 0 ] = initial_guess + engine->grain_size / 2;
	move_set_empty( &book_move );
	if( engine->use_book )
	{
		book_move = book_find_move( &engine->book, engine->board );
	}
	if( move_is_empty( book_move ) )
	{
		for( i = 0; i < engine->thread_count; i++ )
		{
			while( (rc = pthread_create( engine->threads + i, NULL, parallel_thread_handler, (void*) engine )) != 0 )
			{
				printf( "pthread_create returned %i.\n", rc );
				printf( "Line %i File %s\n", __LINE__, __FILE__ );
				abort();
			}
		}
		for( i = 0; i < engine->thread_count; i++ )
		{
			pthread_join( engine->threads[ i ], NULL );
			engine->threads_in_use--;
			search_reset_counters( engine->searches + i );
		}
	}
	else
	{
		for( i = 0; i < MAX_DEPTH; i++ )
		{
			engine->best_move[ i ] = book_move;
			engine->search_status[ i ] = COMPLETE;
			engine->lower_bound[ i ] = engine->upper_bound[ i ] = 0;
		}
		engine->root_best_move = book_move;
	}
	ftime( &engine->end_time );
	for( i = 0; i < max_moves; i++ )
	{
		engine->cutoff_at_move_number[ i ] = 0;
		for( j = 0; j < engine->thread_count; j++ )
			engine->cutoff_at_move_number[ i ] += engine->searches[ j ].cutoff_at_move_number[ i ];
	}
	if( timeb_compare( &engine->start_time, &engine->end_time ) )
		engine->end_time.millitm = engine->start_time.millitm + 1;
	printf( 
			"speed = %i kn/s\n",
			100*(engine->node_count/1024) /
			timeb_difference( &engine->end_time, &engine->start_time)
	);
//	for( j = 0; j < 35; j++ )
//		printf( "%i %i\n", j, engine->cutoff_at_move_number[ j ] );
	for( i = MAX_DEPTH - 1; i >= 0; i-- )
	{
		if( i == 0 )
		{
			goto TRY_AGAIN;
		}
		if( engine->search_status[ i ] == COMPLETE )
		{
			engine->root_best_move = engine->best_move[ i ];
			engine->score = engine->lower_bound[ i ] + engine->grain_size / 2;
			break;
		}
	}
	memory_table_kill_untouched_entries( engine->searches[ 0 ].table  );
	set_in_use( engine, false );
	if( !engine->detached )
		pthread_mutex_unlock( engine->main_lock );
	return 0;
}

static void show_principal_variation( parallel_search_t* engine, int depth, int thread_num, FILE* fp )
{
	move_t move_list[ MAX_DEPTH ];
	int time_elapsed, score, rc;
	struct timeb this_time;
	if( engine->posting )
	{
		ftime( &this_time );
		time_elapsed = timeb_difference( &this_time, &engine->start_time );
		score = engine->lower_bound[ depth ] + engine->grain_size / 2;
		fprintf( fp, "%2i %4i %5i %8i ", depth, score, time_elapsed, engine->node_count );
		fflush( stdout );
		engine->searches[ thread_num ].best_move = engine->best_move[ depth ];
//		search_set_kill_time( engine->searches + thread_num, engine->kill_time );
//		printf( "/****************%i %i******************/\n", engine->searches[ thread_num].kill_time, time( NULL ) );
		search_reconstruct_principal_variation( engine->searches + thread_num, engine->leaf_board_ptr, move_list, depth );
		engine->node_count += engine->searches[ thread_num ].node_count;
		engine->table_hits += engine->searches[ thread_num ].table_hits;
		engine->late_move_reductions += engine->searches[ thread_num ].late_move_reductions;
		engine->null_hits += engine->searches[ thread_num ].null_hits;
		search_reset_counters( engine->searches + thread_num );
		fprintf( fp, "\n" );
	}
}

bool parallel_search_in_use( parallel_search_t* engine )
{
	bool in_use;
	/********************************/
	pthread_mutex_lock( engine->lock );
	/********************************/
	in_use = engine->in_use;
	/**********************************/
	pthread_mutex_unlock( engine->lock );
	/**********************************/
	return in_use;
}


static void set_in_use( parallel_search_t* engine, bool in_use )
{
	/********************************/
	pthread_mutex_lock( engine->lock );
	/********************************/
	engine->in_use = in_use;
	/**********************************/
	pthread_mutex_unlock( engine->lock );
	/**********************************/
}


void parallel_search_set_board( parallel_search_t* engine, board_t* board )
{
	int i;
	board_t* board_copy_ptr;
	memory_table_t* table_ptr;
	engine->board = board;
	table_ptr = engine->searches[ 0 ].table;
	for( i = 0; i < engine->thread_count; i++ )
	{
		board_destroy( engine->searches[ i ].board );
		free( engine->searches[ i ].board );
		board_copy_ptr = (board_t*) malloc( sizeof( board_t ) );
		if( board_copy_ptr == NULL )
		{
			printf( "Error allocating history table\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
		board_create( board_copy_ptr );
		board_copy( board_copy_ptr, board );
		search_destroy( engine->searches + i );
		search_create( engine->searches + i, board_copy_ptr, engine->evaluator, table_ptr );
	}
}


void parallel_search_create( parallel_search_t* engine, board_t* board, static_evaluator_t* evaluator, memory_table_t* table, int thread_count )
{
	int i;
	board_t* board_copy_ptr;
	FILE* fp;
	engine->leaf_board_ptr = NULL;
	engine->board = board;
	engine->thread_count = thread_count;
	engine->late_move_reductions = 0;
	engine->null_hits = 0;
	engine->table_hits = 0;
	book_init( &engine->book );
	book_open( &engine->book );
	engine->use_book = true;
	engine->node_count = 0;
	engine->threads_in_use = 0;
	engine->learning_on = false;
	engine->kill_time = 0;
	engine->contempt_factor = 0;
	engine->computer_color = white;
	engine->detached = false;
	engine->posting = true;
	engine->grain_size = DEFAULT_GRAIN_SIZE;
	engine->depths = (int*) malloc( sizeof( int ) * thread_count );
	if( engine->depths == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	engine->test_values = (int*) malloc( sizeof( int ) * thread_count );
	if( engine->test_values == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	engine->searches = (search_t*) malloc( sizeof( search_t ) * thread_count );
	if( engine->searches == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	engine->threads = (pthread_t*) malloc( sizeof( pthread_t ) * thread_count );
	if( engine->threads == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	engine->lock = (pthread_mutex_t*) malloc( sizeof( pthread_mutex_t ) );
	if( engine->lock == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	engine->main_lock = (pthread_mutex_t*) malloc( sizeof( pthread_mutex_t ) );
	if( engine->main_lock == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	engine->evaluator = evaluator; 
	for( i = 0; i < MAX_DEPTH; i++ )
	{
		engine->upper_bound[ i ] = +INFTY;
		engine->lower_bound[ i ] = -INFTY;
		engine->search_status[ i ] = WAITING;
	}
	for( i = 0; i < thread_count; i++ )
	{
		board_copy_ptr = (board_t*) malloc( sizeof( board_t ) );
		if( board_copy_ptr == NULL )
		{
			printf( "Allocation error\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
		board_create( board_copy_ptr );
		board_copy( board_copy_ptr, board );
		search_create( engine->searches + i, board_copy_ptr, evaluator, table );
		engine->depths[ i ] = -1;
	}
	pthread_mutex_init( engine->lock, NULL );
	pthread_mutex_init( engine->main_lock, NULL );
	engine->in_use = false;
	//	fp = fopen( "brain.dat", "rb" );
	fp = NULL;
	if( fp != NULL )
	{
		i = fread( engine->eval_params, sizeof( evaluator_parameters_t ), 1, fp );
		if( i != 1 )
		{
			printf( "Error reading brain\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
		i = fclose( fp );
		if( i != 0 )
		{
			printf( "Error closing brain after reading\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
		static_evaluator_destroy( engine->evaluator );
		static_evaluator_create( engine->evaluator, engine->eval_params );
	}
	else
	{
		for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		engine->eval_params[ i ] = default_evaluator_parameters[ i ];
	}
}

void parallel_search_destroy( parallel_search_t* engine )
{
	int i;
	for( i = 0; i < engine->thread_count; i++ )
	{
		board_destroy( engine->searches[ i ].board );
		free( engine->searches[ i ].board );
		search_destroy( engine->searches + i );
	}
	book_free( &engine->book );
	free( engine->searches );
	free( engine->threads );
	free( engine->lock );
	free( engine->main_lock );
	free( engine->test_values );
	free( engine->depths );
}


bool time_to_terminate( parallel_search_t* engine, int this_thread )
{
	struct timeb this_time;
	int elapsed;
	int max_time;
	int score;
	int test_score;
	bool rc;
	rc = false;
	if( board_next_move_is_forced( engine->board ) )
		return true;
	if( engine->use_max_time )
		return false;
	if( engine->max_depth_complete >= MIN_DEPTH )
	{
		ftime( &this_time );
		elapsed = timeb_difference( &this_time, &engine->start_time );
		max_time = 100 * engine->max_time;
		score = engine->lower_bound[ engine->max_depth_complete ];
//		printf( "%i %i\n", elapsed, max_time );
		if( elapsed > max_time / 2 )
		{
			test_score = score - TIME_CUT4;
			test_score /= engine->grain_size;
			test_score *= engine->grain_size;
	//		printf( "Testing termination after cutoff 4\n" );
//			printf( "%i\n", test_score );
			if( !static_evaluator_test( engine->searches[ this_thread ].evaluator, engine->searches[ this_thread ].board, test_score - engine->grain_size / 2 ) )
			{
				rc = true;
			//	printf( "Terminating after cutoff 4\n" );
			}
		}
		if( elapsed > max_time / 4 )
		{
			test_score = score - TIME_CUT3;
			test_score /= engine->grain_size;
			test_score *= engine->grain_size;
		//	printf( "Testing termination after cutoff 3\n" );
	//		printf( "%i\n", test_score );
			if( !static_evaluator_test( engine->searches[ this_thread ].evaluator, engine->searches[ this_thread ].board,   test_score - engine->grain_size / 2 ) )
			{
				rc = true;
//				printf( "Terminating after cutoff 3\n" );
			}
		}
		else if( elapsed > max_time / 8 )
		{
			test_score = score - TIME_CUT2;
			test_score /= engine->grain_size;
			test_score *= engine->grain_size;
		//	printf( "Testing termination after cutoff 2\n" );
	//		printf( "%i\n", test_score );
			if( !static_evaluator_test( engine->searches[ this_thread ].evaluator,  engine->searches[ this_thread ].board,  test_score - engine->grain_size / 2 ) )
			{
				rc = true;
//				printf( "Terminating after cutoff 2\n" );
			}
		}
		else if( elapsed > max_time / 16 )
		{
		//	printf( "Testing termination after cutoff 1\n" );
			test_score = score - TIME_CUT1;
			test_score /= engine->grain_size;
			test_score *= engine->grain_size;
	//		printf( "%i %i\n", score, test_score );
			if( !static_evaluator_test( engine->searches[ this_thread ].evaluator, engine->searches[ this_thread ].board,  test_score - engine->grain_size / 2 ) )
			{
				rc = true;
//				printf( "Terminating after cutoff 1\n" );
			}
		}
	}
	return rc;
}


void brake_if_needed( parallel_search_t* engine )
{
#ifndef NOSPEEDLIMIT
	int speed;
	int time_elapsed;
	struct timeb this_time;
	do
	{
		ftime( &this_time );
		time_elapsed = timeb_difference( &this_time, &engine->start_time );
		if( time_elapsed < 1 )
			time_elapsed = 1;
		speed = 100 * engine->node_count / 1024 / time_elapsed;
	} 
	while( speed > SPEED_LIMIT );
#endif	
}

static bool determine_next_search( parallel_search_t* engine, int* test_value_ptr, int* depth_ptr, int this_thread_num )
{
	int depth, test_value, guess_value, thread_num, hi_search, lo_search, score, i;
	bool found_next_search, rc;
	found_next_search = false;
	rc = true;
	if( engine->search_done )
		return false;
	for( depth = 1; depth < MAX_DEPTH; depth++ )
	{
		switch( engine->search_status[ depth ] )
		{
		case WAITING:
			guess_value = (engine->lower_bound[ depth-1 ] + engine->upper_bound[ depth-1 ]) / 2;
			guess_value /= engine->grain_size;
			guess_value *= engine->grain_size;
			guess_value -= engine->grain_size / 2;
			engine->search_status[ depth ] = SEARCHING;
			*depth_ptr = depth;
			*test_value_ptr = guess_value;
			found_next_search = true;
			break;
		case SEARCHING:
			hi_search = engine->lower_bound[ depth ];
			lo_search = engine->upper_bound[ depth ];
			for( thread_num = 0; thread_num < engine->threads_in_use; thread_num++ )
			{
				if( thread_num == this_thread_num )
					continue;
				if( engine->depths[ thread_num ] == depth )
				{
					if( engine->test_values[ thread_num ] > hi_search )
						hi_search = engine->test_values[ thread_num ];
					if( engine->test_values[ thread_num ] < lo_search )
						lo_search = engine->test_values[ thread_num ];
				}
			}
			hi_search += engine->grain_size;
			lo_search -= engine->grain_size;
			if( (hi_search < engine->upper_bound[ depth ]) && (lo_search > engine->lower_bound[ depth ]) )
			{
				guess_value = (engine->lower_bound[ depth-1 ] + engine->upper_bound[ depth-1 ]) / 2;
				guess_value /= engine->grain_size;
				guess_value *= engine->grain_size;
				guess_value -= engine->grain_size / 2;
				if( abs( hi_search - guess_value ) <= abs( lo_search - guess_value ) )
				{
					*depth_ptr = depth;
					*test_value_ptr = hi_search;
					found_next_search = true;
				}
				else
				{
					*depth_ptr = depth;
					*test_value_ptr = lo_search;
					found_next_search = true;
				}
			}
			else if( hi_search < engine->upper_bound[ depth ] )
			{
				*depth_ptr = depth;
				*test_value_ptr = hi_search;
				found_next_search = true;
			}
			else if( lo_search > engine->lower_bound[ depth ]  )
			{
				*depth_ptr = depth;
				*test_value_ptr = lo_search;
				found_next_search = true;
			}
			else
			{
				engine->search_status[ depth ] = FINAL_SEARCHING;
			}
		case FINAL_SEARCHING:
			if( !found_next_search )
			{
				if( engine->upper_bound[ depth ] - engine->lower_bound[ depth ] <= engine->grain_size )
				{
					engine->search_status[ depth ] = COMPLETE;
					show_principal_variation( engine, depth, this_thread_num, stdout );
					if( (engine->max_depth <= depth) || abs( engine->lower_bound[ depth ] ) >= MIN_CHECKMATE_VALUE )
						rc = false;
					if( engine->max_depth_complete < depth )
						engine->max_depth_complete = depth;
					if( time_to_terminate( engine, this_thread_num ) )
					{
						for( i = 0; i < engine->threads_in_use; i++ )
							search_kill( &engine->searches[ i ] );
						rc = false;
					}
				}
			}
			break;
		case COMPLETE:
			if( (engine->max_depth <= depth) || abs( engine->lower_bound[ depth ] ) >= MIN_CHECKMATE_VALUE )
				rc = false;
			break;
		}
		if( found_next_search )
			break;
	}
	if( rc == false )
		engine->search_done = true;
	if( engine->search_done == true )
		rc = false;
	return rc;
}

void parallel_search_set_contempt_factor( parallel_search_t* ps, int cf )
{
	int i;
	ps->contempt_factor = cf;
	for( i = 0; i < ps->thread_count; i++ )
		search_set_contempt_factor( &ps->searches[ i ], cf );
}

void parallel_search_set_computer_color( parallel_search_t* ps, int c )
{
	int i;
	ps->computer_color = c;
	for( i = 0; i < ps->thread_count; i++ )
		search_set_computer_color( &ps->searches[ i ], c );
}


void parallel_search_kill( parallel_search_t* engine, bool call_action_function )
{
	int i;
	/**********************************/
	pthread_mutex_lock( engine->lock );
	/**********************************/
	if( !call_action_function )
		engine->action_function = NULL;
	engine->search_done = true;
	for( i = 0; i < engine->threads_in_use; i++ )
		search_kill( engine->searches + i );
	/**********************************/
	pthread_mutex_unlock( engine->lock );
	/**********************************/
	/**********************************/
	while( pthread_mutex_trylock( engine->main_lock ) != 0 )
	{
		/**********************************/
		pthread_mutex_lock( engine->lock );
		/**********************************/
		if( !call_action_function )
			engine->action_function = NULL;
		engine->search_done = true;
		for( i = 0; i < engine->threads_in_use; i++ )
			search_kill( engine->searches + i );
		/**********************************/
		pthread_mutex_unlock( engine->lock );
		/**********************************/
	}
	/**********************************/
	/**********************************/
	pthread_mutex_unlock( engine->main_lock );
	/**********************************/
}

static void remove_outliers( parallel_search_t* engine, int depth, int this_thread )
{
	int i;
	for( i = 0; i < engine->thread_count; i++ )
	{
		if( i == this_thread )
			continue;
		if( engine->depths[ i ] == depth )
		{
			if( (engine->test_values[i] >= engine->upper_bound[depth]) || (engine->test_values[i] <= engine->lower_bound[depth]) )
				search_kill( engine->searches + i );
		}
	}
}

static void* parallel_thread_handler( void* data_ptr )
{
	parallel_search_t* engine;
	int this_thread, test_score, depth, i;
	bool start;
	bool test_result;
	engine = (parallel_search_t*) data_ptr;
	/********************************/
	pthread_mutex_lock( engine->lock );
	/********************************/
	this_thread = engine->threads_in_use;
	engine->searches[ this_thread ].thread_number = this_thread;
	engine->threads_in_use++;
//	printf( "Starting thread #%i\n", this_thread );
	while( determine_next_search( engine, &test_score, &depth, this_thread ) )
	{
		if( depth > MAX_SEARCH_DEPTH ) 
			break;
		engine->test_values[ this_thread ] = test_score;
		engine->depths[ this_thread ] = depth;
		if( engine->max_depth_complete >= MIN_DEPTH )
			search_set_kill_time( engine->searches + this_thread, engine->kill_time );
		else
			search_set_kill_time( engine->searches + this_thread, 0 );
		/********************************/
		pthread_mutex_unlock( engine->lock );
		/********************************/
		test_result = search_test( engine->searches + this_thread, test_score, depth );
		/********************************/
		pthread_mutex_lock( engine->lock );
		/********************************/
		engine->node_count += engine->searches[ this_thread ].node_count;
		engine->table_hits += engine->searches[ this_thread ].table_hits;
		engine->null_hits += engine->searches[ this_thread ].null_hits;
		engine->late_move_reductions += engine->searches[ this_thread ].late_move_reductions;
		search_reset_counters( engine->searches + this_thread );
		brake_if_needed( engine );
		if( test_score < engine->upper_bound[ depth ] && test_score > engine->lower_bound[ depth ] )
		{
			if( test_result )
			{
				engine->lower_bound[ depth ] = test_score;
			    assert( !move_is_empty( engine->searches[ this_thread ].best_move ) );
				engine->best_move[ depth ] = engine->searches[ this_thread ].best_move;
			}
			else
			{
				engine->upper_bound[ depth ] = test_score;
			}
		}
//		parallel_search_display( engine, stdout );
//		printf( "thread %i depth %i lower %i upper %i test %i\n", this_thread, depth, engine->lower_bound[ depth ], engine->upper_bound[ depth ], test_score  );
		remove_outliers( engine, depth, this_thread );
		if( (engine->kill_time != 0) && (engine->max_depth_complete >= MIN_DEPTH) )
		{
			if( time( NULL ) >= engine->kill_time )
				break;
		}
	}
	/********************************/
	pthread_mutex_unlock( engine->lock );
	/********************************/
//	printf( "Ending thread #%i\n", this_thread );
	pthread_exit( NULL );
	return NULL;
}

void parallel_search_make_move_on_boards( parallel_search_t* engine, move_t move )
{
	int i;
	for( i = 0; i < engine->thread_count; i++ )
		board_make_move( engine->searches[ i ].board, move );
}

void parallel_search_undo_move_on_boards( parallel_search_t* engine )
{
	int i;
	for( i = 0; i < engine->thread_count; i++ )
		board_undo_move( engine->searches[ i ].board );
}

void parallel_search_set_use_max_time( parallel_search_t* engine, bool use )
{
	engine->use_max_time = use;
}
