
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
#include "pgn.h"
#include "rating.h"
#include "params.h"
#include "book.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "board.h"
#include <string.h>

void book_free(book_t* book) 
{
}

void book_init(book_t* book) 
{
	int i;
	for (i = 0; i < BOOK_SIZE; i++) {
		book->book[ i ].key = 0x0LL;
		book->book[ i ].w = 0;
		book->book[ i ].l = 0;
		book->book[ i ].d = 0;
	}
	book->count = 0;
}

int book_entry_compare(const void* _entry1, const void* _entry2) {
	const book_entry_t* entry1 = _entry1;
	const book_entry_t* entry2 = _entry2;
	if (entry1->color > entry2->color)
		return 1;
	else if (entry1->color < entry2->color)
		return -1;
	else if (entry1->key > entry2->key)
		return 1;
	else if (entry1->key < entry2->key)
		return -1;
	else
		return 0;
}

void book_add_entry(book_t* book, board_t* position, char eco[ 4 ]) 
{
	int i;
	bool duplicate;
	duplicate = false;
	for (i = 0; i < book->count; i++) {
		if (book->book[ i ].key == position->position_key)
			if (book->book[ i ].color == position->flags.side_to_move) 
			{
				if( book->book[ i ].rating < INITIAL_BOOK_RATING + 300 )
					book->book[ i ].w++;
				duplicate = true;
				break;
			}
	}
	if ( !duplicate) {
		if (book->count == BOOK_SIZE) {
			///			printf( "book to small\n" );
			abort();
		}
		book->book[ book->count ].key = position->position_key;
		book->book[ book->count ].color = position->flags.side_to_move;
		book->book[ book->count ].w = 1;
		book->book[ book->count ].l = 0;
		book->book[ book->count ].d = 0;
		book->book[ book->count ].rating = INITIAL_BOOK_RATING;
		book->book[ book->count ].eco[ 0 ] = eco[ 0 ];
		book->book[ book->count ].eco[ 1 ] = eco[ 1 ];
		book->book[ book->count ].eco[ 2 ] = eco[ 2 ];
		book->book[ book->count ].eco[ 3 ] = '\0';
		book->count++;
	}
}

int book_digest(const char* filename, book_t* book) {
	board_t board;
	int move_cnt, opening_cnt, move_num;
	pgn_t pgn;
	init_pgn( &pgn);
	book_init(book);
	if ( !load_pgn( &pgn, filename, &board) ) {
		//		printf( "Could not load %s for book digest.\n", filename );
	} else {
		//		printf( "Digesting %s.\n", filename );
		opening_cnt = 0;
		do {
			move_cnt = board.flags.half_move_count;
			while (board.flags.half_move_count > 0) {
				book_add_entry(book, &board, pgn.eco);
				board_undo_move( &board);
			}
			opening_cnt++;
		} while (next_pgn( &pgn, &board) );
	}
	destroy_pgn( &pgn);
	qsort(book->book, book->count, sizeof(book_entry_t), book_entry_compare);
	return 0;
}

void book_tool( book_t* b )
{
	int i;
	int game_cnt;
	int not_rated_cnt = 0;
	int p_rated_cnt = 0;
	int s_rated_cnt = 0;
	for( i = 0; i < b->count; i++ )
	{
		game_cnt = b->book[ i ].w;
		game_cnt += b->book[ i ].l;
		game_cnt += b->book[ i ].d;
		if( game_cnt == 0 )
			not_rated_cnt++;
		else if( game_cnt < 20 )
			p_rated_cnt++;
		else
			s_rated_cnt++;
	}
	printf( "Percent of book not yet rated       = %%%6.2f\n", 100.0 * not_rated_cnt / b->count );
	printf( "Percent of book provisionally rated = %%%6.2f\n", 100.0 * p_rated_cnt / b->count );
	printf( "Percent of book with stable ratings = %%%6.2f\n", 100.0 * s_rated_cnt / b->count );
	printf( "\n" );
	exit( 0 );
}

void book_open(book_t* book) {
	FILE* fp;
	fp = fopen(BOOK_NAME, "rb");
	if (fp == NULL) {
		book_digest("eco.pgn", book);
		book_close(book);
		fp = fopen(BOOK_NAME, "rb");
	}
	fread(book, sizeof(book_t), 1, fp);
	fclose(fp);
#ifdef BOOK_TOOL
	book_tool( book );
#endif
}

