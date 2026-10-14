#pragma once

#include <morphychess/integers.hpp>
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

void rotatedBitsCreateSet(U64 *, U64);
void initBits(void);
int bitsCountOnes(U64);
int bitsFirstOne(U64);
int bitsLastOne(U64);
int bitsRemoveFirstOne(U64 *);
int bitsRemoveLastOne(U64 *);
void bitsVerticalFlip(U64 *);
void bitsHorizontalFlip(U64 *);
void bitsClear(U64 *q, int i);
void bitsSet(U64 *q, int i);
bool bitsTest(U64 q, int i);

U64 bitsMask(int i);
void rotatedBitsClear(U64 *q, int i);
void rotatedBitsSet(U64 *q, int i);
void rotatedBitsZeroOut(U64 *q);
int rotatedIndexFromRank(const U64 *, int);
int rotatedIndexFromFile(const U64 *, int);
int rotatedIndexFromDiag1(const U64 *, int);
int rotatedIndexFromDiag2(const U64 *, int);

} // namespace morphy_chess
