
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

#ifndef __BOARD_H
#define __BOARD_H

#include <stdio.h>
#include "bool.h"
#include "integers.h"

#define initial_position   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define empty              0
#define pawn               1
#define knight             2
#define bishop             3
#define rook               4
#define queen              5
#define king               6
#define white              1
#define black              0
#define A1                 0
#define B1                 1
#define C1                 2
#define D1                 3
#define E1                 4
#define F1                 5
#define G1                 6
#define H1                 7
#define A8                 56
#define B8                 57
#define C8                 58
#define D8                 59
#define E8                 60
#define F8                 61
#define G8                 62 
#define H8                 63
#define max_moves          128
#define max_half_moves     1024
#define file_a             0x0101010101010101LL
#define file_b             0x0202020202020202LL
#define file_c             0x0404040404040404LL
#define file_d             0x0808080808080808LL
#define file_e             0x1010101010101010LL
#define file_f             0x2020202020202020LL
#define file_g             0x4040404040404040LL
#define file_h             0x8080808080808080LL
#define rank_1             0x00000000000000FFLL
#define rank_2             0x000000000000FF00LL
#define rank_3             0x0000000000FF0000LL
#define rank_4             0x00000000FF000000LL
#define rank_5             0x000000FF00000000LL
#define rank_6             0x0000FF0000000000LL
#define rank_7             0x00FF000000000000LL
#define rank_8             0xFF00000000000000LL
#define white_squares      0xAA55AA55AA55AA55LL
#define black_squares      (~(white_squares))


#define color_flip( c )        (((c) == white) ? black : white)
#define move_set_empty( m )    ((*(m)).promo = 1)
#define move_is_empty( l )     ((l).promo == 1)
#define move_compare( m1, m2 ) ((bool)(((m1).from==(m2).from)&&((m1).to==(m2).to)&&((m1).promo==(m2).promo)))

typedef char movetext_t[ 10 ];	

typedef enum 
{ 
	WHITE_WIN   = 1,
	BLACK_WIN   = 2,
	DRAW        = 3,
	IN_PROGRESS = 4,
	INVALID     = 5
} result_t;

typedef struct
{
	uword from  : 6;
	uword to    : 6;
	uword promo : 4;
} move_t;

/* 4 bytes */
typedef struct
{
	udword half_move_count     : 10;
	udword ep_capture_square   :  6;
	udword most_recent_capture :  4;
	udword repeat_count        :  2;
	udword check               :  1;
	udword draw                :  1;
	udword side_to_move        :  1;
	udword black_king_side     :  1;
	udword black_queen_side    :  1;
	udword white_king_side     :  1;
	udword white_queen_side    :  1;
} board_flags_t;

typedef struct
{
	uqword position_key;
	uqword pawn_key;
	board_flags_t flags;
	move_t move;
	ubyte _50_move_rule;
} undo_info_t;

typedef struct
{
	undo_info_t   undo_info[ max_half_moves ];
/*	uqword        pin_by_rank[ max_half_moves ];
	uqword        pin_by_file[ max_half_moves ];
	uqword        pin_by_diag1[ max_half_moves ];
	uqword        pin_by_diag2[ max_half_moves ];*/
	sbyte         piece_counts[ 2 ] [ 7 ];
	uqword        piece_boards[ 2 ] [ 7 ];
	sbyte         piece_values[ 64 ];
	uqword        occupied_squares[ 4 ];
	uqword        position_key;
	uqword        pawn_key;
	board_flags_t flags;
	ubyte         has_kside_castled[ 2 ];
	ubyte         has_qside_castled[ 2 ];
	sbyte         _50_move_rule;
} board_t;

#define board_set_search_mode( b ) ((b)->search_mode = true)
#define board_set_rules_mode( b )  ((b)->search_mode = false)

bool     board_king_in_check( const board_t* );
bool     board_king_in_checkmate( board_t*, int );
bool     board_king_in_stalemate( board_t*, int );
bool     board_make_move( board_t*, move_t );
bool     board_undo_move( board_t* );
bool     board_square_under_attack_by( const board_t*, int, int  );
bool     board_is_draw( const board_t* );
bool     board_is_pseudo_legal_move( board_t*, move_t );
bool     board_is_legal_move( board_t*, move_t );
bool     board_load_FEN( board_t*, const char* );
char*    board_move_to_str( char*, board_t*, move_t );
int      board_gen_pseudolegal_moves( const board_t*, move_t* );
int      board_gen_pseudolegal_captures( const board_t*, move_t* );
int      board_gen_pseudolegal_noncaptures( const board_t*, move_t* );
int      board_gen_pseudolegal_promotions( const board_t*, move_t* );
int      board_pseudolegal_piece_move_count( const board_t*, int );
move_t   board_make_text_move( board_t* b, movetext_t mt );
move_t   str_to_move( char* );
result_t board_result( board_t* r);
void     board_create( board_t* );
void     board_copy( board_t*, const board_t* );
void     board_destroy( board_t* );
void     board_to_stream( const board_t*, FILE* );
int      board_possible_attacks_to_square_by_color( const board_t*, int, int );
bool     board_position_is_repeat( board_t* );
bool     board_last_move_was_capture( board_t* );
bool     board_move_is_capture( board_t* board, move_t move );
bool     board_next_move_is_forced( board_t* board );
int      board_repetitions( board_t* board );
bool     board_gen_LVA_attack_to_square( board_t* board, move_t* move, int square );


#endif
