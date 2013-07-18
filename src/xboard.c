#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "seek_bot.h"
#include "board.h"
#include "debug.h"
#include "parallel_search.h"
//#include "genome.h"
//#include "memory.h"
#include "zobrist.h"
#include "book.h"



memory_table_t memory_table;
board_t* board;
bool draw_offered;
bool pondering;
bool xboard_on;
bool illegal_position;
bool analyze_mode;
bool parallel_search_on;
bool ponder_on;
bool force_mode_on;
bool time_to_quit;
double protocol_version;
parallel_search_t parallel_search;
static_evaluator_t* evaluator;
int parallel_search_color;
int depth;
int time_lim;
int parallel_search_time;
int opponent_time;
int engine_rating;
int opponent_rating;
int last_draw_offer;
int contempt_factor;
result_t result;
bool opponent_running_out_of_time;
char last_opponent_name[ 256 ];
char opponent_name[ 256 ];




static void xboard_send_ics_seeks();
static void xboard_send_ics_seeks( void );
static void enter_analyze( void );
static void leave_analyze( void );
static void xboard_process_command( char* );
static void search_stats( void );
static void xboard_check_for_parallel_search_start( void );
static void send_result( board_t* );


#ifdef __ICC
#pragma optimization_level 1
#endif


int interface_run_xboard( void )
{
	char command[ 1024 ];
	static_evaluator_t eval;
	engine_rating = 0;
	opponent_rating = 0;
	contempt_factor = 0;
	seek_bot_t bot;
	

#ifndef BOOK_TOOL	
	memory_table_create( &memory_table, TABLE_SIZE, TABLE_LOCKS_PER_THREAD*THREAD_COUNT, THREAD_COUNT  );
#else
	memory_table_create( &memory_table, 8, TABLE_LOCKS_PER_THREAD*THREAD_COUNT, THREAD_COUNT  );
#endif
	static_evaluator_create( &eval, default_evaluator_parameters );
	last_draw_offer = -1;
	draw_offered = false;
	evaluator = &eval;
	opponent_running_out_of_time = false;
	xboard_on = false;
	parallel_search_on = false;
	analyze_mode = false;
	force_mode_on = true;
	time_to_quit = false;
	depth = MAX_DEPTH;
	time_lim = 10;
	board = (board_t*) malloc( sizeof( board_t ) );
	*opponent_name = *last_opponent_name = '\0';
	if( board == NULL )
	{
		printf( "Error allocating history table\n" );
		printf( "Line %i File %s\n", __LINE__, __FILE__ );
		abort();
	}
	board_create( board );
 	signal(SIGINT, SIG_IGN);
 	result = IN_PROGRESS;
	illegal_position = false;
	ponder_on = true;
	pondering = false;

	parallel_search_create( &parallel_search, board, evaluator, &memory_table, THREAD_COUNT );
	parallel_search_set_learning( &parallel_search, false );
	parallel_search_set_posting( &parallel_search, false );
	setbuf( stdout, NULL);
//	sleep( STARTUP_WAIT );
	//xboard_send_ics_seeks();
	while( !time_to_quit )
	{
		xboard_check_for_parallel_search_start();
		fgets( command, 255, stdin );
		xboard_process_command( command );
	}	
	
	free( board );
	parallel_search_destroy( &parallel_search );
	memory_table_destroy( &memory_table ); 
	return 0;
}

