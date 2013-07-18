#ifndef __ZOBRIST_H
#define __ZOBRIST_H

#include "board.h"

#define REMOVE_REPEAT_KEY ~(0x0000000000000003LL)

uqword zobrist_key( int, int );
uqword zobrist_repeat_key( int );
uqword zobrist_king_side_castle_key( int );
uqword zobrist_queen_side_castle_key( int );
uqword zobrist_ep_key( int );
uqword compute_position_key( board_t* );
uqword compute_pawn_key( board_t* );
void init_zobrist_keys( void );

#endif
