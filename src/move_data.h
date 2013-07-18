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

