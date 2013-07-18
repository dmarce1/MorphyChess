
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
#include "debug.h"
#include "board.h"
#include "pgn.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct
{
	char name[ 256 ];
	char data[ 256 ];
} raw_tag_t;


static bool is_result_token( const char* );
static bool text_to_pgn( pgn_t*, board_t* );
static char* remove_raw_tags( char*, raw_tag_t* );
static char* remove_comments( char* );
static int text_to_movetext( char*, movetext_t* );
static void process_tags( raw_tag_t*, pgn_t*, char*  );
static char* next_pgn_text( char* );

void init_pgn( pgn_t* pgn )
{
	pgn->stream = NULL;
	pgn->stream_ptr = NULL;
}

void destroy_pgn( pgn_t* pgn )
{
	if( pgn->stream != NULL )
	{
		free( pgn->stream );
	}
}

bool next_pgn( pgn_t* pgn, board_t* board  )
{
	bool rc;
	char* next_ptr;
	if( pgn->stream == NULL )
	{
		rc = false;
	}
	else
	{
		next_ptr = next_pgn_text( pgn->stream_ptr );
		rc = text_to_pgn( pgn, board );
		pgn->stream_ptr = next_ptr;
		if( pgn->stream_ptr == NULL )
		{
			free( pgn->stream );
			pgn->stream = NULL;
			rc = false;
		}
		rc = true;
	}
	return rc;	
}


bool load_pgn( pgn_t* pgn, const char* filename, board_t* board  )
{
	struct stat s;
	bool rc;
	int i;
	FILE* fp;

	i = stat( filename, &s );
	if( i == 0 )
	{
		if( pgn->stream_ptr != NULL )
		{
			free( pgn->stream );
		}
		pgn->stream_ptr = pgn->stream = (char*) malloc( sizeof( char ) * (s.st_size + 1) );
		assert( pgn->stream != NULL );
		fp = fopen( filename, "rt" );
		if( fp != NULL )
		{
			for( i = 0; i < s.st_size; i++ )
			{
				pgn->stream[ i ] = fgetc( fp );
			}
			fclose( fp );
		}
		else
		{
			assert( false );
		}
		pgn->stream[ s.st_size ] = '\0';
		rc = next_pgn( pgn, board );
	}
	else
	{
		rc = false;
	}
	return rc;
}



static bool text_to_pgn( pgn_t* pgn, board_t* board )
{
	const int N = strlen( pgn->stream_ptr ) + 1;
	bool rc;
	char* _text;
	char* text;
	int i;
	movetext_t* move_text;
	raw_tag_t* raw_tags;

	board_destroy( board );
	board_create( board );
	_text = pgn->stream_ptr;
	text = (char*) malloc( sizeof( char ) * N );	
	raw_tags = (raw_tag_t*) malloc( max_tags * sizeof( raw_tag_t ) );
	move_text = (movetext_t*) malloc( max_half_moves * sizeof( movetext_t ) );
	assert( text != NULL );
	assert( board != NULL );
	assert( raw_tags != NULL );
	assert( move_text != NULL );
	board_create( board );
	strcpy( text, _text );
	remove_comments( text );
	remove_raw_tags( text, raw_tags );
	process_tags( raw_tags, pgn, text );
	text_to_movetext( text, move_text );
	rc = true;
	for( i = 0; (move_text[ i ][ 0 ] != '\0') && (i < max_half_moves); i++ )
	{
		pgn->moves[ i ] = board_make_text_move( board,  move_text[ i ] );
		if( debug_mode )
		{
			//fprintf( stdout, "%s\n", move_text[ i ] );
//			board_to_stream( board, stdout );
		}
		if( move_is_empty( pgn->moves[ i ] ) )
		{
			rc = false;
			break;
		}
	}
	free( text );
	free( raw_tags );
	free( move_text );
	return rc;
}