bool xboard_action_func( board_t* board, move_t* branch )
{
	char str[ 256 ];
	int root_score;
	int score;
	bool make_offer;
	str[ 0 ] = branch->from % 8 + 'a';
	str[ 1 ] = branch->from / 8 + '1';
	str[ 2 ] = branch->to % 8 + 'a';
	str[ 3 ] = branch->to / 8 + '1';
	if( branch->promo != empty )
	{
		if( branch->promo == queen )
			str[ 4 ] = 'q';
		else if( branch->promo == rook )
			str[ 4 ] = 'r';
		else if( branch->promo == bishop )
			str[ 4 ] = 'b';
		else if( branch->promo == knight )
			str[ 4 ] = 'n';
		str[ 5 ] = '\0';
	}
	else
	{
		str[ 4 ] = '\0';
	}
	score = parallel_search.score;
	root_score = search_quiescent_value( &parallel_search.searches[0], 0 );
	make_offer = false;
	if( (score < -RESIGN_LIMIT) && (root_score < -RESIGN_LIMIT ) )
	{
		printf( "resign\n" );
		parallel_search_on = false;
	}
	else
	{
		if( (score <= contempt_factor) && !(opponent_running_out_of_time && (opponent_time < parallel_search_time)) )
		{
			if( board->flags.half_move_count >= MIN_HALFMOVES4DRAW )
			{
				if( ((board->flags.half_move_count/2 - DRAW_OFFER_FREQ) >= last_draw_offer) )
				{
					make_offer = true;
					last_draw_offer = board->flags.half_move_count/2;
				}
				else if( draw_offered )
				{
					printf( "accept\n" );
					sleep( 1 );
				}
			}
		}
		printf( "move %s\n", str );
		board_make_move( board, *branch );
		parallel_search_make_move_on_boards( &parallel_search, *branch );
		send_result( board );
		if( make_offer )
			printf( "offer draw\n" );
		draw_offered = false;
	}
	if( ponder_on == true )
		return true;
	else
		return false;
}


static void enter_analyze( void )
{
	force_mode_on = false;
	parallel_search_on = true;
	parallel_search_color = board->flags.side_to_move;
	analyze_mode = true;
}


static void search_stats( void )
{
/*
	printf( 
		"stat01: %i %i %i %i %i %c%i%c%i\n", 
		parallel_search.elapsed_time * 100, 
		parallel_search.total_nodes, 
		parallel_search.current_depth, 
		parallel_search.mvsleft, 
		parallel_search.mvstot, 
		parallel_search.mvconsider.from % 8 + 'a', 
		parallel_search.mvconsider.from / 8 + 1,
		parallel_search.mvconsider.to % 8 + 'a',
		parallel_search.mvconsider.to / 8 + 1
	);*/
}


static void leave_analyze( void )
{
	analyze_mode = false;
	parallel_search_kill( &parallel_search, false );
	parallel_search_on = false;
}


static void xboard_check_for_parallel_search_start( void )
{
	int t, d;

	if( analyze_mode )
	{
		d = MAX_DEPTH;
		t = INFTY;
	}
	else
	{
		d = depth;
		t = 4*parallel_search_time / 30;
	}
	if( !parallel_search_in_use( &parallel_search ) )
	{
		if( parallel_search_on == true )
		{
			if( (parallel_search_color == board->flags.side_to_move) && (board_result( board ) == IN_PROGRESS) )
			{
				parallel_search_set_computer_color( &parallel_search, parallel_search_color );
				parallel_search_detached_test_driver( &parallel_search, xboard_action_func, t );
			}
		}
	}
}


void xboard_send_ics_seeks()
{
	int t, inc, i;
	for( i = 0; i < 3; i++ )
	{
		t = rand() % 18 + 1;
		inc = rand() % 18 + 1;
		printf( "tellics seek %i %i f\n", t, inc );
		if( i != 2 )
			sleep( 1 );		
	}
}


