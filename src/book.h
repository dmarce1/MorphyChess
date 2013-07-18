
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
#ifndef BOOK_H_
#define BOOK_H_


typedef struct 
{
	uqword key;
	int w;
	int l;
	int d;
	int color;
	char eco[ 4 ];
	int rating;
} book_entry_t;

typedef struct
{
	book_entry_t book[ BOOK_SIZE ];
	int count;
	char last_eco[ 4 ];
 } book_t;

void book_open( book_t* book );
void book_close( book_t* book );
int book_digest( const char* filename, book_t* book );
move_t book_find_move( book_t* book, board_t* b );
void book_free( book_t* book );
void book_init( book_t* book );
void book_learning( book_t* book, board_t* board, int, result_t, int );
void book_show_book_moves( book_t* book, board_t* b );


#endif /*BOOK_H_*/