void book_close(book_t* book) 
{
	FILE* fp;
	fp = fopen(BOOK_NAME, "wb");
	fwrite(book, sizeof(book_t), 1, fp);
	fclose(fp);
}

int book_search(book_t* book, board_t* board) 
{
	int upper, lower, test, color;
	bool rc;
	book_entry_t entry;
	color = board->flags.side_to_move;
	entry.key = board->position_key;
	entry.color = board->flags.side_to_move;
	lower = 0;
	upper = book->count - 1;
	rc = false;
	while (upper - lower > 1) {
		test = (upper + lower) / 2;
		switch (book_entry_compare( &entry, book->book + test) ) {
		case 1:
			lower = test;
			break;
		case 0:
			upper = lower = test;
			rc = true;
			break;
		case -1:
			upper = test;
			break;
		}
	}
	if (rc)
		return lower;
	else
		return -1;
}

double rand1() 
{
	return (double) (rand() & 0x7FFFFFFF) / (double) 0x7FFFFFFF;
}

static bool srand_used = false;

move_t book_find_move(book_t* book, board_t* b) {
	move_t best_move, move;
	char str[ 256 ];
	bool found_entry;
	int move_cnt, i, book_index, best_i;
	move_t list[max_moves ];
	double best, score;
	move_t choices[ max_moves ];
	double chances[ max_moves ];
	double total_chances;
	double random_number;
	int choice_count, choice;

	move_set_empty( &best_move);
	if ( !srand_used)
		srand(time(NULL) );
	if ( !((b->flags.half_move_count == 0) || (book_search(book, b) >= 0)))
		return best_move;
#ifdef BOOK_OFF
	return best_move;
#endif
	move_cnt = board_gen_pseudolegal_moves(b, list);
	found_entry = false;
	best = -99999999.0;
	for (i = 0; i < move_cnt; i++) {
		move = list[ i ];
		if ( !board_make_move(b, move) )
			continue;
		book_index = book_search(book, b);
		board_undo_move(b);
		if( book_index >= 0 ) 
		{
			score = book->book[ book_index ].rating;
			if (score > best) 
			{
				best = score;
				best_i = i;
			}
			found_entry = true;
		}
	}
	total_chances = 0.0;
	if( found_entry ) 
	{
		choice_count = 0;
		for( i = 0; i < move_cnt; i++ ) 
		{
			move = list[ i ];
			if ( !board_make_move(b, move) )
				continue;
			book_index = book_search(book, b);
			board_undo_move(b);
			if( book_index >= 0 ) 
			{
				score = book->book[ book_index ].rating;
				choices[ choice_count ] = move;
				chances[ choice_count ] = book->book[ book_index ].w; 
				total_chances += chances[ choice_count ];
				choice_count++;
			}
		}
		for( i = 0; i < choice_count; i++ )
		{
			chances[ i ] /= total_chances;
		}
		random_number = rand1();
		choice = 0;
		for( i = 0; i < choice_count; i++ )
		{
			random_number -= chances[ i ];
			if( random_number <= 0.0 )
			{
				choice = i;
				break;
			}
		}
		best_move = choices[ choice ];
		board_make_move(b, best_move);
		book_index = book_search(book, b);
		board_undo_move(b);
		strcpy(book->last_eco, book->book[ book_index ].eco);
	}
	board_move_to_str(str, b, best_move);
	return best_move;
}

