
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
#include "board.h"
#include "parallel_search.h"
#include "static_evaluator.h"
#include "move_data.h"
#include "memory.h"
#include "zobrist.h"
#include "book.h"
#include <string.h>

int interface_run_xboard( void );
int interface_run_benchmark( void );

int main( int argc, char* argv[] ) 
{
	int rc;
	init_move_data();
	init_zobrist_keys();
	
	printf( "\n" );
	printf( "%s\n", NAME );
#ifdef BOOK_TOOL
	printf( "Book Tool Utility\n" );
#endif
	printf( "Copyright (C) 2007,2008 Dominic C. Marcello\n" );
	printf( "ALL RIGHTS RESERVED\n" );
	printf( "\n" );
	if( argc > 1 )
	{
		if( strcmp( argv[1], "-bench" ) == 0 )
		{
			printf( "sizeof( memory_entry_t ) = %i\n", sizeof( memory_entry_t ) );
			printf( "sizeof( book_t ) = %i\n", sizeof( book_entry_t ) );
			printf( "sizeof( move_t ) = %i\n", sizeof( move_t ) );
			printf( "sizeof( undo_info_t ) = %i\n", sizeof( undo_info_t ) );
			printf( "sizeof( board_flags_t ) = %i\n", sizeof( board_flags_t ) );
			printf( "sizeof( pawn_table_entry_t ) = %i\n", sizeof( pawn_table_entry_t ) );
			rc = interface_run_benchmark();
		}
		else
		{
			rc = interface_run_xboard();
		}
	}
	else
	{
		rc = interface_run_xboard();
	}
	return rc;
}
