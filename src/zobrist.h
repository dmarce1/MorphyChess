
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
