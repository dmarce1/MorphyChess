#include <algorithm>
#include <array>

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

// Fixed-value material helper retained on Board. The search uses the
// configurable StaticEvaluator instead; this helper is White-relative.
constexpr std::array<int, 7> pieceValues = {0, 100, 325, 350, 500, 900, kInfty};
constexpr int maxMaterial = 3100;
constexpr int maxMaterialWoBonus = 2400;
constexpr int startingMaterial =
	16 * pieceValues[pawn] + 4 * pieceValues[knight] + 4 * pieceValues[bishop] +
	4 * pieceValues[rook] + 2 * pieceValues[queen];
constexpr int startingMaterialWoPawns =
	startingMaterial - 16 * pieceValues[pawn];
void initEvaluator() {}

int Board::evaluate() const {
	int score;

	score = evaluateMaterial();

	return score;
}

int Board::evaluateMaterial() const {
	std::array<int, 2> material;
	int materialDifference, totalMaterial;
	int bonus, score, winningSidePawns;
	int i, j;

	for (j = 0; j < 2; j++) {
		material[j] = 0;
	}
	for (i = pawn; i < king; i++) {
		for (j = 0; j < 2; j++) {
			material[j] += pieceCount(j, i) * pieceValues[i];
		}
	}
	materialDifference = material[white] - material[black];
	totalMaterial = material[white] + material[black];
	winningSidePawns = pieceCount(materialDifference > 0 ? white : black, pawn);
	// Favor material exchanges for the leading side while it retains pawns,
	// then limit ordinary scores symmetrically for either color.
	bonus = materialDifference * winningSidePawns *
			(startingMaterial - totalMaterial);
	bonus /= startingMaterialWoPawns * (winningSidePawns + 1);
	score = std::clamp(materialDifference, -maxMaterialWoBonus,
					   maxMaterialWoBonus) +
			bonus;
	score = std::clamp(score, -maxMaterial, maxMaterial);

	return score;
}

} // namespace morphy_chess
