//#define PAWN_TABLES_OFF
//#define TABLES_OFF
//#define LATE_MOVE_REDUCTION_OFF
//#define BOOK_OFF
//#define BOOK_LEARNING_OFF
//#define BOOK_TOOL
#define NOSPEEDLIMIT

/****************************************
0.0.0 - Original release
0.0.1 - Fixed bug in book move display on Xboard
0.0.2 - Fixed bug in verified null move heuristic
 	  - Fixed bug in material evaluation
0.0.3 - Changed REDUCTION_LIMIT to 2
      - Fixed bug with opening book display
1.0.0a - Fixed bug with draw by repetition
       - Enabled engine to decide when to resign, or offer/accept draws
       - added bonuses for pieces based on total pawns on board
       - engine now moves immediately when there is only one legal move
1.0.1a - fixed bug with pondering
1.0.2a - added a "contempt factor" component, which discourages drawing with lower rated opponents
         and encourages drawing against higher rated opponents
1.0.3X - corrected bug with memory tables and made memory table reads more efficient
      - made repeated position draws detectable after the 1st repetition
      - penalty added for side in the lead which increases as the 50 move rule is approached
      - adjusted history branch factor to 3.5 - 1, 3, 12, 42, 150, 525, 1838, 6433, 22518, 78815, 275854, 965491, 3379220, 11827271, 41395491, 144884079, 507094277, 1774829971
      - added more sophisticated book learning
1.0.4 - Added extensions for pawn promotions and moves to the 7th rank
	  - Added futility pruning
1.0.5 - Corrected severe null move bug which allowed all verifications to return true.
 	  - removed futility pruning
      
******************************************************/

#ifndef THREAD_COUNT
#define THREAD_COUNT                  1
#endif
#define MIN_DEPTH                     4
#define FULL_DEPTH_MOVES              4
#define REDUCTION_LIMIT               2
#define FUTILITY_MARGIN               300
#define MAX_DEPTH                     64
#define MAX_SEARCH_DEPTH              32
#define INFTY                         100000
#define MIN_CHECKMATE_VALUE           (10000 - (5 * MAX_DEPTH))
#define DEFAULT_HISTORY_BRANCH_FACTOR 4.0
#define NULL_REDUCTION                3
#define DEFAULT_GRAIN_SIZE            8
#define DEFAULT_POLL_FREQUENCY        1024
#define NAME                          "Morphy Chess 1.0.5"
#define LEARNING_ALPHA                100.0
#define TABLE_SIZE                    (8*1024*1024)
#define TABLE_LOCKS_PER_THREAD        16
#define PAWN_TABLE_SIZE               1024
#define PAWN_TABLE_LOCKS_PER_THREAD   16
#define NUM_BENCHMARKS                10
#define BENCH_TIME                    60
#define BOOK_SIZE                     4096
#define TIME_CUT1                       0
#define TIME_CUT2                    (-50)
#define TIME_CUT3                   (-100)
#define TIME_CUT4                   (-200)
#define RESIGN_LIMIT                  900
#define MIN_HALFMOVES4DRAW            100
#define DRAW_OFFER_FREQ                10
#define CONTEMPT_LEVEL                 25
#define DRAW_TIME_LIMIT               60
#define BOOK_NAME                     "book.bin"
#define INITIAL_BOOK_RATING           2000
#define STARTUP_WAIT                   10
#define SPEED_LIMIT					   200
