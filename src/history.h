
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

#ifndef __HISTORY_H
#define __HISTORY_H

#include "board.h"
#include "params.h"

typedef struct
{
	int move_history[ 64 ][ 64 ];
	double branch_factor;
	int update_increments[ MAX_DEPTH ];
	move_t killer1[ MAX_DEPTH ];
	move_t killer2[ MAX_DEPTH ];
} history_t;

void history_create( history_t*, double );
void history_destroy( history_t* );
void history_update( history_t*, move_t, bool, int, int, bool );
int history_move_history( history_t*, move_t, int );
move_t history_killer1( history_t* history, int ply );
move_t history_killer2( history_t* history, int ply );


#endif



