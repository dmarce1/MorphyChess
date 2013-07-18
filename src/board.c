
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "bits.h"
#include "board.h"
#include "debug.h"
#include "move_data.h"
#include "node.h"
#include "zobrist.h"

#define init_undo_size     128
#define w_pawn             pawn
#define w_knight           knight
#define w_bishop           bishop
#define w_rook             rook
#define w_queen            queen
#define w_king             king
#define b_pawn             -pawn
#define b_knight           -knight
#define b_bishop           -bishop
#define b_rook             -rook
#define b_queen            -queen
#define b_king             -king
#define no_ep_capture      0

#define store_move( f, t, p )   \
	{                           \
		m_list->from = (f);     \
		m_list->to = (t);       \
		m_list->promo = (p);    \
		m_list++;               \
	}


static const uqword may_affect_castle[ 2 ] = { 0x9100000000000000LL, 0x0000000000000091LL };

bool check_for_draw( board_t* );
bool king_in_check( const board_t*, int );
static bool is_en_passant_move( board_t*, move_t );
static bool is_castle_move( board_t*, move_t );
static void adjust_repeat_count( board_t* );
static bool draw_by_material( const board_t* );
static int char_to_piece( int );
static undo_info_t get_undo_info( board_t* b );
static void debug_board_to_stream( const board_t*, FILE* );
static void handle_castle( board_t*, move_t );
static void handle_castle_rights_changes( board_t*, move_t );
static void handle_castle_undo( board_t*, move_t );
static void handle_promotion_undo( board_t*, move_t );
static void handle_en_passant_undo( board_t*, move_t );
static void handle_pawn_promotion( board_t*, move_t );
static void handle_en_passant( board_t*, move_t );
static void store_undo_info( board_t* b, undo_info_t* undo );
static int board_gen_pseudolegal_under_promotions( const board_t*, move_t* );
static board_find_king_pins( board_t* b );


bool check_for_draw( board_t* b )
{
	bool rc;

	rc = false;
	if( b->_50_move_rule >= 100 )
	{
		rc = true;
	}
	else if( b->_50_move_rule == 0 )
	{
		rc = draw_by_material( b );
	}
	else
	{
		adjust_repeat_count( b );
		if( b->flags.repeat_count >= 2 )
			rc = true;
	}
	return rc;
}



static bool board_is_pseudo_legal_move_debug( board_t* b, move_t m )
{
	int rc;
	move_t list[ max_moves ];
	move_t* list_ptr = list;
	board_gen_pseudolegal_moves( b, list );
	rc = false;
	while( !move_is_empty( *list_ptr ) )
	{
		if( m.from == list_ptr->from && m.to == list_ptr->to && m.promo == list_ptr->promo )
				rc = true;
		list_ptr++;
	}
	return rc;
}



bool board_is_pseudo_legal_move( board_t* b, move_t m )
{
	bool legal;
	int from_man, moving_color, to, from;
	uqword own_men, to_board;
	legal = false;
	from_man = b->piece_values[ m.from ];
	moving_color = b->flags.side_to_move;
	own_men = b->piece_boards[ moving_color ][ 0 ];
	to_board = 0LL;
	to = m.to;
	from = m.from;
	if( move_is_empty( m ) )
		return false;
	if( (((moving_color == white) && (from_man > 0)) || ((moving_color == black) && (from_man < 0))) && (move_is_empty( m ) == false) )
	{
		switch( abs( from_man ) )
		{
		case pawn:
			if( moving_color == white )
			{
				if( is_en_passant_move( b, m ) )
				{
					if( to == b->flags.ep_capture_square + 8 )
						legal = true;
				}
				else if( b->piece_values[ to ] == 0 )
				{
					if( to - from == 8 )
						legal = true;
					else if( (to - from == 16) && (b->piece_values[ to - 8 ] == 0) && (from / 8 == 1) )
						legal = true;
				}
				else
				{
					to_board = white_pawn_attacks( from ) & b->piece_boards[ black ][ 0 ];
				}
			}
			else /* moving_color == black*/
			{
				if( is_en_passant_move( b, m ) )
				{
					if( to == b->flags.ep_capture_square - 8 )
						legal = true;
				}
				else if( b->piece_values[ to ] == 0 )
				{
					if( to - from == -8 )
						legal = true;
					else if( (to - from == -16) && (b->piece_values[ to + 8 ] == 0) && (from / 8 == 6) )
						legal = true;
				}
				else
				{
					to_board = black_pawn_attacks( from ) & b->piece_boards[ white ][ 0 ];
				}
			}
			break;
		case knight:
			to_board = knight_attacks( from ) & ~own_men;
			break;
		case bishop:
			to_board = bishop_attacks( b->occupied_squares, from ) & ~own_men;
			break;
		case rook:
			to_board = rook_attacks( b->occupied_squares, from ) & ~own_men;
			break;
		case queen:
			to_board = queen_attacks( b->occupied_squares, from ) & ~own_men;
			break;
		case king:
			if( (from == 4) && (to == 6) && (moving_color == white) && (b->flags.white_king_side == 1) )
			{
				to_board = (b->occupied_squares[ 0 ] & (uqword)0x0000000000000060LL);
				if( to_board == 0x0LL )
				{
					if( !board_square_under_attack_by( b, F1, black ) && !board_square_under_attack_by( b, E1, black ) )
						legal = true;
				}
				to_board = 0x0LL;
			}
			else if( (from == 60) && (to == 62) && (moving_color == black) && (b->flags.black_king_side == 1) )
			{
				to_board = (b->occupied_squares[ 0 ] & (uqword)0x6000000000000000LL);
				if( to_board == 0x0LL )
				{
					if( !board_square_under_attack_by( b, F8, white ) && !board_square_under_attack_by( b, E8, white ) )
						legal = true;
				}
				to_board = 0x0LL;
			}
			else if( (from == 4) && (to == 2) && (moving_color == white) && (b->flags.white_queen_side == 1) )
			{
				to_board = (b->occupied_squares[ 0 ] & 0x000000000000000ELL);
				if( to_board == 0x0LL )
				{
					if( !board_square_under_attack_by( b, D1, black ) && !board_square_under_attack_by( b, E1, black ) )
						legal = true;
				}
				to_board = 0x0LL;
			}
			else if( (from == 60) && (to == 58) && (moving_color == black) && (b->flags.black_queen_side == 1) )
			{
				to_board = (b->occupied_squares[ 0 ] & (uqword)0x0E00000000000000LL);
				if( to_board == 0x0LL )
				{
					if( !board_square_under_attack_by( b, D8, white ) && !board_square_under_attack_by( b, E8, white ) )
						legal = true;
				}
				to_board = 0x0LL;
			}
			else
			{
				to_board = king_attacks( from ) & ~own_men;
			}
			break;
		default:
			assert( false );
		}
		if( legal == false )
		{
			if( bits_test( to_board, to ) == true )
				legal = true;
		}
		if( legal == true )
		{
			if( (abs( from_man ) == pawn) && (((to / 8) == 0) || ((to / 8) == 7)) )
				if( (m.promo < knight) || (m.promo > queen) )
					legal = false;
			else
				if( m.promo != 0 )
					legal = false;
		}
	}
	assert( legal == board_is_pseudo_legal_move_debug( b, m ) );
	return legal;
}



bool board_is_legal_move( board_t* b, move_t m )
{
	int rc;
	move_t list[ max_moves ];
	move_t* list_ptr = list;
	board_gen_pseudolegal_moves( b, list );
	rc = false;
	while( !move_is_empty( *list_ptr ) )
	{
		if( m.from == list_ptr->from && m.to == list_ptr->to && m.promo == list_ptr->promo )
		{
			if( board_make_move( b, m ) )
			{
				board_undo_move( b );
				rc = true;
			}
			break;
		}
		list_ptr++;
	}
	return rc;
}


