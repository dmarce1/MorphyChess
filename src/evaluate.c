
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
#include "board.h"
#include "move_data.h"
#include "bits.h"
#include "params.h"

#define min( a, b ) (((a) < (b)) ? (a) : (b))

int piece_values[ 7 ] = { 0, 100, 325, 350, 500, 900, INFTY };
int max_material = 3100;
int max_material_wo_bonus = 2400;
int starting_material;
int starting_material_wo_pawns;

void init_evaluator( void );
int evaluate( board_t* b );
static int evaluate_material( board_t* b );
static int evaluate_pawns( board_t* b );
static bool test_defenders_gte_attackers( uqword defenders, uqword attackers );
static bool test_defenders_gt_attackers( uqword defenders, uqword attackers );
static bool test_defenders_e_attackers( uqword defenders, uqword attackers );


void init_evaluator( void )
{
	starting_material = 16*piece_values[ pawn ] + 4*piece_values[ knight ] + 4*piece_values[ bishop ] + 
	                    4*piece_values[ rook ] + 2*piece_values[ queen ];
	starting_material_wo_pawns = 4*piece_values[ knight ] + 4*piece_values[ bishop ] + 
	                             4*piece_values[ rook ] + 2*piece_values[ queen ];
}


int evaluate( board_t* b )
{
	int score;

	score = evaluate_material( b );
	
	return score;
}


int evaluate_material( board_t* b )
{
	int material[ 2 ], material_difference, total_material;
	int bonus, score, winning_side_pawns;
	int i, j;

	for( j = 0; j < 2; j++ )
		material[ j ] = 0;
	for( i = pawn; i < king; i++ )
	{
		for( j = 0; j < 2; j++ )
			material[ j ] += b->piece_counts[ j ][ i ] * piece_values[ j ];
	}
	material_difference = material[ white ] - material[ black ];
	total_material = material[ white ] + material[ black ];
	winning_side_pawns = b->piece_counts[ material_difference > 0 ? white : black ][ pawn ];
	bonus = material_difference * winning_side_pawns * (starting_material - total_material);
	bonus /= starting_material_wo_pawns * (winning_side_pawns + 1);
	score = min( material_difference, max_material_wo_bonus ) + bonus;
	score = min( score, max_material );

	return score;
}


