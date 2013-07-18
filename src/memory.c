#include "search.h"
#include "params.h"
#include "debug.h"
#include <stdlib.h>


static void memory_entry_reset( memory_entry_t* entry )
{
	entry->depth = -1;
	entry->touched = 0;
}

static bool memory_entry_is_empty( memory_entry_t* entry, int count )
{
	return (bool) (entry->depth == -1);
}

void memory_table_reset( memory_table_t* table )
{
	int i;
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_lock( table->locks + i );
	/*****************************************/
	for( i = 0; i < table->entry_count; i++ )
	{
		memory_entry_reset( table->upper_bound_local_store[ white ] + i );
		memory_entry_reset( table->lower_bound_local_store[ white ] + i );
		memory_entry_reset( table->upper_bound_global_store[ white ] + i );
		memory_entry_reset( table->lower_bound_global_store[ white ] + i );
		memory_entry_reset( table->upper_bound_local_store[ black ] + i );
		memory_entry_reset( table->lower_bound_local_store[ black ] + i );
		memory_entry_reset( table->upper_bound_global_store[ black ] + i );
		memory_entry_reset( table->lower_bound_global_store[ black ] + i );
	}
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_unlock( table->locks + i );
	/*****************************************/
}

void memory_table_reset_touched_flags( memory_table_t* table )
{
	int i;
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_lock( table->locks + i );
	/*****************************************/
	for( i = 0; i < table->entry_count; i++ )
	{
		table->upper_bound_local_store[ white ][ i ].touched = 0;
		table->lower_bound_local_store[ white ][ i ].touched = 0;
		table->upper_bound_global_store[ white ][ i ].touched = 0;
		table->lower_bound_global_store[ white ][ i ].touched = 0;
		table->upper_bound_local_store[ black ][ i ].touched = 0;
		table->lower_bound_local_store[ black ][ i ].touched = 0;
		table->upper_bound_global_store[ black ][ i ].touched = 0;
		table->lower_bound_global_store[ black ][ i ].touched = 0;
	}
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_unlock( table->locks + i );
	/*****************************************/
}

void memory_table_kill_untouched_entries( memory_table_t* table )
{
	int i;
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_lock( table->locks + i );
	/*****************************************/
	for( i = 0; i < table->entry_count; i++ )
	{
		if( table->upper_bound_local_store[ white ][ i ].touched == 0 )
			memory_entry_reset( table->upper_bound_local_store[ white ] + i );
		if( table->lower_bound_local_store[ white ][ i ].touched == 0 )
			memory_entry_reset( table->lower_bound_local_store[ white ] + i );
		if( table->upper_bound_global_store[ white ][ i ].touched == 0 )
			memory_entry_reset( table->upper_bound_global_store[ white ] + i );
		if( table->lower_bound_global_store[ white ][ i ].touched == 0 )
			memory_entry_reset( table->lower_bound_global_store[ white ] + i );
		if( table->upper_bound_local_store[ black ][ i ].touched == 0 )
			memory_entry_reset( table->upper_bound_local_store[ black ] + i );
		if( table->lower_bound_local_store[ black ][ i ].touched == 0 )
			memory_entry_reset( table->lower_bound_local_store[ black ] + i );
		if( table->upper_bound_global_store[ black ][ i ].touched == 0 )
			memory_entry_reset( table->upper_bound_global_store[ black ] + i );
		if( table->lower_bound_global_store[ black ][ i ].touched == 0 )
			memory_entry_reset( table->lower_bound_global_store[ black ] + i );
	}
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_unlock( table->locks + i );
	/*****************************************/
}


void memory_table_refresh( memory_table_t* table )
{
	int i;
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_lock( table->locks + i );
	/*****************************************/
	for( i = 0; i < table->entry_count; i++ )
	{
		table->upper_bound_local_store[ white ] = table->upper_bound_global_store[ white ];
		table->lower_bound_local_store[ white ] = table->upper_bound_global_store[ white ];  
		memory_entry_reset( table->upper_bound_global_store[ white ] + i );
		memory_entry_reset( table->lower_bound_global_store[ white ] + i );
		table->upper_bound_local_store[ black ] = table->upper_bound_global_store[ black ];
		table->lower_bound_local_store[ black ] = table->upper_bound_global_store[ black ];  
		memory_entry_reset( table->upper_bound_global_store[ black ] + i );
		memory_entry_reset( table->lower_bound_global_store[ black ] + i );
	}
	/*****************************************/
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_unlock( table->locks + i );
	/*****************************************/
}


