
#include "move_data.h"
#include "debug.h"


static uqword wp_attacks[ 64 ];
static uqword bp_attacks[ 64 ];
static uqword n_attacks[ 64 ];
static uqword k_attacks[ 64 ];
static uqword rank_attacks[ 64 ][ 256 ];
static uqword file_attacks[ 64 ][ 256 ];
static uqword diag1_attacks[ 64 ][ 256 ];
static uqword diag2_attacks[ 64 ][ 256 ];


static uqword gen_white_pawn_attacks( int i );
static uqword gen_black_pawn_attacks( int i );
static uqword gen_king_attacks( int i );
static uqword gen_knight_attacks( int i );



uqword white_pawn_attacks( int i )
{
	return wp_attacks[ i ];
}


uqword black_pawn_attacks( int i )
{
	return bp_attacks[ i ];
}


uqword king_attacks( int i )
{
	return k_attacks[ i ];
}


uqword knight_attacks( int i )
{
	return n_attacks[ i ];
}

uqword rank_moves( const uqword* o, int i )
{
	return rank_attacks[ i ][ rotated_index_from_rank( o, i ) ];
}

uqword file_moves( const uqword* o, int i )
{
	return file_attacks[ i ][ rotated_index_from_file( o, i ) ];
}

uqword diag1_moves( const uqword* o, int i )
{
	return diag1_attacks[ i ][ rotated_index_from_diag1( o, i ) ];
}

uqword diag2_moves( const uqword* o, int i )
{
	return diag2_attacks[ i ][ rotated_index_from_diag2( o, i ) ];
}


uqword bishop_attacks( const uqword* o, int i )
{

	uqword u, q;
	q = diag1_attacks[ i ][ rotated_index_from_diag1( o, i ) ];
	u = diag2_attacks[ i ][ rotated_index_from_diag2( o, i ) ];
	return q | u;
}


uqword queen_attacks( const uqword* bb, int i )
{
   assert_range( 0, i, 63 ); 

	return bishop_attacks( bb, i ) | rook_attacks( bb, i );
}


uqword rook_attacks( const uqword* o, int i )
{
	uqword u, q;
	q = rank_attacks[ i ][ rotated_index_from_rank( o, i ) ];
	u = file_attacks[ i ][ rotated_index_from_file( o, i ) ];
	return q | u;
}










