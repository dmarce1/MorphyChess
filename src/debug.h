
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
#ifndef __DEBUG_H
#define __DEBUG_H

#include "board.h"
#include <stddef.h>

#ifndef NDEBUG

#define assert( b )              __assert( (b), __FILE__, __LINE__ )
#define assert_range( l, v, h )  assert( ((v) >= (l)) && ((v) <= (h)) )
#define assert_board_consistency( b ) __assert_board_consistency( (b), __FILE__, __LINE__ )
#define assert_transposition_key_consistency( b ) __assert_transposition_key_consistency( (b), __FILE__, __LINE__ )
#define debug_register_board( b ) __debug_register_board( b ) 

#else /*NDEBUG*/

#define assert( b )
#define assert_range( l, v, h )
#define assert_board_consistency( b )
#define assert_transposition_key_consistency( b )
#define debug_register_board( b )

#endif /*NDEBUG */

void __assert( int, const char*, int );
void __assert_board_consistency( board_t*, const char*, int );
void __assert_transposition_key_consistency( board_t*, const char*, int );
void __debug_register_board( board_t* b );


#ifndef IN_DEBUG_C_FILE
extern int debug_mode;
#endif

#endif /*__DEBUG_H*/