void memory_table_create( memory_table_t* table, int entry_count, int lock_count, int thread_count )
{
	int i;
	table->entry_count = entry_count / 8;
	table->upper_bound_local_store[ white ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->upper_bound_local_store[ white ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->lower_bound_local_store[ white ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->lower_bound_local_store[ white ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->upper_bound_global_store[ white ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->upper_bound_global_store[ white ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->lower_bound_global_store[ white ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->lower_bound_global_store[ white ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->upper_bound_local_store[ black ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->upper_bound_local_store[ black ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->lower_bound_local_store[ black ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->lower_bound_local_store[ black ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->upper_bound_global_store[ black ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->upper_bound_global_store[ black ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	table->lower_bound_global_store[ black ] = (memory_entry_t*) malloc( sizeof( memory_entry_t ) * table->entry_count );
	if( table->lower_bound_global_store[ black ] == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	assert( lock_count > 0 );
	assert( lock_count <= table->entry_count );
	table->locks = (pthread_mutex_t*) malloc( sizeof( pthread_mutex_t ) * lock_count );
	table->lock_count = lock_count;
	for( i = 0; i < lock_count; i++ )
		pthread_mutex_init( table->locks + i, NULL );
	table->thread_count = thread_count;
	table->local_entry_count = table->entry_count / thread_count;
	memory_table_reset( table );
}

void memory_table_destroy( memory_table_t* table )
{
	int i;
	free( table->upper_bound_local_store[ black ] );
	free( table->lower_bound_local_store[ black ] );
	free( table->upper_bound_global_store[ black ] );
	free( table->lower_bound_global_store[ black ] );
	free( table->upper_bound_local_store[ white ] );
	free( table->lower_bound_local_store[ white ] );
	free( table->upper_bound_global_store[ white ] );
	free( table->lower_bound_global_store[ white ] );
	free( table->locks );
	for( i = 0; i < table->lock_count; i++ )
		pthread_mutex_destroy( table->locks + i );
}

void memory_table_write( search_t* search, move_t move, bool test_result, int test_score, int depth )
{
	int global_index;
	int local_index;
	int lock_index;
	int color_index;
	memory_entry_t entry;
	uqword key;
	udword key32;
#ifdef TABLES_OFF
	return;
#endif
	if( board_position_is_repeat( search->board ) )
		return;
	if( abs( test_score ) >= MIN_CHECKMATE_VALUE ) 
			return;
	key = search->board->position_key;
	key32 = (udword) key;
	global_index = key32 % search->table->entry_count;
	local_index = search->thread_number * search->table->local_entry_count;
	local_index += key32 % search->table->local_entry_count;
	lock_index = key32 % search->table->lock_count;
	color_index = search->board->flags.side_to_move;
	entry.key = key;
	entry.score = test_score;
	entry.move = move;
	entry.depth = depth;
	entry.touched = 1;
	entry.null_moves_verified = search->verify_null_move;
	if( test_result == true )
	{
		entry.score += search->grain_size / 2;
		search->table->lower_bound_local_store[ color_index ][ local_index ] = entry;
		pthread_mutex_lock( search->table->locks + lock_index );
		if( (depth >= search->table->lower_bound_global_store[ color_index ][ global_index ].depth) && entry.null_moves_verified )
			search->table->lower_bound_global_store[ color_index ][ global_index ] = entry;
		pthread_mutex_unlock( search->table->locks + lock_index );
	}
	else
	{
		entry.score -= search->grain_size / 2;
		search->table->upper_bound_local_store[ color_index ][ local_index ] = entry;
		pthread_mutex_lock( search->table->locks + lock_index );
		if( (depth >= search->table->upper_bound_global_store[ color_index ][ global_index ].depth) && entry.null_moves_verified )
			search->table->upper_bound_global_store[ color_index ][ global_index ] = entry;
		pthread_mutex_unlock( search->table->locks + lock_index );
	}
}
	
bool memory_table_read( search_t* search, move_t* move, int* lower_bound, int* upper_bound, int min_depth, int test_score )
{
	int global_index;
	int local_index;
	int lock_index;
	int color_index;
	int upper_bound_depth;
	bool rc;
	uqword key;
	udword key32;
	memory_entry_t* entry_ptr;
	*upper_bound = +INFTY;
	*lower_bound = -INFTY;
	move_set_empty( move );
	rc = false;
#ifdef TABLES_OFF
	return false;
#endif
	if( board_position_is_repeat( search->board ) )
		return false;
	key = search->board->position_key;
	key32 = (udword) key;
	global_index = key32 % search->table->entry_count;
	local_index = search->thread_number * search->table->local_entry_count;
	local_index += key32 % search->table->local_entry_count;
	lock_index = key32 % search->table->lock_count;
	color_index = search->board->flags.side_to_move;
	pthread_mutex_lock( search->table->locks + lock_index );
	entry_ptr = search->table->upper_bound_global_store[ color_index ] + global_index;
	upper_bound_depth = -1;
	if( key != entry_ptr->key )
		entry_ptr = search->table->upper_bound_local_store[ color_index ] + local_index; 
	if( (key == entry_ptr->key) && !(search->verify_null_move && !entry_ptr->null_moves_verified))
	{
		entry_ptr->touched = 1;
		*move = entry_ptr->move;
		upper_bound_depth = entry_ptr->depth;
		if( min_depth <= entry_ptr->depth )
		{
			rc = true;
			*upper_bound = entry_ptr->score;
		}
	}
	entry_ptr = search->table->lower_bound_global_store[ color_index ] + global_index; 
	if( key != entry_ptr->key )
		entry_ptr = search->table->lower_bound_local_store[ color_index ] + local_index; 
	if( (key == entry_ptr->key) && !(search->verify_null_move && !entry_ptr->null_moves_verified) )
	{
		entry_ptr->touched = 1;
		if( (upper_bound_depth <= entry_ptr->depth) || move_is_empty( *move ) )
			*move = entry_ptr->move;
		if( min_depth <= entry_ptr->depth )
		{
			rc = true;
			*lower_bound = entry_ptr->score;
		}
	}
	pthread_mutex_unlock( search->table->locks + lock_index );
	if( move_is_empty( *move ) )
		rc = false;
	return rc;
}
	


