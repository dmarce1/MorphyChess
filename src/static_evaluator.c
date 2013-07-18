#define IN_STATIC_EVAL
#include "params.h"
#include "static_evaluator.h"
#include "debug.h"
#include "bits.h"
#include <stdlib.h>

static bool initialized = false;



static void write_pawn_table( static_evaluator_t* evaluator, udword key, int score );
static bool read_pawn_table( static_evaluator_t* evaluator, udword key, int* score );
static int evaluate_pawns_and_kings( static_evaluator_t* evaluator, board_t* board, int white_mat, int black_mat );
static int evaluate_white_king_pawn( int pawn_rank[ 2 ][ 10 ], int file );
static int evaluate_black_king_pawn( int pawn_rank[ 2 ][ 10 ], int file );

const int white_pawn_square_value[ 64 ] = 
{
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0, -40, -40,   0,   0,   0,
		  1,   2,   3, -10, -10,   3,   2,   1,
		  2,   4,   6,   8,   8,   6,   4,   2,
		  3,   6,   9,  12,  12,   9,   6,   3,
		  4,   8,  12,  16,  16,  12,   8,   4,
		  5,  10,  15,  20,  20,  15,  10,   5,
		  0,   0,   0,   0,   0,   0,   0,   0
};

const int black_pawn_square_value[ 64 ] = 
{
		  0,   0,   0,   0,   0,   0,   0,   0,
		  5,  10,  15,  20,  20,  15,  10,   5,
		  4,   8,  12,  16,  16,  12,   8,   4,
		  3,   6,   9,  12,  12,   9,   6,   3,
		  2,   4,   6,   8,   8,   6,   4,   2,
		  1,   2,   3, -10, -10,   3,   2,   1,
		  0,   0,   0, -40, -40,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0
};

const int black_king_square_value[ 64 ] = 
{
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-20, -20, -20, -20, -20, -20, -20, -20,
	  0,  20,  40, -20,   0, -20,  40,  20
};

const int white_king_square_value[ 64 ] = 
{
	  0,  20,  40, -20,   0, -20,  40,  20,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40,
	-20, -20, -20, -20, -20, -20, -20, -20
};

const int endgame_king_square_value[ 64 ] = 
{
	  0,  10,  20,  30,  30,  20,  10,   0,
	 10,  20,  30,  40,  40,  30,  20,  10,
	 20,  30,  40,  50,  50,  40,  30,  20,
	 30,  40,  50,  60,  60,  50,  40,  30,
	 30,  40,  50,  60,  60,  50,  40,  30,
	 20,  30,  40,  50,  50,  40,  30,  20,
	 10,  20,  30,  40,  40,  30,  20,  10,
	  0,  10,  20,  30,  30,  20,  10,   0
};

int white_knight_square_value[64] = 
{
	-10, -30, -10, -10, -10, -10, -30, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -10, -10, -10, -10, -10, -10, -10,
};

int black_knight_square_value[64] = 
{
	-10, -10, -10, -10, -10, -10, -10, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -30, -10, -10, -10, -10, -30, -10
};

int white_bishop_square_value[64] = {
	-10, -10, -20, -10, -10, -20, -10, -10
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -10, -10, -10, -10, -10, -10, -10,
};

int black_bishop_square_value[64] = {
	-10, -10, -10, -10, -10, -10, -10, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-10, -10, -20, -10, -10, -20, -10, -10
};

