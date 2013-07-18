
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