static void process_tags( raw_tag_t* tags, pgn_t* pgn, char* text )
{
	int i;
	i = 0;
	pgn->event[ 0 ] = pgn->site[ 0 ] = pgn->white_name[ 0 ] = pgn->black_name[ 0 ] = '\0';
	pgn->white_elo = pgn->black_elo = pgn->year = pgn->month = pgn->day = pgn->round = - 1;
	pgn->result = INVALID;
	
	while( tags[ i ].name[ 0 ] != '\0' && i <= max_tags )
	{
		if( strncmp( tags[ i ].name, "Event", 5 ) == 0 )
		{
			strcpy( pgn->event, tags[ i ].data );
		}
		else if( strncmp( tags[ i ].name, "Site", 4 ) == 0 )
			{
				strcpy( pgn->site, tags[ i ].data );
			}
		else if( strncmp( tags[ i ].name, "ECO", 3 ) == 0 )
			{
				strncpy( pgn->eco, tags[ i ].data, 3 );
			}
			else if( strncmp( tags[ i ].name, "Date", 4 ) == 0 )
		{
			tags[ i ].data[ 4 ] = tags[ i ].data[ 7 ] = tags[ i ].data[ 10 ] = '\0';
			if( tags[ i ].data[ 0 ] != '?' )
			{
				pgn->year = atoi( tags[ i ].data );
			}
			if( tags[ i ].data[ 5 ] != '?' )
			{
				pgn->month = atoi( tags[ i ].data + 5 );
			}
			if( tags[ i ].data[ 8 ] != '?' )
			{
				pgn->day = atoi( tags[ i ].data + 8 );
			}
		}
		else if( strncmp( tags[ i ].name, "WhiteElo", 8 ) == 0 )
		{
			pgn->white_elo = atoi( tags[ i ].data );
		}
		else if( strncmp( tags[ i ].name, "BlackElo", 8 ) == 0 )
		{
			pgn->black_elo = atoi( tags[ i ].data );
		}
		else if( strncmp( tags[ i ].name, "White", 5 ) == 0 )
		{
			strcpy( pgn->white_name, tags[ i ].data );
		}
		else if( strncmp( tags[ i ].name, "Black", 5 ) == 0 )
		{
			strcpy( pgn->black_name, tags[ i ].data );
		}
		else if( strncmp( tags[ i ].name, "Result", 6 ) == 0 )
		{
			if( strncmp( tags[ i ].data, "1-0", 3 ) == 0 )
			{
				pgn->result = WHITE_WIN;
			}
			else if( strncmp( tags[ i ].data, "0-1", 3 ) == 0 )
			{
				pgn->result = BLACK_WIN;
			}
			else if( strncmp( tags[ i ].data, "1/2-1/2", 7 ) == 0 )
			{
				pgn->result = DRAW;
			}
			else if( tags[ i ].data[ 0 ] == '*' )
			{
				pgn->result = IN_PROGRESS;
			}
		}
		else if( strncmp( tags[ i ].name, "Round", 5 ) == 0 )
		{
			pgn->round = atoi( tags[ i ].data );	
		}
		i++;
	}
}


static bool is_result_token( const char* token )
{
	bool rc;
	if( strcmp( token, "1-0" ) == 0 || strcmp( token, "0-1" ) == 0 || 
       strcmp( token, "1/2-1/2" ) == 0 || strcmp( token, "*" ) == 0 ) 
	{
		rc = true;
	}
	else
	{
		rc = false;
	}
	return rc;
}


static int text_to_movetext( char* text, movetext_t* mt )
{
	char str[ 256 ];	
	char* str_ptr;
	char* text_ptr;
	int cnt;

	text_ptr = text;
	cnt = 0;
	while( *text_ptr != '\0' )
	{
		while( isspace( *text_ptr ) && *text_ptr != '\0' )
		{
			text_ptr++;
		}
		str_ptr = str;
		while( !isspace( *text_ptr ) && *text_ptr != '\0'  && *text_ptr != '.' )
		{
			*str_ptr = *text_ptr;
			str_ptr++;
			text_ptr++;
		}
		if( *text_ptr != '.' )
		{
			*str_ptr = '\0';
			if( (str[ strlen( str ) - 1 ] != '.') && !is_result_token( str ) )
			{
				strncpy( mt[ cnt ], str, 8 );
				cnt++;
			}
		}
		else
		{
			text_ptr++;
		}
	}
	strcpy( mt[ cnt ], "\0" );
	return cnt;
}