bool board_king_in_checkmate( board_t* b, int color )
{
	move_t* moves;
	int i, cnt;
	bool rc;

	moves = (move_t*) malloc( sizeof( move_t ) * max_moves );
	if( moves == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	
	rc = king_in_check( b, color );
	if( rc )
	{
		cnt = board_gen_pseudolegal_moves( b, moves );
		for( i = 0; i < cnt; i++ )
		{
			if( board_make_move( b, moves[ i ] ) )
			{
				board_undo_move( b );
				rc = false;
				break;
			}
		}
	}
	free( moves );
	return rc;
}

bool board_king_in_stalemate( board_t* b, int color )
{
	move_t* moves;
	int i, cnt;
	bool rc;

	moves = (move_t*) malloc( sizeof( move_t ) * max_moves );
	if( moves == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	rc = king_in_check( b, color );
	if( !rc )
	{
		rc = true;
		cnt = board_gen_pseudolegal_moves( b, moves );
		for( i = 0; i < cnt; i++ )
		{
			if( board_make_move( b, moves[ i ] ) )
			{
				board_undo_move( b );
				rc = false;
				break;
			}
		}
	}
	else
	{
		rc = false;
	}
	free( moves );
	return rc;
}


bool king_in_check( const board_t* b, int color )
{
	bool rc;
	int square;
	uqword bb;
	bb = b->piece_boards[ color ][ king ];
	rc = false;
	assert( bb != 0LL );
	square = bits_first_one( bb );
	rc = board_square_under_attack_by( b, square, color_flip( color ) );
	return rc;
}


bool board_king_in_check( const board_t* b )
{
	bool rc;
	rc = (b->flags.check == 1) ? true : false;
	return rc;
}


bool board_is_draw( const board_t* b )
{
	bool rc;
	rc = (b->flags.draw == 1) ? true : false;
	return rc;
}


bool board_load_FEN( board_t* b, const char* _fen )
{
	char fen[ 256 ];
	char temp_str[ 32 ];
	int row, col, n, i, j;
	char* ptr;
	bool rc;

	strcpy( fen, _fen );
	ptr = fen;
	col = 0;
	row = 7;
	ptr = fen;
	for( i = empty; i <= king; i++ )
	{
		b->piece_counts[ white ][ i ] = 0;
		b->piece_counts[ black ][ i ] = 0;
		b->piece_boards[ white ][ i ] = 0LL;
		b->piece_boards[ black ][ i ] = 0LL;
		for( j = 0; j < 4; j++ )
		{
			b->occupied_squares[ j ] = 0LL;
		}
	}

	while( !((row == 0) && (col == 8)) )
	{
		if( col == 8 )
		{
			col = 0;
		}
		i = 8 * row + col;
		if( *ptr == '/' )
		{
			row--;
		}
		else if( isdigit( *ptr ) )
		{
			n = *ptr - '0';
			while( n > 0 )
			{
				b->piece_values[ i ] = empty;
				bits_clear( &b->piece_boards[ white ][ 0 ], i );
				bits_clear( &b->piece_boards[ black ][ 0 ], i );
				rotated_bits_clear( b->occupied_squares, i );
				col++;
				n--;
				i = 8 * row + col;
			}
		}
		else if( isalpha( *ptr ) )
		{
			switch( *ptr ) 
			{
			case 'p':
				b->piece_values[ i ] = b_pawn;
				bits_set( &b->piece_boards[ black ][ pawn ], i );
				b->piece_counts[ black ][ pawn ]++;
				break;
			case 'n':
				b->piece_values[ i ] = b_knight;
				bits_set( &b->piece_boards[ black ][ knight ], i );
				b->piece_counts[ black ][ knight ]++;
				break;
			case 'b':
				b->piece_values[ i ] = b_bishop;
				bits_set( &b->piece_boards[ black ][ bishop ], i );
				b->piece_counts[ black ][ bishop ]++;
				break;
			case 'r':
				b->piece_values[ i ] = b_rook;
				bits_set( &b->piece_boards[ black ][ rook ], i );
				b->piece_counts[ black ][ rook ]++;
				break;
			case 'q':
				b->piece_values[ i ] = b_queen;
				bits_set( &b->piece_boards[ black ][ queen ], i );
				b->piece_counts[ black ][ queen ]++;
				break;
			case 'k':
				b->piece_values[ i ] = b_king;
				bits_set( &b->piece_boards[ black ][ king ], i );
				b->piece_counts[ black ][ king ]++;
				break;
			case 'P':
				b->piece_values[ i ] = w_pawn;
				bits_set( &b->piece_boards[ white ][ pawn ], i );
				b->piece_counts[ white ][ pawn ]++;
				break;
			case 'N':
				b->piece_values[ i ] = w_knight;
				bits_set( &b->piece_boards[ white ][ knight ], i );
				b->piece_counts[ white ][ knight ]++;
				break;
			case 'B':
				b->piece_values[ i ] = w_bishop;
				bits_set( &b->piece_boards[ white ][ bishop ], i );
				b->piece_counts[ white ][ bishop ]++;
				break;
			case 'R':
				b->piece_values[ i ] = w_rook;
				bits_set( &b->piece_boards[ white ][ rook ], i );
				b->piece_counts[ white ][ rook ]++;
				break;
			case 'Q':
				b->piece_values[ i ] = w_queen;
				bits_set( &b->piece_boards[ white ][ queen ], i );
				b->piece_counts[ white ][ queen ]++;
				break;
			case 'K':
				b->piece_values[ i ] = w_king;
				bits_set( &b->piece_boards[ white ][ king ], i );
				b->piece_counts[ white ][ king ]++;
				break;
			default:
				assert( 0 );
			}
			rotated_bits_set( b->occupied_squares, i );
			if( isupper( *ptr ) )
			{
				b->piece_counts[ white ][ 0 ]++;
				bits_set( &b->piece_boards[ white ][ 0 ], i );
			}
			else
			{
				b->piece_counts[ black ][ 0 ]++;
				bits_set( &b->piece_boards[ black ][ 0 ], i );
			}
			col++;
		}
		else if( !((row == 0) && (col == 8)) )
		{
			printf( "%s\n", ptr );
			assert( false );
		}
		ptr++;
	} 

	while( isspace( *ptr ) )
		ptr++;

	if( *ptr == 'w' )
	{
		b->flags.side_to_move = white;
	}
	else if( *ptr == 'b' )
	{
		b->flags.side_to_move = black;
	}
	else
	{
		assert( false );
	}
	ptr++;

	while( isspace( *ptr ) )
		ptr++;

	b->flags.white_queen_side = b->flags.white_king_side = 0;
	b->flags.black_queen_side = b->flags.black_king_side = 0;

	if( *ptr != '-' )
	{
		while( isalpha( *ptr ) )
		{
			switch( *ptr )
			{
			case 'K':
				b->flags.white_king_side = 1;
				break;
			case 'Q':
				b->flags.white_queen_side = 1;
				break;
			case 'k':
				b->flags.black_king_side = 1;
				break;
			case 'q':
				b->flags.black_queen_side = 1;
				break;
			default:
				assert( false );
			}
			ptr++;
		}
	}
	else
	{
		ptr++;
	}

	while( isspace( *ptr ) )
		ptr++;

	if( *ptr != '-' )
	{
		col = *ptr - 'a';
		row = *(ptr+1) - '1';
		ptr += 2 ;
		i = 8 * row + col;
		if( b->flags.side_to_move == white )
		{
			b->flags.ep_capture_square = i - 8;
		}
		else
		{
			b->flags.ep_capture_square = i + 8;
		}
	}
	else
	{
		b->flags.ep_capture_square = 0;
		ptr++;
	}

	while( isspace( *ptr ) )
		ptr++;

	i = 0;
	while( isdigit( *ptr ) )
	{
		temp_str[ i ] = *ptr;
		i++;
		ptr++;
	}
	temp_str[ i ] = '\0';
	b->_50_move_rule = atoi( temp_str );

	b->flags.half_move_count = 0;	
	b->flags.most_recent_capture = 0;
	rc = (bool) ((b->piece_counts[ white ][ king ] == 1) && (b->piece_counts[ black ][ king ] == 1));
	if( king_in_check( b, b->flags.side_to_move ) )
		b->flags.check = 1;
	else
		b->flags.check = 0;
	if( check_for_draw( b ) )
		b->flags.draw = 1;
	else
		b->flags.draw = 0;
	b->flags.repeat_count = 0;
	b->position_key = compute_position_key( b );
	b->pawn_key = compute_pawn_key( b );
	assert( b->piece_counts[ black ][ king ] == 1 );
	assert_board_consistency( b );
	return rc;
}


bool board_next_move_is_forced( board_t* board )
{
	bool rc;
	move_t list[ max_moves ];
	int i, cnt, legal_cnt;
	
	cnt = board_gen_pseudolegal_moves( board, list );
	legal_cnt = 0;
	for( i = 0; i < cnt; i++ )
	{
		if( board_make_move( board, list[ i ] ) )
		{
			legal_cnt++;
			board_undo_move( board );
		}
	}
	if( legal_cnt > 1 )
		rc = false;
	else
		rc = true;
	return rc;
}


bool board_make_move( board_t* b, move_t move )
{
	int moving_piece;
	int captured_piece;
	int moving_color;
	int opposing_color;
	bool rc;
	bool is_ep, is_castle;

	assert( b != NULL );
	is_castle = is_castle_move( b, move );
	is_ep = is_en_passant_move( b, move );
	undo_info_t undo;
	undo.position_key = b->position_key;
	undo.pawn_key = b->pawn_key;
	undo.move = move;
	undo.flags = b->flags;
	undo._50_move_rule = b->_50_move_rule;
	store_undo_info( b, &undo );
	captured_piece = abs( b->piece_values[ move.to ] );
	moving_piece = abs( b->piece_values[ move.from ] );	
	moving_color = b->flags.side_to_move == white;
	opposing_color = color_flip( moving_color );
	b->_50_move_rule++;
	if( !move_is_empty( move ) )
	{
		bits_clear( &b->piece_boards[ moving_color ][ 0 ], move.from );
		bits_clear( &b->piece_boards[ moving_color ][ moving_piece ], move.from );
		rotated_bits_clear( b->occupied_squares, move.from );
		bits_set( &b->piece_boards[ moving_color ][ 0 ], move.to );
		bits_set( &b->piece_boards[ moving_color ][ moving_piece ], move.to );
		rotated_bits_set( b->occupied_squares, move.to );
		b->position_key ^= zobrist_key( b->piece_values[ move.from ], move.to );	
		b->position_key ^= zobrist_key( b->piece_values[ move.from ], move.from );	
		if( (abs( b->piece_values[ move.from ] ) == pawn) || (abs( b->piece_values[ move.from ] ) == king) )
		{
			b->pawn_key ^= zobrist_key( b->piece_values[ move.from ], move.to );	
			b->pawn_key ^= zobrist_key( b->piece_values[ move.from ], move.from );	
		}
		if( captured_piece != 0 )
		{
			b->_50_move_rule = 0;
			bits_clear( &b->piece_boards[ opposing_color ][ 0 ], move.to );
			bits_clear( &b->piece_boards[ opposing_color ][ captured_piece ], move.to );
			b->piece_counts[ opposing_color ][ 0 ]--;
			b->piece_counts[ opposing_color ][ captured_piece ]--;
			b->flags.most_recent_capture = captured_piece;
			b->position_key ^= zobrist_key( b->piece_values[ move.to ], move.to );	
			if( captured_piece == pawn )
				b->pawn_key ^= zobrist_key( b->piece_values[ move.to ], move.to );	
		}
		else
		{
			b->flags.most_recent_capture = empty;
		}
		b->piece_values[ move.to ] = b->piece_values[ move.from ];
		b->piece_values[ move.from ] = 0;
	
		if( moving_piece == pawn )
			b->_50_move_rule = 0;

		if( is_castle )
			handle_castle( b, move );
		else if( (may_affect_castle[ moving_color ] & bits_mask( move.from ) ) | 
	   		((may_affect_castle[ opposing_color ] & bits_mask( move.to ) ) != 0LL) )
			handle_castle_rights_changes( b, move );

		if( move.promo != 0 )
			handle_pawn_promotion( b, move );
		else if( is_ep )
			handle_en_passant( b, move );

	}
	if( b->flags.ep_capture_square != 0 )
	{
		b->position_key ^= zobrist_ep_key( b->flags.ep_capture_square );
		b->pawn_key ^= zobrist_ep_key( b->flags.ep_capture_square );
		b->flags.ep_capture_square = no_ep_capture;
	}
	if( (moving_piece == pawn) && (abs( move.from - move.to ) == 16) && !move_is_empty( move ) )
	{
		b->flags.ep_capture_square = move.to;
		b->position_key ^= zobrist_ep_key( b->flags.ep_capture_square );
		b->pawn_key ^= zobrist_ep_key( b->flags.ep_capture_square );
	}
	b->flags.side_to_move = opposing_color;
	b->flags.half_move_count++;
	b->flags.check = king_in_check( b, opposing_color ) ? 1 : 0;
/*	b->position_key ^= zobrist_repeat_key( b->flags.repeat_count );*/
	b->flags.draw =  check_for_draw( b ) ? 1 : 0;
/*	b->position_key ^= zobrist_repeat_key( b->flags.repeat_count );*/
	if( !king_in_check( b, color_flip( b->flags.side_to_move ) ) )
	{
		rc = true;
	}
	else	
	{
		board_undo_move( b );
		rc = false;
	}
//	board_find_king_pins( b );
	assert_board_consistency( b );
	return rc;
}


bool board_square_under_attack_by( const board_t* b, int square, int color )
{
	uqword bb1, bb2;
	if( color == black )
	{

		bb2 = b->piece_boards[ black ][ pawn ] & white_pawn_attacks( square );
		bb2 |= b->piece_boards[ black ][ knight ] & knight_attacks( square );
		bb2 |= b->piece_boards[ black ][ king ] & king_attacks( square );

		bb1 = b->piece_boards[ black ][ queen ] | b->piece_boards[ black ][ bishop ];
		bb2 |= bb1 & bishop_attacks( b->occupied_squares, square );

		bb1 = b->piece_boards[ black ][ queen ] | b->piece_boards[ black ][ rook ];
		bb2 |= bb1 & rook_attacks( b->occupied_squares, square );
	}
	else
	{
		bb2 = b->piece_boards[ white ][ pawn ] & black_pawn_attacks( square );
		bb2 |= b->piece_boards[ white ][ knight ] & knight_attacks( square );
		bb2 |= b->piece_boards[ white ][ king ] & king_attacks( square );

		bb1 = b->piece_boards[ white ][ queen ] | b->piece_boards[ white ][ bishop ];
		bb2 |= bb1 & bishop_attacks( b->occupied_squares, square );

		bb1 = b->piece_boards[ white ][ queen ] | b->piece_boards[ white ][ rook ];
		bb2 |= bb1 & rook_attacks( b->occupied_squares, square );
	}
	return (bool) (bb2 != 0LL);
}

bool board_undo_move( board_t* b )
{
	int moving_piece, captured_piece, moving_color, opposing_color, rc;
	move_t move;
	undo_info_t undo;
	bool need_castle_undo;

	if( b->flags.half_move_count != 0 )
	{
		rc = true;
		captured_piece = b->flags.most_recent_capture;
		undo = get_undo_info( b );
		b->flags = undo.flags;
		b->_50_move_rule = undo._50_move_rule;
		b->position_key = undo.position_key;
		b->pawn_key = undo.pawn_key;
		move = undo.move;
		if( !move_is_empty( move ) )
		{
			opposing_color = color_flip( b->flags.side_to_move );;
			moving_color = b->flags.side_to_move;
			moving_piece = abs( b->piece_values[ move.to ] );
			bits_set( &b->piece_boards[ moving_color ][ 0 ], move.from );
			bits_set( &b->piece_boards[ moving_color ][ moving_piece ], move.from );
			rotated_bits_set( b->occupied_squares, move.from );
			bits_clear( &b->piece_boards[ moving_color ][ 0 ], move.to );
			bits_clear( &b->piece_boards[ moving_color ][ moving_piece ], move.to );
			rotated_bits_clear( b->occupied_squares, move.to );
			b->piece_values[ move.from ] = b->piece_values[ move.to ];
			b->piece_values[ move.to ] = 0;
			if( captured_piece != 0 )
			{
				bits_set( &b->piece_boards[ opposing_color ][ 0 ], move.to );
				bits_set( &b->piece_boards[ opposing_color ][ captured_piece ], move.to );
				rotated_bits_set( b->occupied_squares, move.to );
				b->piece_counts[ opposing_color ][ 0 ]++;
				b->piece_counts[ opposing_color ][ captured_piece ]++;
				b->piece_values[ move.to ] = captured_piece * ((opposing_color == white) ? 1 : -1);
			}
			if( is_castle_move( b, move ) )
				handle_castle_undo( b, move );
			else if( move.promo != 0 )
				handle_promotion_undo( b, move );
			else if( is_en_passant_move( b, move ) )
				handle_en_passant_undo( b, move );
		}
	}
	else
	{
		rc = false;
	}
	assert_board_consistency( b );
	return rc;
}


char* board_move_to_str( char* buffer, board_t* b, move_t move )
{
	const char piece_chars[ 7 ] = { '\0', '\0', 'N', 'B', 'R', 'Q', 'K' };
	const int from_rank = move.from / 8;
	const int from_file = move.from % 8;
	const int to_rank = move.to / 8;
	const int to_file = move.to % 8;
	const int piece = abs( b->piece_values[ move.from ] );
	bool is_ep;
	char* start = buffer;
	
	is_ep = is_en_passant_move( b, move );
	if( is_castle_move( b, move ) )
	{
		if( move.from > move.to )
			strcpy( buffer, "O-O-O" );
		else 
			strcpy( buffer, "O-O" );
		buffer += strlen( buffer ); 
	}
	else
	{
		if( piece != pawn )
		{
			*buffer = piece_chars[ piece ];
			buffer++;
		}
		*buffer = 'a' + from_file;
		buffer++;
		*buffer = '1' + from_rank;
		buffer++;
		if( (b->piece_values[ move.to ] != 0) || is_en_passant_move( b, move ) )
		{
			*buffer = 'x';
		}
		else
		{
			*buffer = '-';
		}
		buffer++;
		*buffer = 'a' + to_file;
		buffer++;
		*buffer = '1' + to_rank;
		buffer++;
		if( move.promo != 0 )
		{
			*buffer = '=';
			buffer++;
			*buffer = piece_chars[ move.promo ];
			buffer++;
		}
	}
	
	if( is_ep )
	{
		strcpy( buffer, "e.p." );
		buffer += 4;
	}
	if( board_make_move( b, move ) )
	{
		if( b->flags.check == 1 )
		{
			if( board_king_in_checkmate( b, white ) || board_king_in_checkmate( b, black ) )
				*buffer = '#';
			else
				*buffer = '+';
			buffer++;
		}
		board_undo_move( b );
	}
	*buffer = '\0';
	return start;
}


int board_gen_pseudolegal_moves( const board_t* b, move_t* m_list )
{
	int move_cnt;

	move_cnt = 0;
	move_cnt += board_gen_pseudolegal_promotions( b, m_list + move_cnt );
	move_cnt += board_gen_pseudolegal_captures( b, m_list + move_cnt );
	move_cnt += board_gen_pseudolegal_noncaptures( b, m_list + move_cnt) ;

	return move_cnt;
}


int board_gen_pseudolegal_captures( const board_t* b, move_t* m_list )
{
	const move_t* start_ptr = m_list;
	int attack_color, defend_color, from, to, promo;
	uqword to_board, from_board;
	
	assert( b != NULL );
	assert( m_list != NULL );

	attack_color = b->flags.side_to_move;
	defend_color = color_flip( attack_color );

	if( attack_color == white )
	{
		/* White pawn captures */
		to_board = b->piece_boards[ white ][ pawn ] << 7;
		to_board &= b->piece_boards[ black ][ 0 ];
		to_board &= ~rank_8;
		to_board &= ~file_h;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 7, to, 0 );
		}
		to_board = b->piece_boards[ white ][ pawn ] << 9;
		to_board &= b->piece_boards[ black ][ 0 ];
		to_board &= ~rank_8;
		to_board &= ~file_a;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 9, to, 0 );
		}
		/* ep captures */
		if( b->flags.ep_capture_square != no_ep_capture )
		{
			to = b->flags.ep_capture_square + 8;
			from = to - 7;
			if( (from % 8 != 0) && bits_test( b->piece_boards[ white ][ pawn ], from ) )
			{
				store_move( from, to, 0 );
			}
			from = to - 9;
			if( (from % 8 != 7) && bits_test( b->piece_boards[ white ][ pawn ], from ) )
			{
				store_move( from, to, 0 );
			}
		}
	}
	else
	{
		/********* BLACK PAWN Captures ********************/
		to_board = b->piece_boards[ black ][ pawn ] >> 7;
		to_board &= b->piece_boards[ white ][ 0 ];
		to_board &= ~rank_1;
		to_board &= ~file_a;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 7, to, 0 );
		}
		to_board = b->piece_boards[ black ][ pawn ] >> 9;
		to_board &= b->piece_boards[ white ][ 0 ];
		to_board &= ~rank_1;
		to_board &= ~file_h;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 9, to, 0 );
		}
		/* ep move */
		if( b->flags.ep_capture_square != no_ep_capture )
		{
			to = b->flags.ep_capture_square - 8 ;
			from = to + 7;
			if( (from % 8 != 7) && bits_test( b->piece_boards[ black ][ pawn ], from ) )
			{
				store_move( from, to, 0 );
			}
			from = to + 9;
			if( (from % 8 != 0) && bits_test( b->piece_boards[ black ][ pawn ], from ) )
			{
				store_move( from, to, 0 );
			}
		}
	}
	/********* KNIGHT captures********************/
	from_board = b->piece_boards[ attack_color ][ knight ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = knight_attacks( from ) & b->piece_boards[ defend_color ][ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	/********* BISHOP captures********************/
	from_board = b->piece_boards[ attack_color ][ bishop ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = bishop_attacks( b->occupied_squares, from ) & b->piece_boards[ defend_color ][ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	/********* ROOK captures********************/
	from_board = b->piece_boards[ attack_color ][ rook ];
	promo = 0;
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );	
		to_board = rook_attacks( b->occupied_squares, from ) & b->piece_boards[ defend_color ][ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	from_board = b->piece_boards[ attack_color ][ queen ];
	/********* QUEEN  captures ********************/
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = queen_attacks( b->occupied_squares, from ) & b->piece_boards[ defend_color ][ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	/************ KING captures ************************/
	from_board = b->piece_boards[ attack_color ][ king ];
	from = bits_first_one( from_board );
	to_board = king_attacks( from ) & b->piece_boards[ defend_color ][ 0 ];
	while( to_board != 0LL )
	{
		to = bits_remove_first_one( &to_board );
		store_move( from, to, 0 );
	}
	store_move( 0, 0, 1 );
	return (int) (m_list - start_ptr) - 1;
}


int board_pseudolegal_move_count( const board_t* b, uqword* control, int color )
{
	int attack_color, defend_color, from, promo, count;
	uqword to_board, from_board, temp_board;
	
	assert( b != NULL );

	count = 0;
	attack_color = color;
	defend_color = color_flip( attack_color );
	*control = 0x0000000000000000LL;
	if( attack_color == white )
	{
		to_board = b->piece_boards[ white ][ pawn ] << 8;
		to_board &= ~b->occupied_squares[ 0 ];
		temp_board = to_board;
		count += bits_count_ones( to_board );
		to_board = temp_board << 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_4;
		count += bits_count_ones( to_board );
		to_board = b->piece_boards[ white ][ pawn ] << 7;
		to_board &= ~file_h;
		to_board &= b->piece_boards[ black ][ 0 ];
		*control |= to_board;
		count += bits_count_ones( to_board );
		to_board = b->piece_boards[ white ][ pawn ] << 9;
		to_board &= ~file_a;
		to_board &= b->piece_boards[ black ][ 0 ];
		*control |= to_board;
		count += bits_count_ones( to_board );
	}
	else
	{
		to_board = b->piece_boards[ black ][ pawn ] >> 8;
		to_board &= ~b->occupied_squares[ 0 ];
		temp_board = to_board;
		count += bits_count_ones( to_board );
		to_board = temp_board >> 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_5;
		count += bits_count_ones( to_board );
		to_board = b->piece_boards[ black ][ pawn ] >> 7;
		to_board &= ~file_a;
		to_board &= b->piece_boards[ white ][ 0 ];
		*control |= to_board;
		count += bits_count_ones( to_board );
		to_board = b->piece_boards[ black ][ pawn ] >> 9;
		to_board &= ~file_h;
		to_board &= b->piece_boards[ white ][ 0 ];
		*control |= to_board;
		count += bits_count_ones( to_board );
	}
	from_board = b->piece_boards[ attack_color ][ knight ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = knight_attacks( from );
		*control |= to_board;
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ bishop ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = bishop_attacks( b->occupied_squares, from );
		*control |= to_board;
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ rook ];
	promo = 0;
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );	
		to_board = rook_attacks( b->occupied_squares, from );
		*control |= to_board;
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ queen ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = queen_attacks( b->occupied_squares, from );
		*control |= to_board;
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ king ];
	from = bits_first_one( from_board );
	to_board = king_attacks( from );
	*control |= to_board;
	to_board &= ~b->piece_boards[ attack_color ][ 0 ];
	count += bits_count_ones( to_board );
	return count;
}


int board_pseudolegal_piece_move_count( const board_t* b, int color )
{
	int attack_color, defend_color, from, promo, count;
	uqword to_board, from_board, temp_board;
	
	assert( b != NULL );

	count = 0;
	attack_color = color;
	defend_color = color_flip( attack_color );
	from_board = b->piece_boards[ attack_color ][ knight ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = knight_attacks( from );
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ bishop ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = bishop_attacks( b->occupied_squares, from );
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ rook ];
	promo = 0;
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );	
		to_board = rook_attacks( b->occupied_squares, from );
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ queen ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = queen_attacks( b->occupied_squares, from );
		to_board &= ~b->piece_boards[ attack_color ][ 0 ];
		count += bits_count_ones( to_board );
	}	
	from_board = b->piece_boards[ attack_color ][ king ];
	from = bits_first_one( from_board );
	to_board = king_attacks( from );
	to_board &= ~b->piece_boards[ attack_color ][ 0 ];
	count += bits_count_ones( to_board );
	return count;
}


