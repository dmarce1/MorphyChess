
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
