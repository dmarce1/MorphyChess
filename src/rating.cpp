#include <morphychess/rating.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

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

int ratingChangeWin(int engineRating, int opponentRating) {
	const double ratio = std::pow(
		10.0, (static_cast<double>(engineRating) - opponentRating) / 400.0);
	return static_cast<int>(32.0 / (1.0 + ratio) + 0.5);
}

int ratingChangeLoss(int engineRating, int opponentRating) {
	return ratingChangeWin(engineRating, opponentRating) - 32;
}

int ratingChangeDraw(int engineRating, int opponentRating) {
	return ratingChangeWin(engineRating, opponentRating) - 16;
}

int ratingComputeNewRating(int oldRating, int opponentRating, int result,
						   int numGames) {
	if (numGames <= 0) {
		throw std::invalid_argument("numGames must be positive");
	}
	if (result < -1 || result > 1) {
		throw std::invalid_argument("result must be -1, 0, or 1");
	}
	long long rating = oldRating;
	if (numGames <= 20) {
		rating =
			((numGames - 1LL) * oldRating + opponentRating + result * 400LL) /
			numGames;
	} else if (result > 0) {
		rating += ratingChangeWin(oldRating, opponentRating);
	} else if (result < 0) {
		rating += ratingChangeLoss(oldRating, opponentRating);
	} else {
		rating += ratingChangeDraw(oldRating, opponentRating);
	}
	return static_cast<int>(std::clamp(
		rating, static_cast<long long>(std::numeric_limits<int>::min()),
		static_cast<long long>(std::numeric_limits<int>::max())));
}

} // namespace morphy_chess