int board_gen_pseudolegal_noncaptures( const board_t* b, move_t* m_list )
{
	const move_t* start_ptr = m_list;
	int attack_color, defend_color, from, to, promo, cnt;
	uqword to_board, from_board, temp_board;
	
	assert( b != NULL );
	assert( m_list != NULL );

	attack_color = b->flags.side_to_move;
	defend_color = color_flip( attack_color );

	if( attack_color == white )
	{
		/********* WHITE PAWN MOVES ********************/
		to_board = b->piece_boards[ white ][ pawn ] << 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= ~rank_8;
		temp_board = to_board;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 8, to, 0 );
		}
		to_board = temp_board << 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_4;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 16, to, 0 );
		}
		/*********** WHITE CASTLE MOVES ******************************/
		if( b->flags.white_king_side == 1 )
		{
			temp_board = (b->occupied_squares[ 0 ] & (uqword)0x0000000000000060LL);
			if( temp_board  == 0x0LL )
			{
				if( !board_square_under_attack_by( b, F1, black ) && !board_square_under_attack_by( b, E1, black ) )
				{
					store_move( E1, G1, 0 );
				}
			}
		}
		if( b->flags.white_queen_side == 1 )
		{
			temp_board = (b->occupied_squares[ 0 ] & 0x000000000000000ELL);
			if( temp_board == 0x0LL )
			{
				if( !board_square_under_attack_by( b, D1, black ) && !board_square_under_attack_by( b, E1, black ) )
				{
					store_move( E1, C1, 0 );
				}
			}
		}
	}
	else
	{
		/********* BLACK PAWN MOVES ********************/
		assert( attack_color == black );
		to_board = b->piece_boards[ black ][ pawn ] >> 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= ~rank_1;
		temp_board = to_board;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 8, to, 0 );
		}
		to_board = temp_board >> 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_5;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 16, to, 0 );
		}
		/*********** BLACK CASTLE MOVES ******************************/
		if( b->flags.black_king_side == 1 )
		{
			temp_board = (b->occupied_squares[ 0 ] & (uqword)0x6000000000000000LL);
			if( temp_board == 0x0LL )
			{
				if( !board_square_under_attack_by( b, F8, white ) && !board_square_under_attack_by( b, E8, white ) )
				{
					store_move( E8, G8, 0 );
				}
			}
		}
		if( b->flags.black_queen_side == 1 )
		{
			temp_board = (b->occupied_squares[ 0 ] & (uqword)0x0E00000000000000LL);
			if( temp_board == 0x0LL )
			{
				if( !board_square_under_attack_by( b, D8, white ) && !board_square_under_attack_by( b, E8, white ) )
				{
					store_move( E8, C8, 0 );
				}
			}
		}
	}
	/********* KNIGHT MOVES ********************/
	from_board = b->piece_boards[ attack_color ][ knight ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = knight_attacks( from ) & ~b->occupied_squares[ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	/********* BISHOP MOVES ********************/
	from_board = b->piece_boards[ attack_color ][ bishop ];
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = bishop_attacks( b->occupied_squares, from ) & ~b->occupied_squares[ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	/********* ROOK MOVES ********************/
	from_board = b->piece_boards[ attack_color ][ rook ];
	promo = 0;
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );	
		to_board = rook_attacks( b->occupied_squares, from ) & ~b->occupied_squares[ 0 ];
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	from_board = b->piece_boards[ attack_color ][ queen ];
	/********* QUEEN MOVES ********************/
	while( from_board != 0LL )
	{
		from = bits_remove_first_one( &from_board );
		to_board = queen_attacks( b->occupied_squares, from ) & ~b->occupied_squares[ 0 ];;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( from, to, 0 );
		}
	}	
	/************ KING MOVES ************************/
	from_board = b->piece_boards[ attack_color ][ king ];
	from = bits_first_one( from_board );
	to_board = king_attacks( from ) & ~b->occupied_squares[ 0 ];;
	while( to_board != 0LL )
	{
		to = bits_remove_first_one( &to_board );
		store_move( from, to, 0 );
	}
	cnt = (int) (m_list - start_ptr);
	/* under promotions */
	cnt += board_gen_pseudolegal_under_promotions( b, m_list );

	return cnt;	
}


