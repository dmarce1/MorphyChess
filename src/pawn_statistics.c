#include "board.h"
#include "bits.h"
#include "move_data.h"

typedef struct
{
	uqword lanes;
	uqword moves;
	uqword weak;
} pawn_statistics_t;