void static_evaluator_create( static_evaluator_t* evaluator, evaluator_parameters_t params )
{
	int i;
	if( !initialized )
	{
//		initialize();
		initialized = true;
	}
	evaluator->material_values[ 0 ] = 0;
	evaluator->material_values[ pawn ] = 100;
	for( i = knight; i <= queen; i++ )
		evaluator->material_values[ i ] = params[ i - pawn - 1 ];
	evaluator->material_values[ king ] = INFTY;
	evaluator->maximum_non_material_score = params[ 4 ];
	evaluator->pawn_square_value_weight = params[ 5 ];
	evaluator->doubled_pawn_penalty = params[ 6 ];
	evaluator->isolated_pawn_penalty = params[ 7 ];
	evaluator->backwards_pawn_penalty = params[ 8 ];
	evaluator->passed_pawn_bonus = params[ 9 ];
	evaluator->king_square_value_weight = params[ 10 ];
	evaluator->knight_square_value_weight = params[ 11 ];
	evaluator->bishop_square_value_weight = params[ 12 ];
	evaluator->rook_semi_open_file_bonus = params[ 13 ];
	evaluator->rook_open_file_bonus = params[ 14 ];
	evaluator->rook_rank7_bonus = params[ 15 ];

	evaluator->grain_size = DEFAULT_GRAIN_SIZE;
	evaluator->maximum_non_material_score /= evaluator->grain_size;
	evaluator->maximum_non_material_score *= evaluator->grain_size;
	evaluator->beta = 0.25;
	if( evaluator->maximum_non_material_score < 0.0 )
		evaluator->maximum_non_material_score = 0.0;
	for( i = 0; i < PAWN_TABLE_LOCKS; i++ )
		pthread_mutex_init( evaluator->locks + i, NULL );
	static_evaluator_reset_pawn_table( evaluator );
	evaluator->starting_material = 16 * evaluator->material_values[ pawn ];
	evaluator->starting_material += 4 * evaluator->material_values[ knight ];
	evaluator->starting_material += 4 * evaluator->material_values[ bishop ];
	evaluator->starting_material += 4 * evaluator->material_values[ rook ];
	evaluator->starting_material += 2 * evaluator->material_values[ queen ];
	evaluator->max_material = evaluator->starting_material / 2 - evaluator->material_values[ queen ];
	evaluator->max_material_wo_bonus = evaluator->starting_material / 2 - 1600;
}

int static_evaluator_material_score( static_evaluator_t* evaluator, board_t* board, int* white_mat, int* black_mat )
{
	int i, total_mat, winners_pawns, dif_mat, bonus, score, n_bonus, r_bonus, q_bonus;
	int total_pawns;
	uqword qword;
	*white_mat = 0;
	*black_mat = 0;
	for( i = pawn; i <= queen; i++ )
	{
		*white_mat += board->piece_counts[ white ][ i ] * evaluator->material_values[ i ];
		*black_mat += board->piece_counts[ black ][ i ] * evaluator->material_values[ i ];
	}
	dif_mat = *white_mat - *black_mat;
	total_mat = *white_mat + *black_mat;
	winners_pawns = board->piece_counts[ (dif_mat > 0) ? white : black ][ pawn ];
	bonus = dif_mat * (evaluator->starting_material - total_mat) * winners_pawns;
	bonus /= (evaluator->starting_material - 1600) * (winners_pawns + 1);
	

	total_pawns = board->piece_counts[ white ][ pawn ] + board->piece_counts[ black ][ pawn ];
	n_bonus = 2 * total_pawns - 16;
	q_bonus = total_pawns;
	q_bonus += board->piece_counts[ white ][ bishop ];
	q_bonus += board->piece_counts[ white ][ knight ];
	q_bonus += board->piece_counts[ black ][ bishop ];
	q_bonus += board->piece_counts[ black ][ knight ];
	r_bonus = 32 - 2 * total_pawns;
	if( board->piece_counts[ white ][ knight ] >= 1 )
	{
		bonus += n_bonus;
	    if( board->piece_counts[ white ][ knight ] > 1 )
			bonus -= 11;
	}
	if( board->piece_counts[ black ][ knight ] >= 1 )
	{
		bonus -= n_bonus;
	    if( board->piece_counts[ black ][ knight ] > 1 )
			bonus += 11;
	}
	if( board->piece_counts[ white ][ bishop ] > 1 )
		bonus += 13;
	if( board->piece_counts[ black ][ bishop ] > 1 )
		bonus -= 13;
	if( board->piece_counts[ white ][ queen ] > 0 )
		bonus += q_bonus;
	if( board->piece_counts[ black ][ queen ] > 0 )
		bonus -= q_bonus;
	bonus += board->piece_counts[ white ][ rook ] * r_bonus;
	bonus -= board->piece_counts[ black ][ rook ] * r_bonus;
		

		
	score = dif_mat;
	if( score > evaluator->max_material_wo_bonus )
		score = evaluator->max_material_wo_bonus;
	if( score < -evaluator->max_material_wo_bonus )
		score = -evaluator->max_material_wo_bonus;
	score += bonus;

	if( score > evaluator->max_material )
		score = evaluator->max_material;
	if( score < -evaluator->max_material )
		score = -evaluator->max_material;
	if( board->flags.side_to_move == black )
		score = -score;
	return score;
}

int static_evaluator_material_balance( static_evaluator_t* evaluator, board_t* board )
{
	int white_mat, black_mat;
	return static_evaluator_material_score( evaluator, board, &white_mat, &black_mat );
}