int evaluate_pawns( board_t* b )
{
	const uqword white_pawns = b->piece_boards[ white ][ pawn ];
	const uqword black_pawns = b->piece_boards[ black ][ pawn ];
	const uqword all_pawns = white_pawns | black_pawns;
	bool is_iso, is_weak;
	uqword pawns, attackers, defenders, w_passers, b_passers, w_candidates, b_candidates;
	uqword wp_moves, wp_lanes, bp_moves, bp_lanes;
	int w_doubled, b_doubled, score, w_isolated, b_isolated, w_groups, b_groups, w_weak, b_weak;
	int w_duos, b_duos;
	int square, file;
	score = 0;
	/**** WHITE ****/
	pawns = white_pawns;
	wp_lanes = 0x0LL;
	w_doubled = 0;
	w_isolated = 0;
	w_groups = 0;
	w_duos = 0;
	wp_moves = (pawns << 8) & ~all_pawns;
	wp_moves |= ((wp_moves & rank_3) << 8) & ~all_pawns;
	while( pawns )
	{
		square = bits_remove_first_one( &pawns );
		wp_lanes |= bits_mask( square );
		square += 8;
		while( square < 64 )
		{
			if( bits_test( all_pawns, square ) )
			{
				if( bits_test( white_pawns, square ) )
					w_doubled++;
				break;
			}
			attackers = black_pawns & white_pawn_attacks( square );
			defenders = white_pawns & black_pawn_attacks( square );
			if( !test_defenders_gte_attackers( defenders, attackers ) )
				break;
			wp_lanes |= bits_mask( square );
			square += 8;
		}
	}
	/**** BLACK ****/
	pawns = black_pawns;
	bp_lanes = 0x0LL;
	b_doubled = 0;
	b_isolated = 0;
	b_duos = 0;
	b_groups = 0;
	bp_moves = (pawns >> 8) & ~all_pawns;
	bp_moves |= ((wp_moves & rank_6) >> 8) & ~all_pawns;
	while( pawns )
	{
		square = bits_remove_first_one( &pawns );
		bp_lanes |= bits_mask( square );
		square -= 8;
		while( square >= 0 )
		{
			if( bits_test( all_pawns, square ) )
			{
				if( bits_test( black_pawns, square ) )
					b_doubled++;
				break;
			}
			attackers = white_pawns & black_pawn_attacks( square );
			defenders = black_pawns & white_pawn_attacks( square );
			if( !test_defenders_gte_attackers( defenders, attackers ) )
				break;
			bp_lanes |= bits_mask( square );
			square -= 8;
		}
	}
	/********** white 2nd pass *************/
	pawns = white_pawns;
	while( pawns )
	{
		square = bits_remove_first_one( &pawns );
		file = square % 8;
		is_iso = false;
		if( file == 0 )
		{
			if( white_pawns & file_b == 0x0LL )
			{
				w_isolated++;
				w_groups++;
				is_iso = true;
			}
			else if( white_pawns & bits_mask( square + 1 ) )
			{
				w_duos++;
			}
		}
		else if( file % 8 != 7 )
		{
			if( white_pawns & (file_a << (file + 1)) == 0x0LL )
			{
				w_groups++;
				if( white_pawns & (file_a << (file - 1)) == 0x0LL )
				{
					w_isolated++;
					is_iso = true;
				}
			}
			else if( white_pawns & bits_mask( square + 1 ) )
			{
				w_duos++;
			}
			else if( white_pawns & bits_mask( square - 1 ) )
			{
				w_duos++;
			}
		}
		else
		{
			w_groups++;
			if( white_pawns & file_g == 0x0LL )
			{
				w_isolated++;
				is_iso = true;
			}
			else if( white_pawns & bits_mask( square - 1 ) )
			{
				w_duos++;
			}
		}
		is_weak = is_iso;
		if( !is_weak )
		{
			is_weak = true;
			attackers = white_pawns & black_pawn_attacks( square );
			defenders = black_pawns & white_pawn_attacks( square );
			if( test_defenders_gt_attackers( defenders, attackers ) )
			{
				is_weak = false;
			}
			else
			{
				if( (wp_moves & wp_lanes & black_pawn_attacks( square )) && test_defenders_e_attackers( defenders, attackers ) )
				{
					is_weak = false;
				}
				else if( wp_moves & bits_mask( square + 8 ) )
				{
					attackers = white_pawns & black_pawn_attacks( square + 8 );
					defenders = black_pawns & white_pawn_attacks( square + 8 );
					if( test_defenders_gt_attackers( defenders, attackers ) )
						is_weak = false;
				}
				else if( is_weak && (square < 48) && (wp_moves & bits_mask( square + 16 )) )
				{
					attackers = white_pawns & black_pawn_attacks( square + 16 );
					defenders = black_pawns & white_pawn_attacks( square + 16 );
					if( test_defenders_gt_attackers( defenders, attackers ) )
						is_weak = false;
				}
			}
		}
		if( is_weak )
			w_weak++;
	}
	/********** black 2nd pass *************/
	pawns = black_pawns;
	while( pawns )
	{
		square = bits_remove_first_one( &pawns );
		file = square % 8;
		is_iso = false;
		if( file == 0 )
		{
			if( black_pawns & file_b == 0x0LL )
			{
				b_isolated++;
				b_groups++;
				is_iso = true;
			}
			else if( black_pawns & bits_mask( square + 1 ) )
			{
				b_duos++;
			}
		}
		else if( file % 8 != 7 )
		{
			if( black_pawns & (file_a << (file + 1)) == 0x0LL )
			{
				b_groups++;
				if( black_pawns & (file_a << (file - 1)) == 0x0LL )
				{
					b_isolated++;
					is_iso = true;
				}
			}
			else if( black_pawns & bits_mask( square + 1 ) )
			{
				b_duos++;
			}
			else if( black_pawns & bits_mask( square - 1 ) )
			{
				b_duos++;
			}
		}
		else
		{
			b_groups++;
			if( black_pawns & file_g == 0x0LL )
			{
				is_iso = true;
				b_isolated++;
			}
			else if( black_pawns & bits_mask( square - 1 ) )
			{
				b_duos++;
			}
		}
		is_weak = is_iso;
		if( !is_weak )
		{
			is_weak = true;
			attackers = black_pawns & white_pawn_attacks( square );
			defenders = white_pawns & black_pawn_attacks( square );
			if( test_defenders_gt_attackers( defenders, attackers ) )
			{
				is_weak = false;
			}
			else
			{
				if( (bp_moves & bp_lanes & black_pawn_attacks( square )) && test_defenders_e_attackers( defenders, attackers )  )
					is_weak = false;
				else if( wp_moves & bits_mask( square - 8 ) )
				{
					attackers = black_pawns & white_pawn_attacks( square - 8 );
					defenders = white_pawns & black_pawn_attacks( square - 8 );
					if( test_defenders_gt_attackers( defenders, attackers ) )
						is_weak = false;
				}
				else if( is_weak && (square >= 16) && (wp_moves & bits_mask( square - 16 )) )
				{
					attackers = black_pawns & white_pawn_attacks( square - 16 );
					defenders = white_pawns & black_pawn_attacks( square - 16 );
					if( test_defenders_gt_attackers( defenders, attackers ) )
						is_weak = false;
				}
			}
		}
		if( is_weak )
			b_weak++;
	}
	return score;
}




static bool test_defenders_gte_attackers( uqword defenders, uqword attackers )
{
	bool rc;
	if( attackers == 0x0LL )
	{
		rc = true;
	}
	else if( defenders == 0x0LL)
	{
		rc = false;
	}
	else
	{
		attackers &= attackers - 1;
		defenders &= defenders - 1;
		if( attackers == 0x0LL )
			rc = true;
		else if( defenders == 0x0LL)
			rc = false;
		else
			rc = true;
	}
	return rc;
}

static bool test_defenders_gt_attackers( uqword defenders, uqword attackers )
{
	bool rc;
	if( defenders == 0x0LL )
	{
		rc = false;
	}
	else if( attackers == 0x0LL)
	{
		rc = true;
	}
	else
	{
		attackers &= attackers - 1;
		defenders &= defenders - 1;
		if( defenders == 0x0LL )
			rc = false;
		else if( attackers == 0x0LL)
			rc = true;
		else
			rc = false;
	}
	return rc;
}

static bool test_defenders_e_attackers( uqword defenders, uqword attackers )
{
	bool rc;
	if( !defenders && !attackers )
	{
		rc = true;
	}
	else if( attackers && defenders ) 
	{
		attackers &= attackers - 1;
		defenders &= defenders - 1;
		if( !defenders && !attackers )
			rc = true;
		else if( attackers & defenders )
			rc = true;
	}
	return rc;
}
