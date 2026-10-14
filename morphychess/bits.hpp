#pragma once

#include <morphychess/bitboard.hpp>
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

inline constexpr auto kRankIndex = 0;
inline constexpr auto kFileIndex = 1;
inline constexpr auto kDiag1Index = 2;
inline constexpr auto kDiag2Index = 3;

void rotatedBitsCreateSet(Bitboard *, Bitboard);
void initBits(void);
void rotatedBitsClear(Bitboard *q, int i);
void rotatedBitsSet(Bitboard *q, int i);
void rotatedBitsZeroOut(Bitboard *q);
int rotatedIndexFromRank(const Bitboard *, int);
int rotatedIndexFromFile(const Bitboard *, int);
int rotatedIndexFromDiag1(const Bitboard *, int);
int rotatedIndexFromDiag2(const Bitboard *, int);

} // namespace morphy_chess
