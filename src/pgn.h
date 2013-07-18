
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
