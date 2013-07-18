#ifndef PARALELL_SEARCH_H_
#define PARALELL_SEARCH_H_

#include "search.h"
#include <pthread.h>
#include <sys/timeb.h>

#define WAITING         0 
#define SEARCHING       1
#define FINAL_SEARCHING 2
#define COMPLETE        3

#include "book.h"

typedef struct
{
	search_t* searches;
	pthread_t* threads;
	pthread_mutex_t* lock;
	pthread_mutex_t* main_lock;
	board_t* board;
	int* depths;
	int* test_values;
	int upper_bound[ MAX_DEPTH ];
	int lower_bound[ MAX_DEPTH ];
	int search_status[ MAX_DEPTH ];
	int score;
	move_t best_move[ MAX_DEPTH ];
	int thread_count;
	int node_count;
	int table_hits;
	int grain_size;
	int threads_in_use;
	int initial_guess;
	struct timeb start_time;
	struct timeb end_time;
	int max_depth;
	int kill_time;
	bool search_done;
	move_t root_best_move;
	bool (*action_function)( board_t*, move_t*);
	pthread_t thread_id;
	board_t* leaf_board_ptr;
	bool posting;
	bool in_use;
	bool learning_on;
	FILE* learning_file;
	evaluator_parameters_t eval_params;
	static_evaluator_t* evaluator;
	int null_hits;
	int late_move_reductions;
	int max_time;
	int max_depth_complete;
	int cutoff_at_move_number[ max_moves ]; 
	bool detached;
	bool use_max_time;
	book_t book;
	bool use_book;
	int contempt_factor;
	int computer_color;
} parallel_search_t;

void parallel_search_create( parallel_search_t*, board_t*, static_evaluator_t*, memory_table_t*, int );
int parallel_search_test_driver( parallel_search_t* search, move_t* move, int depth, int );
void parallel_search_detached_test_driver( parallel_search_t* engine, bool (*action_function)( board_t*, move_t*), int max_time );
void parallel_search_reset_memory( parallel_search_t* engine );
void parallel_search_destroy( parallel_search_t* );
void parallel_search_set_kill_time( parallel_search_t*, int );
void parallel_search_kill( parallel_search_t*, bool );
void parallel_search_set_posting( parallel_search_t*, bool );
void parallel_search_set_board( parallel_search_t* engine, board_t* board );
void parallel_search_make_move_on_boards( parallel_search_t*, move_t );
void parallel_search_trigger_learning( parallel_search_t*, int, result_t );
void parallel_search_set_learning( parallel_search_t*, bool );
void parallel_search_undo_move_on_boards( parallel_search_t* );
bool parallel_search_in_use( parallel_search_t* );
void parallel_search_set_use_max_time( parallel_search_t*, bool );
void parallel_search_set_contempt_factor( parallel_search_t*, int );
void parallel_search_set_computer_color( parallel_search_t*, int );
void parallel_search_set_book( parallel_search_t*, bool );

#endif /*parallel_SEARCH_H_*/
