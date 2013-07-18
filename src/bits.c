
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

#include "bits.h"
#include "board.h"
#include "debug.h"

uqword mask_set    [ 64 ][   4 ];
static uqword inv_mask_set[ 64 ][   4 ];
static int first_one_table[ 128 ];

static int inv_first_one_table[ 64 ] =
{
	 96,  53,  95,  51,  91,  43,  75, 76,
	 78,  82,  90,  41,   8,   5, 127, 52,
	 93,  47,  83,  92,  45,  79,  84, 94,
	 49,  87,  35, 124,  46,  81,  88, 37,
	  0, 117,  32, 118,  34, 122,  42, 10,
	  9,   7,   3, 123,  44,  77,  80, 86,
	 33, 120,  38,   2, 121,  40,   6,  1,
	119,  36, 126,  50,  89,  39,   4,125
};

static const sbyte rotated_index[ 4 ][ 64 ] = 
{
	{  
		 0,  1,  2,  3,  4,  5,  6,  7,
		 8,  9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63
	},
	{
		 0,  8, 16, 24, 32, 40, 48, 56,
		 1,  9, 17, 25, 33, 41, 49, 57,
		 2, 10, 18, 26, 34, 42, 50, 58,
		 3, 11, 19, 27, 35, 43, 51, 59,
		 4, 12, 20, 28, 36, 44, 52, 60,
		 5, 13, 21, 29, 37, 45, 53, 61,
		 6, 14, 22, 30, 38, 46, 54, 62,
		 7, 15, 23, 31, 39, 47, 55, 63
	},
	{
		 0,  8, 16, 24, 32, 40, 48, 56,
		57,  1,  9, 17, 25, 33, 41, 49,
		50, 58,  2, 10, 18, 26, 34, 42,
		43, 51, 59,  3, 11, 19, 27, 35,
		36, 44, 52, 60,  4, 12, 20, 28,
		29, 37, 45, 53, 61,  5, 13, 21,
		22, 30, 38, 46, 54, 62,  6, 14,
		15, 23, 31, 39, 47, 55, 63,  7,
	},
	{
		56, 48, 40, 32, 24, 16,  8,  0,
		49, 41, 33, 25, 17,  9,  1, 57,
		42, 34, 26, 18, 10,  2, 58, 50,
		35, 27, 19, 11,  3, 59, 51, 43,
		28, 20, 12,  4, 60, 52, 44, 36,
		21, 13,  5, 61, 53, 45, 37, 29,
		14,  6, 62, 54, 46, 38, 30, 22,
		 7, 63, 55, 47, 39, 31, 23, 15
	}
};


int bits_count_ones( uqword data ) 
{
	unsigned n;
	const uqword a = data - ((data >> 1) & 0x5555555555555555LL);
	const uqword c = (a & 0x3333333333333333LL) + ((a >> 2) & 0x3333333333333333LL);

	n = ((unsigned) c) + ((unsigned) (c >> 32));
	n = (n & 0x0F0F0F0F) + ((n >> 4) & 0x0F0F0F0F);
	n = (n & 0xFFFF) + (n >> 16);
	n = (n & 0xFF) + (n >> 8);
	return n;
}


int bits_first_one( uqword data ) 
{

#ifdef __ICC
#ifdef __x86_64
	const uqword* qw = &data;
	uqword i;

	__asm	
	{
		mov	rbx, qw
		mov	rdx, qword ptr[ rbx ]
		bsf	rax, rdx
		mov	i, rax
	}
	return (int) i;
#else
	const uqword* qw = &data;
	int i;

	__asm	
	{
		mov	ebx, qw
		mov	edx, dword ptr[ ebx ]
		bsf	eax, edx
		jz	NEXT
		jmp	DONE
NEXT:	mov	edx, dword ptr[ ebx + 4 ]
		bsf	eax, edx
		add	eax, 32
DONE:	mov	i, eax
	}
	return i;
#endif
#else
	data |= data << 1;
	data |= data << 2;
	data |= data << 4;
	data |= data << 8;
	data |= data << 16;
	data |= data << 32;
	data = ~data;
	return bits_count_ones( data );
#endif
}


int bits_last_one( uqword data ) 
{
	data |= data >> 1;
	data |= data >> 2;
	data |= data >> 4;
	data |= data >> 8;
	data |= data >> 16;
	data |= data >> 32;
	return bits_count_ones( data ) - 1;
}


int bits_remove_first_one( uqword* data )
{
	int i, j;
	uqword q;

	assert( data != NULL );

	i = bits_first_one( *data );
	*data &= *data - 1;
	return i;
}