int board_gen_pseudolegal_promotions( const board_t* b, move_t* m_list )
{
	const move_t* start_ptr = m_list;
	int attack_color, defend_color, to;
	uqword to_board, temp_board;
	
	assert( b != NULL );
	assert( m_list != NULL );

	attack_color = b->flags.side_to_move;
	defend_color = color_flip( attack_color );

	if( attack_color == white )
	{
		/* White pawn promotions */
		to_board = b->piece_boards[ white ][ pawn ] << 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_8;
		temp_board = to_board;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 8, to, queen );
		}
		to_board = b->piece_boards[ white ][ pawn ] << 7;
		to_board &= b->piece_boards[ black ][ 0 ];
		to_board &= rank_8;
		to_board &= ~file_h;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 7, to, queen );
		}
		to_board = b->piece_boards[ white ][ pawn ] << 9;
		to_board &= b->piece_boards[ black ][ 0 ];
		to_board &= rank_8;
		to_board &= ~file_a;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 9, to, queen );
		}
	}
	else
	{
		/* Black pawn promotions */
		to_board = b->piece_boards[ black ][ pawn ] >> 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_1;
		temp_board = to_board;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 8, to, queen );
		}
		to_board = b->piece_boards[ black ][ pawn ] >> 7;
		to_board &= b->piece_boards[ white ][ 0 ];
		to_board &= rank_1;
		to_board &= ~file_a;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 7, to, queen );
		}
		to_board = b->piece_boards[ black ][ pawn ] >> 9;
		to_board &= b->piece_boards[ white ][ 0 ];
		to_board &= rank_1;
		to_board &= ~file_h;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 9, to, queen );
		}
	}
	store_move( 0, 0, 1 );
	return (int) (m_list - start_ptr) - 1;
}