int static_evaluator_non_material_score( static_evaluator_t* evaluator, board_t* board, int white_mat, int black_mat )
{
	int i;
	int score;
	int square;
	int file;
	uqword pieces;
	uqword mask;
	uqword test;
	
	score = evaluate_pawns_and_kings( evaluator, board, white_mat, black_mat );
	
	pieces = board->piece_boards[ white ][ knight ];
	while( pieces != 0x0LL )
	{
		square = bits_remove_first_one( &pieces );
		score += (evaluator->knight_square_value_weight * white_knight_square_value[ square ]) / 100;
	}
	pieces = board->piece_boards[ white ][ bishop ];
	while( pieces != 0x0LL )
	{
		square = bits_remove_first_one( &pieces );
		score += (evaluator->bishop_square_value_weight * white_bishop_square_value[ square ]) / 100;
	}
	pieces = board->piece_boards[ white ][ rook ];
	while( pieces != 0x0LL )
	{
		square = bits_remove_first_one( &pieces );
		file = square % 8;
		mask = 0x0101010101010101LL << file;
		test = board->piece_boards[ white ][ pawn ] & mask; 
		if( test == 0x0LL )
		{
			test = board->piece_boards[ black ][ pawn ] & mask; 
			if( test == 0x0LL )
				score += evaluator->rook_open_file_bonus;
			else
				score += evaluator->rook_semi_open_file_bonus;
		}
		if( square / 8 == 6 )
			score += evaluator->rook_rank7_bonus;
	}
	
	pieces = board->piece_boards[ black ][ knight ];
	while( pieces != 0x0LL )
	{
		square = bits_remove_first_one( &pieces );
		score -= (evaluator->knight_square_value_weight  * black_knight_square_value[ square ]) / 100;
	}
	pieces = board->piece_boards[ black ][ bishop ];
	while( pieces != 0x0LL )
	{
		square = bits_remove_first_one( &pieces );
		score -= (evaluator->bishop_square_value_weight * black_bishop_square_value[ square ]) / 100;
	}
	pieces = board->piece_boards[ black ][ rook ];
	while( pieces != 0x0LL )
	{
		square = bits_remove_first_one( &pieces );
		file = square % 8;
		mask = 0x0101010101010101LL << file;
		test = board->piece_boards[ black ][ pawn ] & mask; 
		if( test == 0x0LL )
		{
			test = board->piece_boards[ white ][ pawn ] & mask; 
			if( test == 0x0LL )
				score -= evaluator->rook_open_file_bonus;
			else
				score -= evaluator->rook_semi_open_file_bonus;
		}
		if( square / 8 == 1 )
			score -= evaluator->rook_rank7_bonus;
	}
	
	if( score > evaluator->maximum_non_material_score )
		score = evaluator->maximum_non_material_score;
	else if( score < -evaluator->maximum_non_material_score )
		score = -evaluator->maximum_non_material_score;
	if( score > 0 )
	{
		if( 2 * board->_50_move_rule > score )
			score /= 2;
		else
			score -= board->_50_move_rule;
	}
	else if( score < 0 )
	{
		if( 2 * board->_50_move_rule < score )
			score /= 2;
		else
			score += board->_50_move_rule;
	}
	if( board->flags.side_to_move == black )
		score = -score;
	return score;
}

int static_evaluator_checkmate_value( static_evaluator_t* evaluator, int ply )
{
	int score;
	score = MIN_CHECKMATE_VALUE + 5 * (MAX_DEPTH - ply);
	score /= evaluator->grain_size;
	score *= evaluator->grain_size;
	return score;
}

bool static_evaluator_test( static_evaluator_t* evaluator, board_t* board, int test_value )
{
	int score, material_score;
	int white_mat, black_mat;
	bool test_result;
	material_score = static_evaluator_material_score( evaluator, board, &white_mat, &black_mat );
	score = material_score / evaluator->grain_size;
	score *= evaluator->grain_size;
	if( (score - evaluator->maximum_non_material_score > test_value) )
	{
		test_result = true;
	}
	else if( (score + evaluator->maximum_non_material_score < test_value) )
	{
		test_result = false;
	}
	else
	{
		score = material_score + static_evaluator_non_material_score( evaluator, board, white_mat, black_mat );
		score /= evaluator->grain_size;
		score *= evaluator->grain_size;
		assert( score != test_value );
		if( score > test_value )
			test_result = true;
		else
			test_result = false;
	}
	return test_result;
}


void static_evaluator_destroy( static_evaluator_t* evaluator )
{
	int i;
	for( i = 0; i < PAWN_TABLE_LOCKS; i++ )
		pthread_mutex_destroy( evaluator->locks + i );
}


