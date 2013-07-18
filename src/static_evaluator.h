#ifndef STATIC_EVALUATOR_T_H_
#define STATIC_EVALUATOR_T_H_

#include "board.h"
#include <pthread.h>

#define EVAL_PARAM_COUNT 16
#define PAWN_TABLE_LOCKS (PAWN_TABLE_LOCKS_PER_THREAD*THREAD_COUNT)
#define ENDGAME_DEFINITION 1200

typedef double evaluator_parameters_t[ EVAL_PARAM_COUNT ];

static evaluator_parameters_t default_evaluator_parameters = 
{ 
		/* knight value */          325.0, 
		/* bishop value */          325.0, 
		/* rook value */            500.0, 
		/* queen value */           900.0, 
		/* max nonmaterial value */ 199.9,  
		/* pawn sq.val. weight   */ 100.0,
        /* doubled pawn penalty  */  10.0,
        /* isolated   "      "   */  20.0,
        /* backwards "     "     */   8.0,
        /* passed pawn bonus     */  20.0,
		/* king sq.val. weight   */ 100.0,
		/* knight sq.val. weight */ 100.0,
		/* bishop sq.val. weight */ 100.0, 
		/* rook semi open file   */  10.0, 
		/* rook open file        */  15.0,
		/* rook on 7th rank      */  20.0
};

typedef struct
{
	udword key;
	sdword score;
} pawn_table_entry_t;

typedef struct
{
	int material_values[ 7 ];
	int draw_value;
	int maximum_non_material_score;
	int grain_size;
	int pawn_square_value_weight;
	int king_square_value_weight;
	int knight_square_value_weight;
	int bishop_square_value_weight;
	int doubled_pawn_penalty;
	int isolated_pawn_penalty;
	int backwards_pawn_penalty;
	int passed_pawn_bonus;
	int rook_semi_open_file_bonus;
	int rook_open_file_bonus;
	int rook_rank7_bonus;
	int max_material;
	int max_material_wo_bonus;
	int starting_material;
	double beta;
	pawn_table_entry_t pawn_table[ PAWN_TABLE_SIZE ];
	pthread_mutex_t locks[ PAWN_TABLE_LOCKS ];
} static_evaluator_t;

void static_evaluator_create( static_evaluator_t* evaluator, evaluator_parameters_t params );
int static_evaluator_material_balance( static_evaluator_t* evaluator, board_t* board );
bool static_evaluator_test( static_evaluator_t* evaluator, board_t* board, int test_value );
void static_evaluator_destroy( static_evaluator_t* evaluator );
int static_evaluator_draw_value( static_evaluator_t* evaluator );
int static_evaluator_checkmate_value( static_evaluator_t* evaluator, int ply );
void static_evaluator_reset_pawn_table( static_evaluator_t* evaluator );

#endif /*STATIC_EVALUATOR_T_H_*/
