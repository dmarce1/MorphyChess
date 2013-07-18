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
