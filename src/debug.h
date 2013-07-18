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



