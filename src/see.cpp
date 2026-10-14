#include <array>
#include <morphychess/board.hpp>
#include <stdlib.h>

namespace morphy_chess {

/*********************************************************************************
MorphyChess 1.0.5 Copyright (C) 2008 Dominic C. Marcello
(dmarcello@phys.lsu.edu)

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

std::array<int, 7> materialScore = {0, 100, 300, 300, 500, 900, 1000};

int Board::see(Move move, int score) {
	Move reply;
	int replyScore;
	int capture;

	capture = abs(pieceValue(move.to()));
	if (makeMove(move)) {
		score += materialScore[capture];
		if (genLvaAttackToSquare(reply, move.to())) {
			replyScore = -see(reply, -score);
			if (replyScore < score) {
				score = replyScore;
			}
		}
		undoMove();
	}
	return score;
}

} // namespace morphy_chess
