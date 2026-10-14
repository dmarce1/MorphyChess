#include <algorithm>
#include <morphychess/evaluate.hpp>
#include <morphychess/params.hpp>

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

using std::min;

constexpr int pieceValues[7] = {0, 100, 325, 350, 500, 900, kInfty};
constexpr int maxMaterial = 3100;
constexpr int maxMaterialWoBonus = 2400;
constexpr int startingMaterial =
	16 * pieceValues[pawn] + 4 * pieceValues[knight] + 4 * pieceValues[bishop] +
	4 * pieceValues[rook] + 2 * pieceValues[queen];
constexpr int startingMaterialWoPawns =
	startingMaterial - 16 * pieceValues[pawn];
static int evaluateMaterial(Board *board);
void initEvaluator() {}

int evaluate(Board *b) {
	int score;

	score = evaluateMaterial(b);

	return score;
}

int evaluateMaterial(Board *b) {
	int material[2], materialDifference, totalMaterial;
	int bonus, score, winningSidePawns;
	int i, j;

	for (j = 0; j < 2; j++) {
		material[j] = 0;
	}
	for (i = pawn; i < king; i++) {
		for (j = 0; j < 2; j++) {
			material[j] += b->pieceCount(j, i) * pieceValues[i];
		}
	}
	materialDifference = material[white] - material[black];
	totalMaterial = material[white] + material[black];
	winningSidePawns =
		b->pieceCount(materialDifference > 0 ? white : black, pawn);
	bonus = materialDifference * winningSidePawns *
			(startingMaterial - totalMaterial);
	bonus /= startingMaterialWoPawns * (winningSidePawns + 1);
	score = min(materialDifference, maxMaterialWoBonus) + bonus;
	score = min(score, maxMaterial);

	return score;
}

} // namespace morphy_chess
