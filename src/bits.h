
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

#ifndef __BITS1_OPERATIONS_H
#define __BITS1_OPERATIONS_H

#define RANK_INDEX  0
#define FILE_INDEX  1
#define DIAG1_INDEX 2
#define DIAG2_INDEX 3

#include "integers.h"
#include "bool.h"

void rotated_bits_create_set( uqword*, uqword );
void init_bits( void );
int bits_count_ones( uqword );
int bits_first_one( uqword );
int bits_last_one( uqword );
int bits_remove_first_one( uqword* );
int bits_remove_last_one( uqword* );
void bits_vertical_flip( uqword* );
void bits_horizontal_flip( uqword* );
void bits_clear( uqword* q, int i );
void bits_set( uqword* q, int i );
bool bits_test( uqword q, int i );

uqword bits_mask( int i );
void rotated_bits_clear( uqword* q, int i );
void rotated_bits_set( uqword* q, int i );
void rotated_bits_zero_out( uqword* q );
int rotated_index_from_rank ( const uqword*, int );
int rotated_index_from_file ( const uqword*, int );
int rotated_index_from_diag1( const uqword*, int );
int rotated_index_from_diag2( const uqword*, int );
void bits_clear( uqword* q, int i );
void bits_set( uqword* q, int i );
bool bits_test( uqword q, int i );
uqword bits_mask( int i );





#endif
