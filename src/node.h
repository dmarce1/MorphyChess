
#ifndef __NODE_H
#define __NODE_H

#include "board.h"
#include "params.h"
#include "history.h"
#include "search.h"


typedef enum
{
	START, TABLE, PROMOTIONS, LOSING_CAPTURES, CAPTURES, NONCAPTURES, END
} gen_stage_t;

typedef struct
{
	move_t      list[ max_moves ];
	int         scores[ max_moves / 2 ]; /* THis buffer only used for capture moves */
	gen_stage_t mg_stage;
	search_t*   search;
	int         total_moves_generated;
	int         moves_in_queue;
	move_t      killer;
	move_t      table_move;
	bool        first_noncap;
} node_t;


#define NODE_TERMINAL        1
#define NODE_MOVES_AVAILABLE 2
#define NODE_NO_MORE_MOVES 3
#define MAX_BRANCHES         max_moves
#define CAPTURE_INSTABILITY  0
#define CRITICAL_INSTABILITY 1
#define STABLE               3

void          node_create( node_t*, search_t*, move_t );
void          node_destroy( node_t* );
move_t        node_table_move( node_t* );
int           node_do_next_move( node_t*, move_t* );
int           node_do_next_nonquiescent_move( node_t*, move_t* );
void          node_restore( node_t* );
void          nove_principal_continuation_to_string( char*, move_t*, board_t* );
int           node_total_moves( node_t* node );
bool          node_ok_for_null_ext( node_t* );
bool          node_is_draw( node_t* );
bool          node_no_moves_generated( node_t* );
gen_stage_t   node_generation_stage( node_t* );
bool          node_ok_to_reduce( node_t* search, int depth );
bool          node_move_is_futile( node_t* node, int, int );


#endif
