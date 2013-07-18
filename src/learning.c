#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "parallel_search.h"


#define LAMBDA 0.70

void parallel_search_set_learning( parallel_search_t* engine, bool setting )
{
	int rc;
	if( engine->learning_on )
	{
		rc = fclose( engine->learning_file );
		if( rc != 0 )
		{
			printf( "Error closing learning file after writing\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
	}
	if( setting )
	{
		engine->learning_file = fopen( "learning.dat", "wb" );
		if( engine->learning_file == NULL )
		{
			printf( "Error opening learning file for writing\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
	}
	engine->learning_on = setting;
}


void parallel_search_trigger_learning( parallel_search_t* engine, int computer_side, result_t result )
{
	double td[ max_half_moves ];
	double weights[ max_half_moves ];
	int rc;
	FILE* fp;
	board_t leaf_node;
	int score, old_grain, i, j;
	double normal_score, nscore1, nscore2, derivative, last_nscore, total_moves;
	search_t search;
	static_evaluator_t* evaluator;
	static_evaluator_t deval;
	evaluator_parameters_t params;
	evaluator_parameters_t jacobian[ max_half_moves ];
	evaluator_parameters_t changes[ max_half_moves ];
	evaluator_parameters_t total_changes;
	if( engine->learning_on == false )
		return;
	parallel_search_kill( engine, false );
	printf( "Learning...\n " );
	parallel_search_set_learning( engine, false );
	evaluator = engine->searches[ 0 ].evaluator;
	fp = fopen( "learning.dat", "rb" );
	if( fp == NULL )
	{
		printf( "Error opening learning file for reading\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	score = 0;
	old_grain = evaluator->grain_size;
	for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		params[ i ] = engine->eval_params[ i ];
	j = 0;
	last_nscore = 0.0;
	static_evaluator_destroy( &deval );
	while( !feof( fp ) )
	{
		printf( "Evaluating position %i...\n", j );
		rc = fread( &leaf_node, sizeof( board_t ), 1, fp );
		if( rc == 0 )
		{
			break;
		}
		else if( rc != 1 )
		{
			printf( "Error reading learning file\n" );
			printf( "Line %i File %s\n", __LINE__, __FILE__ );
			abort();
		}
		static_evaluator_create( &deval, params );
		evaluator->grain_size = 2;
		search_create( &search, &leaf_node, evaluator, engine->searches[ 0 ].table );
		search.grain_size = 2;
		memory_table_reset( engine->searches[ 0 ].table );
		score = search_quiescent_value( &search, score );
		static_evaluator_destroy( &deval );
		if( computer_side == black )
			score = -score;
		normal_score = tanh( evaluator->beta * ((double) score / 100.0) );
//		printf( "Score = %i %f | ", score, normal_score );
		search_destroy( &search );
		for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		{
			params[ i ] += search.grain_size;
			static_evaluator_create( &deval, params );
			evaluator->grain_size = 2;
			search_create( &search, &leaf_node, &deval, engine->searches[ 0 ].table );
			search.grain_size = 2;
			score = search_quiescent_value( &search, score );
			nscore1 = tanh( deval.beta * ((double) score / 100.0) );
			search_destroy( &search );
			static_evaluator_destroy( &deval );
			
			params[ i ] -= 2 * search.grain_size;
			static_evaluator_create( &deval, params );
			evaluator->grain_size = 2;
			search_create( &search, &leaf_node, &deval, engine->searches[ 0 ].table );
			search.grain_size = 2;
			score = search_quiescent_value( &search, score );
			nscore2 = tanh( deval.beta * ((double) score / 100.0) );
			derivative = (double) (nscore1 - nscore2) / (double) (2 * search.grain_size);
			if( computer_side == black )
				derivative = -derivative;
			search_destroy( &search );
			static_evaluator_destroy( &deval );

			params[ i ] += search.grain_size;
			
			jacobian[ j ][ i ] = derivative;
		}
		td[ j ] = normal_score - last_nscore;
		last_nscore = normal_score;
//		printf( "%f \n", td[ j ] );
		j++;
	}
	if( fclose( fp ) != 0 )
	{
		printf( "Error closing learning file for reading\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	printf( "Performing TD-Leaf algorithm\n" );
	if( result == WHITE_WIN )
		normal_score = +1.0;
	else if( result == BLACK_WIN )
		normal_score = -1.0;
	else
		normal_score = 0.0;
	if( computer_side == black )
		normal_score = -normal_score;
//	printf( "Score = +/-inf %f | ", normal_score );
	td[ j ] = normal_score - last_nscore;
//	printf( "%f \n", td[ j ] );
	j++;
	total_moves = j;
	for( j = 0; j < total_moves; j++ )
	{
		weights[ j ] = 0.0;
		for( i = j; i < total_moves; i++ )
			weights[ j ] += td[ i ] * pow( LAMBDA, (double) (i-j) );
//		printf( "%e\n",weights[j] );
		for( i = 0; i < EVAL_PARAM_COUNT; i++ )
				changes[ j ][ i ] = weights[ j ] * jacobian[ j ][ i ];
	}
	for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		total_changes[ i ] = 0.0;
	for( j = 0; j < total_moves; j++ )
	{
		for( i = 0; i < EVAL_PARAM_COUNT; i++ )
			total_changes[ i ] += LEARNING_ALPHA * changes[ j ][ i ];
	}
	printf( "Done Learning\n" );
	printf( "Old   = " );
	for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		printf( "%.1f ", engine->eval_params[ i ] );
	printf( "\n" ); 
	printf( "Change= " );
	for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		printf( "%.1f ", total_changes[ i ] );
	printf( "\n" ); 
	printf( "New   = " );
	for( i = 0; i < EVAL_PARAM_COUNT; i++ )
	{
		engine->eval_params[ i ] += total_changes[ i ];
		printf( "%.1f ", engine->eval_params[ i ] );
	}
	printf( "\n" ); 
	fp = fopen( "history.txt", "at" );
	for( i = 0; i < EVAL_PARAM_COUNT; i++ )
		fprintf( fp, "%e ", engine->eval_params[ i ] );
	fprintf( fp, "\n" );
	rc = fclose( fp );
	if( rc != 0 )
	{
		printf( "Error closing learning file after reading" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	fp = fopen( "brain.dat", "wb" );
	if( fp == NULL )
	{
		printf( "Error opening brain for writing\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	rc = fwrite( engine->eval_params, sizeof( evaluator_parameters_t ), 1, fp );
	if( rc != 1 )
	{
		printf( "Error writing to brain\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	rc = fclose( fp );
	if( rc != 0 )
	{
		printf( "Error closing brain after writing\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	static_evaluator_destroy( engine->evaluator );
	static_evaluator_create( engine->evaluator, engine->eval_params );
	evaluator->grain_size = old_grain;
	parallel_search_set_learning( engine, true );
}