void book_learning(book_t* book, board_t* _board, int computer_color, result_t result, int opponent_rating ) {
	board_t* board;
	int bw, bl, bd, ww, wl, wd;
	int book_index, num_games, or;
	board = (board_t*) malloc(sizeof(board_t));
	board_copy(board, _board);
	switch (result) {
	case WHITE_WIN:
		ww = bl = 1;
		wl = bw = 0;
		bd = wd = 0;
		break;
	case BLACK_WIN:
		ww = bl = 0;
		wl = bw = 1;
		bd = wd = 0;
		break;
	case DRAW:
		ww = bl = 0;
		wl = bw = 0;
		bd = wd = 1;
		break;
	default:
		return;
	}
	while (board->flags.half_move_count > 0) {
		board_undo_move(board);
		book_index = book_search(book, board);
		num_games = 1;
		num_games += book->book[ book_index ].w;
		num_games += book->book[ book_index ].l;
		num_games += book->book[ book_index ].d;
		or = book->book[ book_index ].rating;
		if (book_index >= 0) 
		{
			if (board->flags.side_to_move == black) 
			{
				if (computer_color == black)
					continue;
				book->book[ book_index ].w += ww;
				book->book[ book_index ].l += wl;
				book->book[ book_index ].d += wd;
				if( num_games < 5 )
					num_games = 5;
				if( ww )
					book->book[ book_index ].rating = rating_compute_new_rating( or, opponent_rating, 1, 100 );
				else if( wl )
					book->book[ book_index ].rating = rating_compute_new_rating( or, opponent_rating, -1, 100 );
				else
					book->book[ book_index ].rating = rating_compute_new_rating( or, opponent_rating, 0, 100 );
		//		printf( "New rating for white is %i\n", book->book[ book_index ].rating );
			} 
			else 
			{
				if (computer_color == white)
					continue;
				book->book[ book_index ].w += bw;
				book->book[ book_index ].l += bl;
				book->book[ book_index ].d += bd;
				if( bw )
					book->book[ book_index ].rating = rating_compute_new_rating( or, opponent_rating, 1, num_games );
				else if( bl )
					book->book[ book_index ].rating = rating_compute_new_rating( or, opponent_rating, -1, num_games );
				else
					book->book[ book_index ].rating = rating_compute_new_rating( or, opponent_rating, 0, num_games );
		//		printf( "New rating for black is %i\n", book->book[ book_index ].rating );
			}
		}
	}
	book_close(book);
	book_open(book);
	free(board);
}

void book_show_book_moves(book_t* book, board_t* b) {
	move_t move;
	char str[ 256 ];
	bool found_entry;
	int move_cnt, i, book_index, j, k;
	move_t list[max_moves ];
	move_t book_moves[max_moves ];
	int ratings[ max_moves ];
	int num_games[ max_moves ];
	double chances[ max_moves ];
	double total_chances;
	int used[max_moves ];
	double best;
	int best_i;

	if ( !((b->flags.half_move_count == 0) || (book_search( book, b ) >= 0)))
	{
		printf("	No book moves found\n");
		printf("	Last book entry was ECO %s\n", book->last_eco);
	}
	move_cnt = board_gen_pseudolegal_moves(b, list);
	found_entry = false;
	j = 0;
	best = -9999999.0;
	for( i = 0; i < move_cnt; i++ ) 
	{
		used[ i ] = false;
		move = list[ i ];
		if ( !board_make_move(b, move) )
			continue;
		book_index = book_search(book, b);
		board_undo_move(b);
		if( book_index >= 0 ) 
		{
			ratings[ j ] = book->book[ book_index ].rating;
			num_games[ j ] = book->book[ book_index ].w;
			num_games[ j ] += book->book[ book_index ].l;
			num_games[ j ] += book->book[ book_index ].d;
			book_moves[ j ] = move;
			if( ratings[ j ] > best )
			{
				best = ratings[ j ];
				best_i = j;
			}
			j++;
			found_entry = true;
		}
	}
	total_chances = 0.0;
	if( found_entry ) 
	{
		printf( "	Book moves\n" );
		for( i = 0; i < j; i++ )
		{
			chances[ i ] = num_games[ i ];
			total_chances += chances[ i ];
		}
		for( i = 0; i < j; i++ )
			chances[ i ] /= total_chances;
		for( i = 0; i < j; i++ ) 
		{
			best = -999999.0;
			for( k = 0; k < j; k++ ) 
			{
				if( (num_games[ k ] > best) && !used[ k ] ) 
				{
					best = num_games[ k ];
					best_i = k;
				}
			}
			used[ best_i ] = true;
			move = book_moves[ best_i ];
			board_move_to_str(str, b, move);
			printf("	%2i. %10s %3i%%\n", i + 1, str, (int) (100*chances[ best_i ] + 0.5 ) );
		}
	} 
	else 
	{
		printf("	No book moves found\n");
		printf("	Last book entry was ECO %s\n", book->last_eco);
	}
	printf("\n");
}