static void xboard_process_command( char* str )
{
	move_t move;
	char* ptr;
	char string[ 256 ];
	int rc;
	int score;
	if( strncmp( str, "?", 1 ) == 0 )
	{
		if( parallel_search_in_use( &parallel_search ) )
		{
			parallel_search_kill( &parallel_search, true );
		}
	}
	else if( strncmp( str, ".", 1 ) == 0 )
	{
		search_stats();
	}
	else if( strncmp( str, "accepted", 8 ) == 0 )
	{
	}
	else if( strncmp( str, "analyze", 7 ) == 0 )
	{
		enter_analyze();
	}
	else if( strncmp( str, "bk", 2 ) == 0 )
	{
		book_show_book_moves( &parallel_search.book, board );
	}
	else if( strncmp( str, "computer", 8 ) == 0 )
	{
	}
	else if( strncmp( str, "draw", 4 ) == 0 )
	{
		draw_offered = true;
	}
	else if( strncmp( str, "easy", 4 ) == 0 )
	{
		ponder_on = false;
	}
	else if( strncmp( str, "exit", 4 ) == 0 )
	{
		leave_analyze();
	}
	else if( strncmp( str, "force", 5 ) == 0 )
	{
		force_mode_on = true;
		parallel_search_on = false;
		if( analyze_mode )
		{
			leave_analyze();
		}
		else
		{
			if( parallel_search_in_use( &parallel_search ) )
				parallel_search_kill( &parallel_search, false );
		}
	}
	else if( strncmp( str, "go", 2 ) == 0 )
	{
		force_mode_on = false;
		parallel_search_on = true;
		parallel_search_color = board->flags.side_to_move;
	}
	else if( strncmp( str, "hard", 4 ) == 0 )
	{
		ponder_on = true;
	}
	else if( strncmp( str, "name", 4 ) == 0 )
	{
		strcpy( last_opponent_name, opponent_name );
		strcpy( opponent_name, str + 5 );
	}
	else if( strncmp( str, "level", 5 ) == 0 )
	{
	}
	else if( strncmp( str, "new", 3 ) == 0 )
	{
		init_zobrist_keys();
		parallel_search_reset_memory( &parallel_search );
		illegal_position = false;
		board_create( board );
		parallel_search_set_board( &parallel_search, board );
		parallel_search_on = true;
		parallel_search_color = black;
		force_mode_on = false;
		depth = MAX_DEPTH;
		result = IN_PROGRESS;
	}
	else if( strncmp( str, "nopost", 6 ) == 0 )
	{
		parallel_search.posting = 0;
	}
	else if( strncmp( str, "otim", 4 ) == 0 )
	{
		opponent_time = atoi( str + 5 ) / 100.0;
		if( opponent_time < DRAW_TIME_LIMIT )
			opponent_running_out_of_time = true;
		else
			opponent_running_out_of_time = false;
	}
	else if( strncmp( str, "pause", 5 ) == 0 )
	{
	}
	else if( strncmp( str, "ping", 4 ) == 0 )
	{
		str[ 1 ] = 'o';
		printf( str );
		fflush( stdout );
	}
	else if( strncmp( str, "playother", 9 ) == 0 )
	{
	}
	else if( strncmp( str, "post", 4 ) == 0 )
	{
		parallel_search.posting = 1;
	}
	else if( strncmp( str, "protover", 8 ) == 0 )
	{
		protocol_version = atof( str + 9 );
		if( protocol_version >= 2.0 )
		{
			printf( "feature analyze=1\n" );
			printf( "feature colors=0\n" );
			printf( "feature done=1\n" );
			printf( "feature draw=1\n" );
			printf( "feature ics=1\n" );
			printf( "feature myname=\"%s\"\n", NAME );
			printf( "feature name=1\n" );
			printf( "feature pause=0\n" );
			printf( "feature ping=1\n" );
			printf( "feature playother=1\n" );
			printf( "feature reuse=1\n" );
			printf( "feature san=0\n" );
			printf( "feature setboard=1\n" );
			printf( "feature sigint=0\n" );
			printf( "feature sigterm=1\n" );
			printf( "feature time=1\n" );
			printf( "feature usermove=0\n" );
			printf( "feature variants=\"normal\"\n" );
			fflush( stdout );
		}
	}
	else if( strncmp( str, "quit", 4 ) == 0 )
	{
		parallel_search_kill( &parallel_search, false );
		time_to_quit = true;
	}
	else if( strncmp( str, "ics", 3 ) == 0 )
	{
 	}
	else if( strncmp( str, "random", 6 ) == 0 )
	{
 	}
	else if( strncmp( str, "rating", 6 ) == 0 )
	{
		sscanf( str, "rating %i %i\n", &engine_rating, &opponent_rating );
		if( (opponent_rating == 0) || (engine_rating == 0) )
			contempt_factor = 0;
		else
			contempt_factor = CONTEMPT_LEVEL * (opponent_rating - engine_rating) / 100;
		parallel_search_set_contempt_factor( &parallel_search, contempt_factor );  
	}
	else if( strncmp( str, "rejected", 8 ) == 0 )
	{
		abort();
	}
	else if( strncmp( str, "remove", 6 ) == 0 )
	{
		board_undo_move( board );
		board_undo_move( board );
	}
	else if( strncmp( str, "result", 6 ) == 0 )
	{
		if( strncmp( str + 7, "1-0", 3 ) == 0 )
			result = WHITE_WIN;
		else if( strncmp( str + 7, "1/2-1/2", 3 ) == 0 )
			result = DRAW;
		else if( strncmp( str + 7, "0-1", 3 ) == 0 )
			result = BLACK_WIN;
		else 
			result = INVALID;
		if( result != INVALID )
		{
			if( (opponent_rating != 0) && (engine_rating != 0) )
				book_learning( &parallel_search.book, board, parallel_search_color, result, opponent_rating );
		}
		if( strcmp( opponent_name, last_opponent_name ) != 0 )
			printf( "tellics rematch\n" );
		xboard_send_ics_seeks();
		
	}
	else if( strncmp( str, "resume", 6 ) == 0 )
	{
	}
	else if( strncmp( str, "setboard", 8 ) == 0 )
	{
		char* fen_str;
		illegal_position = false;
		fen_str = str + 9;
		rc = board_load_FEN( board, fen_str );
		parallel_search_set_board( &parallel_search, board );
		if( rc == false )
		{
			printf( "tellusererror Illegal position\n" );
			fflush( stdout );
			illegal_position = true;
		}
		
	}
	else if( strncmp( str, "sd", 2 ) == 0 )
	{
	}
	else if( strncmp( str, "st", 2 ) == 0 )
	{
	}
	else if( strncmp( str, "time", 4 ) == 0 )
	{
		parallel_search_time = atoi( str + 5 ) / 100.0;
	}
	else if( strncmp( str, "undo", 4 ) == 0 )
	{
		if( analyze_mode )
		{
			leave_analyze();
			board_undo_move( board );
			enter_analyze();
		}
		else
		{
			board_undo_move( board );
			parallel_search_undo_move_on_boards( &parallel_search );
		}
	}
	else if( strncmp( str, "variant", 7 ) == 0 )
	{
	}
	else if( strncmp( str, "xboard", 6 ) == 0 )
	{
		xboard_on = true;
	}
	else
	{
		rc = false;
		ptr = str;
		if( !illegal_position )
		{
			move.from = (ptr[ 0 ] - 'a') + 8 * (ptr[ 1 ] - '1');
			move.to = (ptr[ 2 ] - 'a') + 8 * (ptr[ 3 ] - '1');
			if( tolower( ptr[ 4 ] ) == 'q' )
				move.promo = queen;
			else if( tolower( ptr[ 4 ] ) == 'r' )
				move.promo = rook;
			else if( tolower( ptr[ 4 ] ) == 'b' )
				move.promo = bishop;
			else if( tolower( ptr[ 4 ] ) == 'n' )
				move.promo = knight;
			else
				move.promo = empty;
			if( board_is_legal_move( board, move ) )
			{
				parallel_search_kill( &parallel_search, false );
				rc = board_make_move( board, move );
				parallel_search_make_move_on_boards( &parallel_search, move );
				if( rc )
					send_result( board );
			}
		}
		if( !rc )
		{
			printf( "tellusererror Illegal move\n" );
			fflush( stdout );
		}
	}
}

void send_result( board_t* board )
{
	switch( board_result( board ) )
	{
	case IN_PROGRESS:
	case INVALID:
		break;
	case WHITE_WIN:
		printf( "1-0\n" );
		break;
	case BLACK_WIN:
		printf( "0-1\n" );
		break;
	case DRAW:
		printf( "1/2-1/2\n" );
		break;
	}
	fflush( stdout );
}