void write_pawn_table( static_evaluator_t* evaluator, udword key, int score )
{
	int table_index;
	int lock_index;
#ifdef PAWN_TABLE_OFF
	return;
#endif
	
	table_index = key % PAWN_TABLE_SIZE;
	lock_index = key % PAWN_TABLE_LOCKS;
	/****************************************************/
	pthread_mutex_lock( evaluator->locks + lock_index );
	/****************************************************/
	evaluator->pawn_table[ table_index ].key = key;
	evaluator->pawn_table[ table_index ].score = score;
	/****************************************************/
	pthread_mutex_unlock( evaluator->locks + lock_index );
	/****************************************************/
}


bool read_pawn_table( static_evaluator_t* evaluator, udword key, int* score )
{
	int table_index;
	int lock_index;
	bool rc;
	
#ifdef PAWN_TABLE_OFF
	return false;
#endif
	table_index = key % PAWN_TABLE_SIZE;
	lock_index = key % PAWN_TABLE_LOCKS;
	/****************************************************/
	pthread_mutex_lock( evaluator->locks + lock_index );
	/****************************************************/
	if( (evaluator->pawn_table[ table_index ].key == key) &&  (key != 0) )
	{
		*score = evaluator->pawn_table[ table_index ].score;
		rc = true;
	}
	else
	{
		rc = false;
	}
	/****************************************************/
	pthread_mutex_unlock( evaluator->locks + lock_index );
	/****************************************************/
	return rc;
}


void static_evaluator_reset_pawn_table( static_evaluator_t* evaluator )
{
	int i;
	/*********************************************/
	for( i = 0; i < PAWN_TABLE_LOCKS; i++ )
		pthread_mutex_lock( evaluator->locks + i );
	/*********************************************/
	for( i = 0; i < PAWN_TABLE_SIZE; i++ )
		evaluator->pawn_table[ i ].key = 0;
	/*********************************************/
	for( i = 0; i < PAWN_TABLE_LOCKS; i++ )
		pthread_mutex_unlock( evaluator->locks + i );
	/*********************************************/

}


