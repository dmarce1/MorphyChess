#include <stdlib.h>

#include <morphychess/board.hpp>
#include <morphychess/material_values.hpp>

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

namespace {
constexpr MaterialValues materialScore({0, 100, 300, 300, 500, 900, 1000});
} // namespace

int Board::see(Move move, int score) {
	// Recursive swap-off/SEE: force this move, then follow least-valued legal
	// recaptures on its destination. Negating the accumulated score changes
	// viewpoint; taking the smaller reply lets the opponent stop exchanging.
	// Actual make/undo operations reveal x-ray attackers and enforce king
	// safety. This is a material heuristic, not a full tactical search; see
	// bibliography.md [static_exchange].
	Move reply;
	int replyScore;
	int capture;

	capture = abs(pieceValue(move.to()));
	if (capture == empty && moveIsCapture(move)) {
		// En passant removes a pawn behind the otherwise empty target square.
		capture = pawn;
	}
	if (makeMove(move)) {
		score += materialScore[capture];
		if (!move.isEmpty() && move.promo() != empty) {
			// The promoted piece replaces a pawn even on a quiet move. A later
			// recapture then removes the full promoted-piece value normally.
			score += materialScore[move.promo()] - materialScore[pawn];
		}
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