static int board_gen_pseudolegal_under_promotions( const board_t* b, move_t* m_list )
{
	const move_t* start_ptr = m_list;
	int attack_color, defend_color, to;
	uqword to_board, temp_board;
	
	assert( b != NULL );
	assert( m_list != NULL );

	attack_color = b->flags.side_to_move;
	defend_color = color_flip( attack_color );

	if( attack_color == white )
	{
		/* White pawn promotions */
		to_board = b->piece_boards[ white ][ pawn ] << 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_8;
		temp_board = to_board;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 8, to, rook );
			store_move( to - 8, to, bishop );
			store_move( to - 8, to, knight );
		}
		to_board = b->piece_boards[ white ][ pawn ] << 7;
		to_board &= b->piece_boards[ black ][ 0 ];
		to_board &= rank_8;
		to_board &= ~file_h;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 7, to, rook );
			store_move( to - 7, to, bishop );
			store_move( to - 7, to, knight );
		}
		to_board = b->piece_boards[ white ][ pawn ] << 9;
		to_board &= b->piece_boards[ black ][ 0 ];
		to_board &= rank_8;
		to_board &= ~file_a;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to - 9, to, rook );
			store_move( to - 9, to, bishop );
			store_move( to - 9, to, knight );
		}
	}
	else
	{
		/* Black pawn promotions */
		to_board = b->piece_boards[ black ][ pawn ] >> 8;
		to_board &= ~b->occupied_squares[ 0 ];
		to_board &= rank_1;
		temp_board = to_board;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 8, to, rook );
			store_move( to + 8, to, bishop );
			store_move( to + 8, to, knight );
		}
		to_board = b->piece_boards[ black ][ pawn ] >> 7;
		to_board &= b->piece_boards[ white ][ 0 ];
		to_board &= rank_1;
		to_board &= ~file_a;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 7, to, rook );
			store_move( to + 7, to, bishop );
			store_move( to + 7, to, knight );
		}
		to_board = b->piece_boards[ black ][ pawn ] >> 9;
		to_board &= b->piece_boards[ white ][ 0 ];
		to_board &= rank_1;
		to_board &= ~file_h;
		while( to_board != 0LL )
		{
			to = bits_remove_first_one( &to_board );
			store_move( to + 9, to, rook );
			store_move( to + 9, to, bishop );
			store_move( to + 9, to, knight );
		}
	}
	store_move( 0, 0, 1 );
	return (int) (m_list - start_ptr) - 1;
}


move_t board_make_text_move( board_t* b, movetext_t mt )
{
	move_t m = { 0, 0, 0 };
	char* ptr;
	ptr = mt;
	m.promo = empty;
	move_t moves[ max_moves ];
	int nmoves, i, piece;
	int fr, tr, ff, tf;
	bool match_found;

	for( i = 0; i < strlen( mt ); i++ )
	{
		if( mt[ i ] == '\n' )
		{
			mt[ i ] = '\0';
			break;
		}
	}
	if( strcmp( mt, "O-O" ) == 0 )
	{
		if( b->flags.side_to_move == white )
		{
			m.from = E1;
			m.to = G1;
		}
		else
		{
			m.from = E8;
			m.to = G8;
		}
		match_found = true;
	}
	else if( strcmp( mt, "O-O-O" ) == 0 )
	{
		if( b->flags.side_to_move == white )
		{
			m.from = E1;
			m.to = C1;
		}
		else
		{
			m.from = E8;
			m.to = C8;
		}
		match_found = true;
	}

	else 
	{
		nmoves = board_gen_pseudolegal_moves( b, moves );
		if( isupper( *ptr ) )
		{
			piece = char_to_piece( *ptr );
			ptr++;
		}
		else
		{
			piece = pawn;
		}
		ff = fr = tf = tr = -1;
		for( i = 0; (i < 5) && (*ptr != '\0') && (*ptr != '='); i++ )
		{
			if( isalpha( *ptr ) && *ptr != 'x' )
			{
				if( tf == -1 )
				{
					tf = *ptr - 'a';
				}
				else
				{
					ff = tf;
					tf = *ptr - 'a';
				}
			}
			else if( isdigit( *ptr ) )
			{
				if( tr == -1 )
				{
					tr = *ptr - '1';
				}
				else
				{
					fr = tr;
					tr = *ptr - '1';
				}
			}
			ptr++;
		}
		if( *ptr == '=' )
		{
			ptr++;
			m.promo = char_to_piece( *ptr );
		}
		else
		{
			m.promo = 0;
		}
		m.to = 8 * tr + tf;
		if( (fr == -1) || (ff == -1) )
		{
			match_found = false;
			for( i = 0; !move_is_empty( moves[ i ] ); i++ )
			{

				if( (moves[ i ].to == m.to) && (abs( b->piece_values[ moves[ i ].from ] ) == piece) )
				{
					if( (fr == -1) && (ff == -1) )
					{
 						m.from = moves[ i ].from;
						match_found = true;
						break;
					}
					if( (ff == -1) && (fr != -1) )
					{
						if( moves[ i ].from / 8 == fr )
						{
							m.from = moves[ i ].from;
							match_found = true;
							break;
						}
					}
					if( (ff != -1) && (fr == -1) )
					{
						if( moves[ i ].from % 8 == ff )
						{
							m.from = moves[ i ].from;
							match_found = true;
							break;
						}
					}
				}
			}
		}
		else
		{
			m.from = 8 * fr + ff;
			match_found = true;
		}
	}
	if( match_found && board_is_legal_move( b, m ) )
	{
		board_make_move( b, m );
	}
	else
	{
		move_set_empty( &m );
	}
	return m;
}


move_t str_to_move( char* str )
{
	move_t move = { 0, 0, 0 };
	int rank, file, from, to, promo;
	if( isupper( *str ) )
	{
		str++;
	}
	file = (str[ 0 ] - 'a');
	rank = (str[ 1 ] - '1');
	from = 8 * rank + file;
	if( str[ 3 ] == 'x' )
	{
		str++;
	}
	file = (str[ 2 ] - 'a');
	rank = (str[ 3 ] - '1');
	to = 8 * rank + file;
	if( str[ 4 ] == '=' )
	{
		switch( str[ 5 ] )
		{
		case 'q':
		case 'Q':
			promo = queen;
			break;
		case 'r':
		case 'R':
			promo = rook;
			break;
		case 'b':
		case 'B':
			promo = bishop;
			break;
		case 'n':
		case 'N':
			promo = knight;
			break;
		default:
			promo = -1;
		}
	}
	else
	{
		promo = 0;
	}
	if( from >= 0 && from < 64 && to >= 0 && to < 64 && promo != -1 )
	{
		move.from = from;
		move.to = to;
		move.promo = promo;
	}
	else
	{
		move_set_empty( &move );
	}
	return move;
}


result_t board_result( board_t* b )
{
	result_t result;
	move_t move;
	result = IN_PROGRESS;

	if( board_is_draw( b ) )
		result = DRAW;
	else if( board_king_in_checkmate( b, white ) )
		result = BLACK_WIN;
	else if( board_king_in_checkmate( b, black ) )
		result = WHITE_WIN;
	else if( board_king_in_stalemate( b, white ) )
		result = DRAW;
	return result;
}


void board_copy( board_t* d, const board_t* s )
{
	int i, j;
	

	for( i = 0; i < 64; i++ )
		d->piece_values[ i ] = s->piece_values[ i ];
	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j <= 6; j++ )
		{
			d->piece_counts[ i ][ j ] = s->piece_counts[ i ][ j ];
			d->piece_boards[ i ][ j ] = s->piece_boards[ i ][ j ];
		}
	}
	d->occupied_squares[ 0 ] = s->occupied_squares[ 0 ];
	d->occupied_squares[ 1 ] = s->occupied_squares[ 1 ];
	d->occupied_squares[ 2 ] = s->occupied_squares[ 2 ];
	d->occupied_squares[ 3 ] = s->occupied_squares[ 3 ];
	for( i = 0; i < max_half_moves; i++ )
		d->undo_info[ i ] = s->undo_info[ i ];
	d->position_key = s->position_key;
	d->pawn_key = s->pawn_key;
	d->flags = s->flags;
	d->_50_move_rule = s->_50_move_rule;
	assert_board_consistency( d );
}


void board_create( board_t* b )
{
	int i;
	assert( b != NULL );
	char fen_position[ 128 ] = initial_position;
	board_load_FEN( b, fen_position );
	for( i = 0; i < 2; i++ )
	{
		b->has_kside_castled[ i ] = 0;
		b->has_qside_castled[ i ] = 0;
	}
}


void board_destroy( board_t* b )
{
}


void board_to_stream( const board_t* b, FILE* fp )
{
	int rank, file, i;
	const char piece_char[ 13 ] = { 'k', 'q', 'r', 'b', 'n', 'o', ' ', 'x', 'N', 'B', 'R', 'Q', 'K' };
	char c;

	assert( b != NULL );
	
	if( debug_mode )
	{
		debug_board_to_stream( b, fp );
	}
	else
	{
		for( rank = 7; rank >= 0; rank-- )
		{
			fprintf( fp, "----------------------------------\n" );
			fprintf( fp, "%i", rank + 1 );
			for( file = 0; file <= 7; file++ )
			{
				i = 8 * rank + file;
				c = piece_char[ b->piece_values[ i ] + 6 ];
				fprintf( fp, "| %c ", c );
			}
			fprintf( fp, "|\n" );
		}
		fprintf( fp, "----------------------------------\n" );
		fprintf( fp, "%c| a | b | c | d | e | f | g | h |\n", (b->flags.side_to_move == white) ? 'W' : 'B' );
	}
	fprintf( fp, "#%i\n", (b->flags.half_move_count / 2) + 1 );
}