static char* remove_comments( char* text )
{
	int i, j;
	const int N = strlen( text ) + 1;
	bool in_comment = false;
	bool in_line_comment = false;
	j = 0;
	for( i = 0; i < N; i++ )
	{
		if( in_line_comment )	
		{
			if( text[ i ] == '\n' )
			{
				in_line_comment = false;
				text[ j ] = '\n';
				j++;
			}
		}
		else if( in_comment )
		{
			if( text[ i ] == '}' )
			{
				in_comment = false;
			}
			else if( text[ i ] == '\n' )
			{
				text[ j ] = '\n';
				j++;
			}
		}
		else
		{
			if( text[ i ] == ';' )
			{
				in_line_comment = true;
			}
			else if( text[ i ] == '{' )
			{
				in_comment = true;
			}
			else
			{
				text[ j ] = text[ i ];
				j++;
			}
		}
	}
	return text;
}


static char* remove_raw_tags( char* _text, raw_tag_t* tags )
{
	int i, j, k;
	const int N = strlen( _text ) + 1;
	char* text = (char*) malloc( sizeof( char ) * N );
	int num_tags;
	bool in_quote;
	int last_tag_char = 0;
	strcpy( text, _text );
	j = 0;
	for( i = 0; i < N; i++ )
	{
		if( text[ i ] == ']' )
		{
			text[ i ] = '\0';
			last_tag_char = i;
		}
	}
	for( i = 0; i < N; i++ )
	{
		if( text[ i ] == '[' )
		{
			strcpy( tags[ j ].data, text + i + 1 );
			j++;
			if( j == max_tags )
			{
				break;
			}
		}
	}
	num_tags = j;
	for( i = 0; i < num_tags; i++ )
	{
		k = 0;
		in_quote = false;
		for( j = 0; j < 256; j++ )
		{
			if( in_quote )
			{
				if( tags[ i ].data[ j ] == '"' )
				{
					break;
				}
			}
			else
			{
				if( tags[ i ].data[ j ] == '"' )
				{
					in_quote = true;
				}
				else if( !isspace( tags[ i ].data[ j ] ) )
				{
					tags[ i ].name[ k ] = tags[ i ].data[ j ];
					k++;
				}
			}
			if( tags[ i ].data[ j ] == '\n' )
			{
				break;
			}
		}
		in_quote = false;
		k = 0;
		for( j = 0; j < 256; j++ )
		{
			if( in_quote )
			{
				if( tags[ i ].data[ j ] == '"' )
				{
					break;
				}
				tags[ i ].data[ k ] = tags[ i ].data[ j ];
				k++;
			}
			else
			{
				if( tags[ i ].data[ j ] == '"' )
				{
					in_quote = true;
				}
			}
			if( tags[ i ].data[ j ] == '\n' )
			{
				break;
			}
		}
		tags[ i ].data[ k ] = '\0';
	}
	strcpy( _text, text + last_tag_char + 1 );
	free( text );
	if( num_tags != max_tags )
	{
		tags[ num_tags ].name[ 0 ] = '\0';
	}
	return _text;
}


static char* next_pgn_text( char* buffer )
{
	bool moves_section, in_field;
	int len, i;
	char* ptr;

   in_field = false;
	len = strlen( buffer );
	moves_section = false;
	ptr = NULL;
	for( i = 1; i < len; i++ )
	{
      if( buffer[ i ] == '[' ) 
		{
			in_field = true;
			if( moves_section == true )
			{
				buffer[ i - 1 ] = '\0';
				ptr = buffer + i ;
				break;
			}
		}
		else if( buffer[ i ] == ']' )
		{
			in_field = false;
		}
		else if( (buffer[ i ] == '1' && buffer[ i + 1 ] == '.') && !in_field )
		{
			moves_section = true;
		}
	}
	return ptr;
}