static int evaluate_pawns_and_kings( static_evaluator_t* evaluator, board_t* board, int white_mat, int black_mat )
{
	int i;
	int score;
	int king_score;
	int square;
	int pawn_rank[ 2 ][ 10 ];
	int rank;
	int file;
	udword key;
	uqword pawns;
	for( i = 0; i < 10; i++ )
	{
		pawn_rank[ white ][ i ] = 7; 
		pawn_rank[ black ][ i ] = 0;
	}
	key = board->pawn_key & 0xFFFFFFFF;
	if( white_mat < ENDGAME_DEFINITION )
		key ^= 0xFFFF0000;
	if( black_mat < ENDGAME_DEFINITION )
		key ^= 0x0000FFFF;
	if( !read_pawn_table( evaluator, key, &score ) )
	{
		pawns = board->piece_boards[ white ][ pawn ];
		while( pawns != 0x0LL )
		{
			square = bits_remove_first_one( &pawns );
			rank = square / 8;
			file = square % 8;
			if( rank < pawn_rank[ white ][ file+1 ] )
				pawn_rank[ white ][ file+1 ] = rank;
		}
		pawns = board->piece_boards[ black ][ pawn ];
		while( pawns != 0x0LL )
		{
			square = bits_remove_first_one( &pawns );
			rank = square / 8;
			file = square % 8;
			if( rank > pawn_rank[ black ][ file+1 ] )
				pawn_rank[ black ][ file+1 ] = rank;
		}
		score = 0;
		pawns = board->piece_boards[ white ][ pawn ];
		while( pawns != 0x0LL )
		{
			square = bits_remove_first_one( &pawns );
			score += (evaluator->pawn_square_value_weight * white_pawn_square_value[ square ]) / 100;
			rank = square / 8;
			file = square % 8;
			if( (pawn_rank[ black ][ file+1 - 1 ] <= rank) && (pawn_rank[ black ][ file+1 ] <= rank) && (pawn_rank[ black ][ file+1 + 1 ] <= rank) )
				score += evaluator->passed_pawn_bonus * rank;
			if( pawn_rank[ white ][ file+1 ] < rank )
				score -= evaluator->doubled_pawn_penalty;
			if( (pawn_rank[ white ][ file+1 - 1 ] == 7) && (pawn_rank[ white ][ file+1 + 1 ] == 7) )
				score -= evaluator->isolated_pawn_penalty;
			else if( (pawn_rank[ white ][ file+1 - 1 ] < rank) && (pawn_rank[ white ][ file+1 + 1 ] < rank) )
				score -= evaluator->backwards_pawn_penalty;
		}
		pawns = board->piece_boards[ black ][ pawn ];
		while( pawns != 0x0LL )
		{
			square = bits_remove_first_one( &pawns );
			score -= (evaluator->pawn_square_value_weight * black_pawn_square_value[ square ]) / 100; 
			rank = square / 8;
			file = square % 8;
			if( (pawn_rank[ white ][ file+1 - 1 ] >= rank) && (pawn_rank[ white ][ file+1 ] >= rank) && (pawn_rank[ white ][ file+1 + 1 ] >= rank) )
				score -= evaluator->passed_pawn_bonus * (7 - rank);
			if( pawn_rank[ black ][ file+1 ] > rank )
				score += evaluator->doubled_pawn_penalty;
			if( (pawn_rank[ black ][ file+1 - 1 ] == 0) && (pawn_rank[ black ][ file+1 + 1 ] == 0) )
				score += evaluator->isolated_pawn_penalty;
			else if( (pawn_rank[ black ][ file+1 - 1 ] > rank) && (pawn_rank[ black ][ file+1 + 1 ] > rank) )
				score += evaluator->backwards_pawn_penalty;
		}

		king_score = 0;
		square = bits_first_one( board->piece_boards[ white ][ king ] );
		if( black_mat < ENDGAME_DEFINITION )
		{
			king_score += endgame_king_square_value[ square ];
		}
		else
		{
			king_score += white_king_square_value[ square ];
			file = square % 8;
			if( file < 3 )
			{
				king_score += evaluate_white_king_pawn( pawn_rank, 0 );
				king_score += evaluate_white_king_pawn( pawn_rank, 1 );
				king_score += evaluate_white_king_pawn( pawn_rank, 2 ) / 2;
			}
			else if( file > 4 )
			{
				king_score += evaluate_white_king_pawn( pawn_rank, 7 );
				king_score += evaluate_white_king_pawn( pawn_rank, 6 );
				king_score += evaluate_white_king_pawn( pawn_rank, 5 ) / 2;
			}
		}

		square = bits_first_one( board->piece_boards[ black ][ king ] );
		if( white_mat < ENDGAME_DEFINITION )
		{
			king_score -= endgame_king_square_value[ square ];
		}
		else
		{
			king_score -= black_king_square_value[ square ];
			file = square % 8;
			if( file < 3 )
			{
				king_score -= evaluate_black_king_pawn( pawn_rank, 0 );
				king_score -= evaluate_black_king_pawn( pawn_rank, 1 );
				king_score -= evaluate_black_king_pawn( pawn_rank, 2 ) / 2;
			}
			else if( file > 4 )
			{
				king_score -= evaluate_black_king_pawn( pawn_rank, 7 );
				king_score -= evaluate_black_king_pawn( pawn_rank, 6 );
				king_score -= evaluate_black_king_pawn( pawn_rank, 5 ) / 2;
			}
		}
		king_score = (evaluator->king_square_value_weight * king_score) / 100;
		score += king_score;
		write_pawn_table( evaluator, key, score );
	}
	return score;
}

static int evaluate_white_king_pawn( int pawn_rank[ 2 ][ 10 ], int file )
{
	int king_score;
	king_score = 0;
	if( pawn_rank[ white ][ file+1 ] == 1 )
		NULL;
	else if( pawn_rank[ white ][ file+1 ] == 2 )
		king_score -= 10;
	else if( pawn_rank[ white ][ file+1 ] != 7 )
		king_score -= 20;
	else
		king_score -= 25;
	if( pawn_rank[ black ][ file+1 ] == 0 )
		king_score -= 15;
	else if( pawn_rank[ black ][ file+1 ] == 2 )
		king_score -= 10;
	else if( pawn_rank[ black ][ file+1 ] == 3 )
		king_score -= 5;
	return king_score;
}

static int evaluate_black_king_pawn( int pawn_rank[ 2 ][ 10 ], int file )
{
	int king_score;
	king_score = 0;
	if( pawn_rank[ black ][ file+1 ] == 6 )
		NULL;
	else if( pawn_rank[ black ][ file+1 ] == 5 )
		king_score -= 10;
	else if( pawn_rank[ black ][ file+1 ] != 0 )
		king_score -= 20;
	else
		king_score -= 25;
	if( pawn_rank[ white ][ file+1 ] == 7 )
		king_score -= 15;
	else if( pawn_rank[ white ][ file+1 ] == 5 )
		king_score -= 10;
	else if( pawn_rank[ white ][ file+1 ] == 4 )
		king_score -= 5;
	return king_score;
}

