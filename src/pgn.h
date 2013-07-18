#ifndef __PGN_H
#define __PGN_H

#include "board.h"

#define max_tags           64

typedef struct
{
	char event[ 256 ];
	char site[ 256 ];
	char white_name[ 256 ];
	char black_name[ 256 ];
	int white_elo;
	int black_elo;
	int year;
	int month;
	int day;
	int round;
	result_t result;
	move_t moves[ max_half_moves ];
	int move_cnt;
	char* stream;
	char* stream_ptr;
	char eco[ 4 ];
} pgn_t;


bool load_pgn( pgn_t*, const char*, board_t* );
bool next_pgn( pgn_t*, board_t* );
void init_pgn( pgn_t* );
void destroy_pgn( pgn_t* );


#endif