int bits_remove_last_one( uqword* data )
{
	int i;

	assert( data != NULL );

	i = bits_last_one( *data );
	if( i > 0 )
	{
		bits_clear( data, i );
	}
	return i;
}

void bits_horizontal_flip( uqword* data )
{
	uqword bb;

	assert( data != NULL );

	bb =         ((*data & 0xF0F0F0F0F0F0F0F0LL) >> 4);
	*data = bb | ((*data & 0x0F0F0F0F0F0F0F0FLL) << 4);
	bb =         ((*data & 0xCCCCCCCCCCCCCCCCLL) >> 2);
	*data = bb | ((*data & 0x3333333333333333LL) << 2);
	bb =         ((*data & 0xAAAAAAAAAAAAAAAALL) >> 1);
	*data = bb | ((*data & 0x5555555555555555LL) << 1);
}


void bits_vertical_flip( uqword* data )
{
	uqword bb;

	assert( data != NULL );

	bb =         ((*data & 0xFFFFFFFF00000000LL) >> 32);
	*data = bb | ((*data & 0x00000000FFFFFFFFLL) << 32);
	bb =         ((*data & 0xFFFF0000FFFF0000LL) >> 16);
	*data = bb | ((*data & 0x0000FFFF0000FFFFLL) << 16);
	bb =         ((*data & 0xFF00FF00FF00FF00LL) >> 8);
	*data = bb | ((*data & 0x00FF00FF00FF00FFLL) << 8);
}


void init_bits( void )
{
	int k, j;
	for( j = 0; j < 64; j++ )
	{
		first_one_table[ inv_first_one_table[ j ] ] = j;
		for( k = 0; k < 4; k++ )
		{
 			mask_set[ j ][ k ] = 1LL << rotated_index[ k ][ j ];
			inv_mask_set[ j ][ k ] = ~mask_set[ j ][ k ];
		}
	}
}


void rotated_bits_clear( uqword* q, int i )
{
	q[ 0 ] &= inv_mask_set[ i ][ 0 ];
	q[ 1 ] &= inv_mask_set[ i ][ 1 ];
	q[ 2 ] &= inv_mask_set[ i ][ 2 ];
	q[ 3 ] &= inv_mask_set[ i ][ 3 ];
}


void rotated_bits_set( uqword* q, int i )
{
	q[ 0 ] |= mask_set[ i ][ 0 ];
	q[ 1 ] |= mask_set[ i ][ 1 ];
	q[ 2 ] |= mask_set[ i ][ 2 ];
	q[ 3 ] |= mask_set[ i ][ 3 ];
}


void rotated_bits_create_set( uqword* set, uqword q )
{
	int i, j;

	for( j = 0; j < 4; j++ )
		set[ j ] = 0LL;
	for( i = 0; i < 64; i++ )
	{
		for( j = 0; j < 4; j++ )
		{
			if( bits_test( q, i ) )
				rotated_bits_set( set, i );
		}
	}
}


int rotated_index_from_rank( const uqword* o, int i )
{
	int j;
	j = 8 * (i / 8);
	return (o[ RANK_INDEX ] >> j) & 0xFF;
}


int rotated_index_from_file( const uqword* o, int i )
{
	int j;
	j = 8 * (rotated_index[ FILE_INDEX ][ i ] / 8);
	return (o[ FILE_INDEX ] >> j) & 0xFF;
}


int rotated_index_from_diag1( const uqword* o, int i )
{
	int j;
	j = 8 * (rotated_index[ DIAG1_INDEX ][ i ] / 8);
	return (o[ DIAG1_INDEX ] >> j) & 0xFF;
}


int rotated_index_from_diag2( const uqword* o, int i )
{
	int j;
	j = 8 * (rotated_index[ DIAG2_INDEX ][ i ] / 8);
	return (o[ DIAG2_INDEX ] >> j) & 0xFF;
}


void bits_clear( uqword* q, int i ) 
{
	*q &= ~(1LL << i);
}


void bits_set( uqword* q, int i )
{
	*q |= (1LL << i);
}


bool bits_test( uqword q, int i )   
{ 
	bool rc;
	rc = ((q & (1LL << i)) != 0LL);
	return rc;
}


uqword bits_mask( int i )
{ 
	return (1LL << i);
}


void rotated_bits_zero_out( uqword* q )
{
	int i;
	for( i = 0; i < 4; i++ )
		q[ i ] = 0x0LL; 
}