static bool draw_by_material( const board_t* b )
{
	bool rc;
	rc = false;
	if( (b->piece_counts[ white ][ 0 ] <= 2 ) && (b->piece_counts[ black ][ 0 ] <= 2) )
	{
		if( (b->piece_counts[ white ][ 0 ] == 1 ) && (b->piece_counts[ black ][ 0 ] == 1) )
		{
			rc = true;
		}
		else if( b->piece_counts[ white ][ 0 ] == 1 )
		{
			if( (b->piece_counts[ black ][ knight ] == 1) || (b->piece_counts[ black ][ bishop ] == 1) )
			{
				rc = true;
			}
		}
		else if( b->piece_counts[ black ][ 0 ] == 1 )
		{
			if( (b->piece_counts[ white ][ knight ] == 1) || (b->piece_counts[ white ][ bishop ] == 1) )
			{
				rc = true;
			}
		}
		else
		{
			if( (b->piece_counts[ white ][ bishop ] == 1) && (b->piece_counts[ black ][ bishop ] == 1) )
			{
				if( !((((b->piece_boards[ white ][ bishop ] & white_squares) != 0LL) && 
				       ((b->piece_boards[ black ][ bishop ] & black_squares) != 0LL)) ||
				      (((b->piece_boards[ white ][ bishop ] & black_squares) != 0LL) &&
				       ((b->piece_boards[ black ][ bishop ] & white_squares) != 0LL))) )
				{
					rc = true;
				}
			}

		}
	}
	return rc;
}


static bool is_castle_move( board_t* b, move_t move )
{
	int rc = 0;

	assert( b != NULL );

	if( (abs( b->piece_values[ move.from ] ) == king) && (abs( move.from - move.to ) == 2) )
		rc = true;
	else
		rc = false;
	return rc;
}


static bool is_en_passant_move( board_t* b, move_t move )
{
	int rc;
	rc = false;
	if( (abs( b->piece_values[ move.from ] ) == pawn) && (b->piece_values[ move.to ] == empty) )
	{
		if( b->flags.side_to_move == white )
		{
			if( ((move.to - move.from) == 7) || ((move.to - move.from) == 9) )
				rc = true;
		}	
		else
		{
			if( ((move.to - move.from) == -7) || ((move.to - move.from) == -9) )
				rc = true;
		}
	}

	return rc;
}

bool board_position_is_repeat( board_t* b )
{
	return (bool) (b->flags.repeat_count > 0);
}

static void adjust_repeat_count( board_t* b )
{
	int i, start, stop;
	uqword this_key;

	b->flags.repeat_count = 0;
	start = b->flags.half_move_count - 4;
	stop = b->flags.half_move_count - b->_50_move_rule;
	this_key = b->position_key/* & REMOVE_REPEAT_KEY*/;
	if( start >= 0 )
	{
		stop = (stop >= 0) ? stop : 0;
		for( i = start; i >= stop; i -= 2 )
		{
			if( this_key == (b->undo_info[ i ].position_key /*& REMOVE_REPEAT_KEY*/) )
			{
				b->flags.repeat_count++;
				if( b->flags.repeat_count >= 2 )
					break;
			}
		}
	}
}


static int char_to_piece( int c )
{
	int piece;
	switch( c )
	{
	case 'K':
		piece = king;
		break;
	case 'Q':
		piece = queen;
		break;
	case 'R':
		piece = rook;
		break;
	case 'B':
		piece = bishop;
		break;
	case 'N':
		piece = knight;
		break;
	default:
		piece = pawn;
		break;
	}
	return piece;
}


static undo_info_t get_undo_info( board_t* b )
{
	undo_info_t u;
	u = b->undo_info[ b->flags.half_move_count - 1 ];
	return u;
}


static void debug_board_to_stream( const board_t* b, FILE* fp )
{
	int rank, file, i, j;
	const char piece_char[ 13 ] = { 'k', 'q', 'r', 'b', 'n', 'p', ' ', 'P', 'N', 'B', 'R', 'Q', 'K' };
	char c;

	assert( b != NULL );
	
	rank = 7;
	fprintf( fp, "----------------------------------\n" );
	fprintf( fp, "%i", rank + 1 );
	for( file = 0; file <= 7; file++ )
	{
		i = 8 * rank + file;
		c = piece_char[ b->piece_values[ i ] + 6 ];
		fprintf( fp, "| %c ", c );
	}
	fprintf( fp, "|            white           black\n" );
	j = 6;
	for( rank = 6; rank > 3; rank-- )
	{
		fprintf( fp, "---------------------------------" );
		fprintf( fp, "| %c: %016llx  %016llx\n", piece_char[ 6 + j ], 
			b->piece_boards[ white ][ j ], b->piece_boards[ black ][ j ] );
		j--;
		fprintf( fp, "%i", rank + 1 );
		for( file = 0; file <= 7; file++ )
		{
			i = 8 * rank + file;
			c = piece_char[ b->piece_values[ i ] + 6 ];
			fprintf( fp, "| %c ", c );
		}
		fprintf( fp, "| %c: %016llx  %016llx\n", piece_char[ 6 + j ], 
			b->piece_boards[ white ][ j ], b->piece_boards[ black ][ j ] );
		j--;
	}
	for( rank = 3; rank >= 0; rank-- )
	{
		fprintf( fp, "---------------------------------" );
		if( rank == 3 )
		{
			fprintf( fp, "| %c: %016llx  %016llx\n", piece_char[ 6 + j ], 
				b->piece_boards[ white ][ j ], b->piece_boards[ black ][ j ] );
		}
		else if( rank == 2 )
		{
			fprintf( fp, "|%016llx %016llx %016llx %016llx\n", b->occupied_squares[ 0 ] , 
				b->occupied_squares[ 1 ] , b->occupied_squares[ 2 ] , b->occupied_squares[ 3 ] );
		}
		else if( rank == 1 )
		{
			printf( "|   %i, %i, %i, %i, %i, %i <> %i, %i, %i, %i, %i, %i\n", 
				b->piece_counts[ white ][ pawn ], b->piece_counts[ white ][ knight ], 
				b->piece_counts[ white ][ bishop ], b->piece_counts[ white ][ rook ], 
				b->piece_counts[ white ][ queen ], b->piece_counts[ white ][ king ],
				b->piece_counts[ black ][ pawn ], b->piece_counts[ black ][ knight ], 
				b->piece_counts[ black ][ bishop ], b->piece_counts[ black ][ rook ], 
				b->piece_counts[ black ][ queen ], b->piece_counts[ black ][ king ] );
		}
		else if( rank == 0 )
		{
			printf( "|          %5i         %5i\n", b->flags.half_move_count, b->_50_move_rule );
		}
		fprintf( fp, "%i", rank + 1 );
		for( file = 0; file <= 7; file++ )
		{
			i = 8 * rank + file;
			c = piece_char[ b->piece_values[ i ] + 6 ];
			fprintf( fp, "| %c ", c );
		}
		fprintf( fp, "|\n" );
	}
	fprintf( fp, "---------------------------------" );
	printf( "|0x%016llx        %i%i%i%i          %i\n", b->position_key, b->flags.white_king_side, b->flags.white_queen_side, 
		b->flags.black_king_side, b->flags.black_queen_side, b->flags.ep_capture_square );
	fprintf( fp, "%c| a | b | c | d | e | f | g | h |    %c\n", 
		(b->flags.side_to_move == white) ? 'W' : 'B', piece_char[ b->flags.most_recent_capture + 6 ] );
}


static void handle_castle( board_t* b, move_t move )
{
	uqword key_change = 0LL;
	if( b->flags.side_to_move == white )
	{
		key_change = b->flags.white_king_side * zobrist_king_side_castle_key( white );
		key_change ^= b->flags.white_queen_side * zobrist_queen_side_castle_key( white );
		b->flags.white_king_side = 
		b->flags.white_queen_side = 0;
		/* King Side */
		if( move.from < move.to )
		{
			rotated_bits_clear( b->occupied_squares, H1 );
			bits_clear( &(b->piece_boards[ white ][ 0 ]), H1 );
			bits_clear( &(b->piece_boards[ white ][ rook ]), H1 );
			b->piece_values[ H1 ] = 0;
			rotated_bits_set( b->occupied_squares, F1 );
			bits_set( &(b->piece_boards[ white ][ 0 ]), F1 );
			bits_set( &(b->piece_boards[ white ][ rook ]), F1 );
			b->piece_values[ F1 ] = rook;
			key_change ^= zobrist_key( rook, H1 );
			key_change ^= zobrist_key( rook, F1 );
			b->has_kside_castled[ white ] = 1;
		}
		/* Qside */
		else
		{
			rotated_bits_clear( b->occupied_squares, A1 );
			bits_clear( &b->piece_boards[ white ][ 0 ], A1 );
			bits_clear( &b->piece_boards[ white ][ rook ], A1 );
			b->piece_values[ A1 ] = 0;
			rotated_bits_set( b->occupied_squares, D1 );
			bits_set( &b->piece_boards[ white ][ 0 ], D1 );
			bits_set( &b->piece_boards[ white ][ rook ], D1 );
			b->piece_values[ D1 ] = rook;
			key_change ^= zobrist_key( rook, A1 );
			key_change ^= zobrist_key( rook, D1 );
			b->has_qside_castled[ white ] = 1;
		}
	}
	else
	{
		key_change = b->flags.black_king_side * zobrist_king_side_castle_key( black );
		key_change ^= b->flags.black_queen_side * zobrist_queen_side_castle_key( black );
		b->flags.black_king_side = 
		b->flags.black_queen_side = 0;
		if( move.from < move.to )
		{
			rotated_bits_clear( b->occupied_squares, H8 );
			bits_clear( &(b->piece_boards[ black ][ 0 ]), H8 );
			bits_clear( &(b->piece_boards[ black ][ rook ]), H8 );
			b->piece_values[ H8 ] = 0;
			rotated_bits_set( b->occupied_squares, F8 );
			bits_set( &(b->piece_boards[ black ][ 0 ]), F8 );
			bits_set( &(b->piece_boards[ black ][ rook ]), F8 );
			b->piece_values[ F8 ] = -rook;
			key_change ^= zobrist_key( -rook, H8 );
			key_change ^= zobrist_key( -rook, F8 );
			b->has_kside_castled[ black ] = 1;
		}
		else
		{
			rotated_bits_clear( b->occupied_squares, A8 );
			bits_clear( &b->piece_boards[ black ][ 0 ], A8 );
			bits_clear( &b->piece_boards[ black ][ rook ], A8 );
			b->piece_values[ A8 ] = 0;
			rotated_bits_set( b->occupied_squares, D8 );
			bits_set( &b->piece_boards[ black ][ 0 ], D8 );
			bits_set( &b->piece_boards[ black ][ rook ], D8 );
			b->piece_values[ D8 ] = -rook;
			key_change ^= zobrist_key( -rook, A8 );
			key_change ^= zobrist_key( -rook, D8 );
			b->has_qside_castled[ black ] = 1;
		}
	}
	b->position_key ^= key_change;
}