void init_move_data( void )
{
	ubyte byte;
	int from, to, rank, file, i, j;
	unsigned int index;
	uqword q;
	uqword o_masks[ 256 ][ 4 ];
	uqword* o;

	init_bits();

	for( from = 0; from < 64; from++ )
	{
		wp_attacks[ from ] = gen_white_pawn_attacks( from );
		bp_attacks[ from ] = gen_black_pawn_attacks( from );
		k_attacks [ from ] = gen_king_attacks( from );
		n_attacks [ from ] = gen_knight_attacks( from );
	}
	for( index = 0; index < 256; index++ )
	{
		o = o_masks[ index ];
		for( j = 0; j < 4; j++ )
			o[ j ] = 0LL;
		for( i = 0; i < 8; i++ )
		{
			if( (index & (1 << i)) != 0 )
			{
				for( j = 0; j < 8; j++ )
					rotated_bits_set( o, 8 * i + j );
			}
		}
	}

	for( from = 0; from < 8; from++ )
	{
		for( index = 0; index < 256; index++ )
		{
			byte = index;
			q = 0LL;
			if( from < 7 )
			{
				for( to = from + 1; to <= 7; to++ )
				{
					bits_set( &q, to );
					if( (byte & (1 << to)) != 0 )
						break;
				}
			}
			if( from > 0 )
			{
				for( to = from - 1; to >= 0; to-- )
				{
					bits_set( &q, to );
					if( (byte & (1 << to)) != 0 )
						break;
				}
			}
			for( rank = 0; rank < 8; rank++ )
				rank_attacks[ 8 * rank + from ][ index ] = q << (8 * rank);
		}
	}
	for( from = 0; from < 64; from += 8 )
	{
		for( index = 0; index < 256; index++ )
		{
			byte = index;
			q = 0LL;
			rank = from / 8;
			if( rank < 7 )
			{
				for( to = from + 8; to < 64; to += 8 )
				{
					bits_set( &q, to );
					if( (byte & (1 << (to / 8))) != 0 )
						break;
				}
			}
			if( rank > 0 )
			{
				for( to = from - 8; to >= 0; to -= 8 )
				{
					bits_set( &q, to );
					if( (byte & (1 << (to / 8))) != 0 )
						break;
				}
			}
			for( file = 0; file < 8; file++ )
				file_attacks[ 8 * rank + file ][ index ] = q << file;
		}
	}
	for( from = 0; from < 64; from++ )
	{
		for( i = 0; i < 256; i++ )
		{
			o = o_masks[ i ];
			index = rotated_index_from_diag1( o, from );
			q = 0LL;
			if( ((from / 8) < 7) && ((from % 8) < 7) )
			{
				for( to = from + 9; to < 64; to += 9 )
				{
					bits_set( &q, to );
					if( bits_test( o[ 0 ], to ) )
						break;
					else if( !(((to / 8) < 7) && ((to % 8) < 7)) )
						break;
				}
			}
			if( ((from / 8) > 0) && ((from % 8) > 0) )
			{
				for( to = from - 9; to >= 0; to -= 9 )
				{
					bits_set( &q, to );
					if( bits_test( o[ 0 ], to ) )
						break;
					else if( !(((to / 8) > 0) && ((to % 8) > 0)) )
						break;
				}
			}
			diag1_attacks[ from ][ index ] = q;
		}
	}
	for( from = 0; from < 64; from++ )
	{
		for( i = 0; i < 256; i++ )
		{
			o = o_masks[ i ];
			index = rotated_index_from_diag2( o, from );
			q = 0LL;
			if( ((from / 8) < 7) && ((from % 8) > 0) )
			{
				for( to = from + 7; to < 64; to += 7 )
				{
					bits_set( &q, to );
					if( bits_test( o[ 0 ], to ) )
						break;
					else if( !(((to / 8) < 7) && ((to % 8) > 0)) )
						break;
				}
			}
			if( ((from / 8) > 0) && ((from % 8) < 7) )
			{
				for( to = from - 7; to >= 0; to -= 7 )
				{
					bits_set( &q, to );
					if( bits_test( o[ 0 ], to ) )
						break;
					else if( !(((to / 8) > 0) && ((to % 8) < 7)) )
						break;
				}
			}
			diag2_attacks[ from ][ index ] = q;
		}
	}
	return;
}



static uqword gen_white_pawn_attacks( int i )
{
	const int side = (i&0x7)>>2;

   assert_range( 0, i, 63 ); 

	uqword bb = (uqword) 0x00000000000000001LL << i;
	bb = (bb << 7) | (bb << 9);
	bb &= ~(side * file_a);
	bb &= ~((1-side) * file_h);
	bits_clear( &bb, i );
	return bb;
}


static uqword gen_black_pawn_attacks( int i )
{
	const int side = (i&0x7)>>2;
	uqword bb = (uqword) 0x00000000000000001LL << i;

	assert_range( 0, i, 63 ); 

	bb = (bb >> 7) | (bb >> 9);
	bb &= ~(side * file_a);
	bb &= ~((1-side) * file_h);
	bits_clear( &bb, i );
	return bb;
}


static uqword gen_king_attacks( int i )
{
	const int side = (i&0x7)>>2;
	uqword bb = (uqword) 0x00000000000000001LL << i;

    assert_range( 0, i, 63 ); 

	bb |= (bb >> 1) | (bb << 1);
	bb |= (bb >> 8) | (bb << 8);
	bb &= ~(side * file_a);
	bb &= ~((1-side) * file_h);
	bits_clear( &bb, i );
	return bb;
}


static uqword gen_knight_attacks( int i )
{
	const int side = (i&0x7)>>2;
	uqword bb = (uqword) 0x00000000000000001LL << i;
	uqword bb1;

   assert_range( 0, i, 63 ); 

	bb1 = (bb >> 6);
	bb1 |= (bb >> 10);
	bb1 |= (bb >> 15);
	bb1 |= (bb >> 17);
	bb1 |= (bb << 6);
	bb1 |= (bb << 10);
	bb1 |= (bb << 15);
	bb1 |= (bb << 17);
	bb1 &= ~(side * (file_a | file_b));
	bb1 &= ~((1-side) * (file_h | file_g));
	bits_clear( &bb1, i );
	return bb1;
}


