
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
#ifndef MOVE_DATA_H
#define MOVE_DATA_H

#include "integers.h"
#include "bits.h"

uqword white_pawn_attacks( int );
uqword black_pawn_attacks( int );
uqword knight_attacks( int );
uqword bishop_attacks( const uqword*, int );
uqword rook_attacks( const uqword*, int );
uqword queen_attacks( const uqword*, int );
uqword rank_moves( const uqword*, int );
uqword file_moves( const uqword*, int );
uqword diag1_moves( const uqword*, int );
uqword diag2_moves( const uqword*, int );
uqword king_attacks( int );
void init_move_data( void );

#endif