static void handle_castle_rights_changes( board_t* b, move_t move )
{
	if( b->flags.side_to_move == white )
	{
		if( move.from == A1 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( white ) * b->flags.white_queen_side;
			b->flags.white_queen_side = 0;
		}
		else if( move.from == E1 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( white ) * b->flags.white_queen_side;
			b->position_key ^= zobrist_king_side_castle_key( white ) * b->flags.white_king_side;
			b->flags.white_queen_side = 0;
			b->flags.white_king_side = 0;
		}
		else if( move.from == H1 )
		{
			b->position_key ^= zobrist_king_side_castle_key( white ) * b->flags.white_king_side;
			b->flags.white_king_side = 0;
		}
		if( move.to == A8 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( black ) * b->flags.black_queen_side;
			b->flags.black_queen_side = 0;
		}
		else if( move.to == E8 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( black ) * b->flags.black_queen_side;
			b->position_key ^= zobrist_king_side_castle_key( black ) * b->flags.black_king_side;
			b->flags.black_queen_side = 0;
			b->flags.black_king_side = 0;
		}
		else if( move.to == H8 )
		{
			b->position_key ^= zobrist_king_side_castle_key( black ) * b->flags.black_king_side;
			b->flags.black_king_side = 0;
		}
	}
	else
	{
		if( move.from == A8 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( black ) * b->flags.black_queen_side;
			b->flags.black_queen_side = 0;
		}
		else if( move.from == E8 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( black ) * b->flags.black_queen_side;
			b->position_key ^= zobrist_king_side_castle_key( black ) * b->flags.black_king_side;
			b->flags.black_queen_side = 0;
			b->flags.black_king_side = 0;
		}
		else if( move.from == H8 )
		{
			b->position_key ^= zobrist_king_side_castle_key( black ) * b->flags.black_king_side;
			b->flags.black_king_side = 0;
		}
		if( move.to == A1 )
		{
			b->position_key ^= zobrist_queen_side_castle_key( white ) *  b->flags.white_queen_side;
			b->flags.white_queen_side = 0;
		}
		else if( move.to == E1 )
		{
			b->position_key ^= zobrist_king_side_castle_key( white ) * b->flags.white_king_side;
			b->position_key ^= zobrist_queen_side_castle_key( white ) * b->flags.white_queen_side;
			b->flags.white_queen_side = 0;
			b->flags.white_king_side = 0;
		}
		else if( move.to == H1)
		{
			b->position_key ^= zobrist_king_side_castle_key( white ) * b->flags.white_king_side;
			b->flags.white_king_side = 0;
		}
	}
}


static void handle_castle_undo( board_t* b, move_t m )
{
	if( b->flags.side_to_move == white )
	{
		if( m.from < m.to )
		{
			rotated_bits_set( b->occupied_squares, H1 );
			bits_set( &b->piece_boards[ white ][ 0 ], H1 );
			bits_set( &b->piece_boards[ white ][ rook ], H1 );
			rotated_bits_clear( b->occupied_squares, F1 );
			bits_clear( &b->piece_boards[ white ][ 0 ], F1 );
			bits_clear( &b->piece_boards[ white ][ rook ], F1 );
			b->piece_values[ F1 ] = empty;
			b->piece_values[ H1 ] = w_rook;
			b->has_kside_castled[ white ] = 0;
		}
		else
		{
			rotated_bits_set( b->occupied_squares, A1 );
			bits_set( &b->piece_boards[ white ][ 0 ], A1 );
			bits_set( &b->piece_boards[ white ][ rook ], A1 );
			rotated_bits_clear( b->occupied_squares, D1 );
			bits_clear( &b->piece_boards[ white ][ 0 ], D1 );
			bits_clear( &b->piece_boards[ white ][ rook ], D1 );
			b->piece_values[ D1 ] = empty;
			b->piece_values[ A1 ] = w_rook;
			b->has_qside_castled[ white ] = 0;
		}
	}
	else
	{
		if( m.from < m.to )
		{
			rotated_bits_set( b->occupied_squares, H8 );
			bits_set( &b->piece_boards[ black ][ 0 ], H8 );
			bits_set( &b->piece_boards[ black ][ rook ], H8 );
			rotated_bits_clear( b->occupied_squares, F8 );
			bits_clear( &b->piece_boards[ black ][ 0 ], F8 );
			bits_clear( &b->piece_boards[ black ][ rook ], F8 );
			b->piece_values[ F8 ] = empty;
			b->piece_values[ H8 ] = b_rook;
			b->has_kside_castled[ black ] = 0;
		}
		else
		{
			rotated_bits_set( b->occupied_squares, A8 );
			bits_set( &b->piece_boards[ black ][ 0 ], A8 );
			bits_set( &b->piece_boards[ black ][ rook ], A8 );
			rotated_bits_clear( b->occupied_squares, D8 );
			bits_clear( &b->piece_boards[ black ][ 0 ], D8 );
			bits_clear( &b->piece_boards[ black ][ rook ], D8 );
			b->piece_values[ D8 ] = empty;
			b->piece_values[ A8 ] = b_rook;
			b->has_qside_castled[ black ] = 0;
		}
	}
}


static void handle_en_passant( board_t* b, move_t move )
{
	if( b->flags.side_to_move == white )
	{
		rotated_bits_clear( b->occupied_squares, move.to - 8 );
		bits_clear( &b->piece_boards[ black ][ 0 ], move.to - 8 );
		bits_clear( &b->piece_boards[ black ][ pawn ], move.to - 8 );
		b->piece_values[ move.to - 8 ] = empty;
		b->piece_counts[ black ][ 0 ]--;
		b->piece_counts[ black ][ pawn ]--;
		b->position_key ^= zobrist_key( b_pawn, move.to - 8 );
		b->pawn_key ^= zobrist_key( b_pawn, move.to - 8 );
	}
	else
	{
		rotated_bits_clear( b->occupied_squares, move.to + 8 );
		bits_clear( &b->piece_boards[ white ][ 0 ], move.to + 8 );
		bits_clear( &b->piece_boards[ white ][ pawn ], move.to + 8 );
		b->piece_values[ move.to + 8 ] = empty;
		b->piece_counts[ white ][ 0 ]--;
		b->piece_counts[ white ][ pawn ]--;
		b->position_key ^= zobrist_key( w_pawn, move.to + 8 );
		b->pawn_key ^= zobrist_key( w_pawn, move.to + 8 );
	}
}


static void handle_en_passant_undo( board_t* b, move_t move )
{
	if( b->flags.side_to_move == white )
	{
		rotated_bits_set( b->occupied_squares, move.to - 8 );
		bits_set( &b->piece_boards[ black ][ 0 ], move.to - 8 );
		bits_set( &b->piece_boards[ black ][ pawn ], move.to - 8 );
		b->piece_values[ move.to - 8 ] = b_pawn;
		b->piece_counts[ black ][ 0 ]++;
		b->piece_counts[ black ][ pawn ]++;
	}
	else
	{
		rotated_bits_set( b->occupied_squares, move.to + 8 );
		bits_set( &b->piece_boards[ white ][ 0 ], move.to + 8 );
		bits_set( &b->piece_boards[ white ][ pawn ], move.to + 8 );
		b->piece_values[ move.to + 8 ] = w_pawn;
		b->piece_counts[ white ][ 0 ]++;
		b->piece_counts[ white ][ pawn ]++;
	}
}


static void handle_pawn_promotion( board_t* b, move_t move )
{
	const int color = b->flags.side_to_move;
	if( color == white )
	{
		b->piece_values[ move.to ] = move.promo;
		b->position_key ^= zobrist_key( pawn, move.to );
		b->pawn_key ^= zobrist_key( pawn, move.to );
		b->position_key ^= zobrist_key( move.promo, move.to );
	}
	else
	{
		b->piece_values[ move.to ] = -move.promo;
		b->position_key ^= zobrist_key( -pawn, move.to );
		b->pawn_key ^= zobrist_key( -pawn, move.to );
		b->position_key ^= zobrist_key( -move.promo, move.to );
	}
	bits_set( &b->piece_boards[ color ][ move.promo ], move.to );
	bits_clear( &b->piece_boards[ color ][ pawn ], move.to );
	b->piece_counts[ color ][ pawn ]--;
	b->piece_counts[ color ][ move.promo ]++;
}


static void handle_promotion_undo( board_t* b, move_t move )
{
	if( b->flags.side_to_move == white )
	{
		bits_clear( &b->piece_boards[ white ][ move.promo ], move.from );
		bits_set( &b->piece_boards[ white ][ pawn ], move.from );
		b->piece_counts[ white ][ move.promo ]--;
		b->piece_counts[ white ][ pawn ]++;
		b->piece_values[ move.from ] = w_pawn;
	}
	else
	{
		bits_clear( &b->piece_boards[ black ][ move.promo ], move.from );
		bits_set( &b->piece_boards[ black ][ pawn ], move.from );
		b->piece_counts[ black ][ move.promo ]--;
		b->piece_counts[ black ][ pawn ]++;
		b->piece_values[ move.from ] = b_pawn;
	}
}


static void store_undo_info( board_t* b, undo_info_t* undo )
{
	b->undo_info[ b->flags.half_move_count ] = *undo;
}


bool board_last_move_was_capture( board_t* board )
{
	if( board->flags.most_recent_capture == 0 )
		return false;
	else
		return true;
}


bool board_move_is_capture( board_t* board, move_t move )
{
	return (board->piece_values[ move.to ] != 0);
}


int board_repetitions( board_t* board )
{
	return board->flags.repeat_count;
}
/*
static board_find_king_pins( board_t* b )
{
	int stm, sntm;
	int ksquare, pin_square;
	uqword pinned_board, pinning_board;
	uqword* rank_pins;
	uqword* file_pins;
	uqword* diag1_pins;
	uqword* diag2_pins;
	stm = b->flags.side_to_move;
	sntm = color_flip( stm );
	rank_pins = b->pin_by_rank + b->flags.half_move_count;
	file_pins = b->pin_by_file + b->flags.half_move_count;
	diag1_pins = b->pin_by_diag1 + b->flags.half_move_count;
	diag2_pins = b->pin_by_diag2 + b->flags.half_move_count;
	*rank_pins = 0x0LL;
	*file_pins = 0x0LL;
	*diag1_pins = 0x0LL;
	*diag2_pins = 0x0LL;
	ksquare = bits_first_one( b->piece_boards[ stm ][ king ] );
	if( b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ rook ] )
	{
		pinned_board = rank_moves( b->occupied_squares, ksquare );
		pinned_board &= b->piece_boards[ stm ][ 0 ];
		if( pinned_board )
		{
			pin_square = bits_remove_first_one( &pinned_board );
			pinning_board = rank_moves( b->occupied_squares, pin_square );
			pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ rook ];
			if( pinning_board )
				bits_set( rank_pins, pin_square );
			if( pinned_board )
			{
				pin_square = bits_first_one( pinned_board );
				pinning_board = rank_moves( b->occupied_squares, pin_square );
				pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ rook ];
				if( pinning_board )
					bits_set( rank_pins, pin_square );
			}
		}
		pinned_board = file_moves( b->occupied_squares, ksquare );
		pinned_board &= b->piece_boards[ stm ][ 0 ];
		if( pinned_board )
		{
			pin_square = bits_remove_first_one( &pinned_board );
			pinning_board = file_moves( b->occupied_squares, pin_square );
			pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ rook ];
			if( pinning_board )
				bits_set( file_pins, pin_square );
			if( pinned_board )
			{
				pin_square = bits_first_one( pinned_board );
 				pinning_board = file_moves( b->occupied_squares, pin_square );
				pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ rook ];
				if( pinning_board )
					bits_set( file_pins, pin_square );
			}
		}
	}
	if( b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ bishop ] )
	{
		pinned_board = diag1_moves( b->occupied_squares, ksquare );
		pinned_board &= b->piece_boards[ stm ][ 0 ];
		if( pinned_board )
		{
			pin_square = bits_remove_first_one( &pinned_board );
			pinning_board = diag1_moves( b->occupied_squares, pin_square );
			pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ bishop ];
			if( pinning_board )
				bits_set( diag1_pins, pin_square );
			if( pinned_board )
			{
				pin_square = bits_first_one( pinned_board );
				pinning_board = diag1_moves( b->occupied_squares, pin_square );
				pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ bishop ];
				if( pinning_board )
					bits_set( diag1_pins, pin_square );
			}
		}
		pinned_board = diag2_moves( b->occupied_squares, ksquare );
		pinned_board &= b->piece_boards[ stm ][ 0 ];
		if( pinned_board )
		{
			pin_square = bits_remove_first_one( &pinned_board );
			pinning_board = diag2_moves( b->occupied_squares, pin_square );
			pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ bishop ];
			if( pinning_board )
				bits_set( diag2_pins, pin_square );
			if( pinned_board )
			{
				pin_square = bits_first_one( pinned_board );
				pinning_board = diag2_moves( b->occupied_squares, pin_square );
				pinning_board &= b->piece_boards[ sntm ][ queen ] | b->piece_boards[ stm ][ bishop ];
				if( pinning_board )
					bits_set( diag2_pins, pin_square );
			}
		}
	}
}

*/


/*int gen_king_evasions( board_t* b, move_t* m_list )
{
	int stm, sntm, hmc;
	int ksquare, checker_square, from_square;
	bool double_check;
	uqword attackers, all_pinned, q1;
	
	hmc = b->flags.half_move_count;
	stm = b->flags.side_to_move;
	sntm = color_flip( stm );
	ksquare = bits_first_one( b->piece_boards[ stm ][ king ] );
	if( stm == white )
		attackers = white_pawn_attacks( ksquare ) & b->piece_boards[ black ][ pawn ];
	else
		attackers = black_pawn_attacks( ksquare ) & b->piece_boards[ white ][ pawn ];
	attackers |= knight_attacks( ksquare ) & b->piece_boards[ sntm ][ pawn ];
	attackers |= bishop_attacks( b->occupied_squares, ksquare ) 
	             & (b->piece_boards[ sntm ][ queen ] | b->piece_boards[ sntm ][ bishop ]);
	attackers |= rook_attacks( b->occupied_squares, ksquare ) 
	             & (b->piece_boards[ sntm ][ queen ] | b->piece_boards[ sntm ][rook ]);
	checker_square = bits_remove_first_one( &attackers );
	double_check = (bool) (attackers != 0x0LL);
	all_pinned = b->pin_by_diag1[ hmc ];
	all_pinned |= b->pin_by_diag2[ hmc ];
	all_pinned |= b->pin_by_rank[ hmc ];
	all_pinned |= b->pin_by_file[ hmc ];
	if( !double_check )
	{
		if( stm == white )
		{
			if(	(checker_square % 8 > 0) &&
				bits_test( b->piece_boards[ white ][ pawn ], checker_square - 9 ) && 
				(!bits_test( all_pinned, checker_square - 9) || bits_test( b->pin_by_diag1[ hmc ], checker_square - 9 )) )
			{
				store_move( checker_square - 9, checker_square );
			}
			if(	(checker_square % 8 < 7) &&
				bits_test( b->piece_boards[ white ][ pawn ], checker_square - 7 ) && 
				(!bits_test( all_pinned, checker_square - 7) || bits_test( b->pin_by_diag2[ hmc ], checker_square - 7 )) )
			{
				store_move( checker_square - 7, checker_square );
			}
		}
		else
		{
			if(	(checker_square % 8 < 7) &&
				bits_test( b->piece_boards[ black ][ pawn ], checker_square + 9 ) && 
				(!bits_test( all_pinned, checker_square + 9) || bits_test( b->pin_by_diag1[ hmc ], checker_square + 9 )) )
			{
				store_move( checker_square + 9, checker_square );
			}
			if(	(checker_square % 8 > 0) &&
				bits_test( b->piece_boards[ black ][ pawn ], checker_square + 7 ) && 
				(!bits_test( all_pinned, checker_square + 7) || bits_test( b->pin_by_diag2[ hmc ], checker_square + 7 )) )
			{
				store_move( checker_square + 7, checker_square );
			}
		}
		attackers = knight_attacks( checker_square ) & b->piece_boards[ stm ][ knight ] & ~all_pinned;
		while( attackers )
		{
			from_square = bits_remove_first_one( &attackers );
			store_move( from_square, checker_square );
		}
		q1 = b->piece_boards[ stm ][ bishop ] | b->piece_boards[ stm ][ queen ];
//		attackers = 
		
	}
	return 0;
}



*/


bool board_gen_LVA_attack_to_square( board_t* board, move_t* move, int square )
{
	bool rc;
	int stm;
	uqword attackers;
	rc = false;
	move->promo = empty;
	move->to = square;
	stm = board->flags.side_to_move; 
	if( rc == false )
	{
		if( board->flags.side_to_move == white )
			attackers = black_pawn_attacks( square ) & board->piece_boards[ white ][ pawn ];
		else
			attackers = white_pawn_attacks( square ) & board->piece_boards[ black ][ pawn ];
		if( attackers )
		{
			rc = true;
			move->from = bits_first_one( attackers );
			if( (move->to / 8 == 0) || (move->to / 8 == 7) )
				move->promo = queen;
		}
	}
	if( rc == false )
	{
		attackers = knight_attacks( square ) & board->piece_boards[ stm ][ knight ];
		if( attackers )
		{
			rc = true;
			move->from = bits_first_one( attackers );
		}
	}
	if( rc == false )
	{
		attackers = bishop_attacks( board->occupied_squares, square ) & board->piece_boards[ stm ][ bishop ];
		if( attackers )
		{
			rc = true;
			move->from = bits_first_one( attackers );
		}
	}
	if( rc == false )
	{
		attackers = rook_attacks( board->occupied_squares, square ) & board->piece_boards[ stm ][ rook ];
		if( attackers )
		{
			rc = true;
			move->from = bits_first_one( attackers );
		}
	}
	if( rc == false )
	{
		attackers = queen_attacks( board->occupied_squares, square ) & board->piece_boards[ stm ][ queen ];
		if( attackers )
		{
			rc = true;
			move->from = bits_first_one( attackers );
		}
	}
	if( rc == false )
	{
		attackers = king_attacks( square ) & board->piece_boards[ stm ][ king ];
		if( attackers )
		{
			rc = true;
			move->from = bits_first_one( attackers );
		}
	}
	if( rc )
		assert( board_is_pseudo_legal_move( board, *move ) );
	return rc;
}
