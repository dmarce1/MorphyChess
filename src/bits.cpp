#include <bit>
#include <morphychess/bits.hpp>
#include <morphychess/board.hpp>
#include <morphychess/debug.hpp>
#include <mutex>

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

U64 maskSet[64][4];
static U64 invMaskSet[64][4];
static int firstOneTable[128];

static int invFirstOneTable[64] = {
	96, 53,	 95, 51,  91,  43,	75, 76, 78,	 82, 90,  41,  8,  5,  127, 52,
	93, 47,	 83, 92,  45,  79,	84, 94, 49,	 87, 35,  124, 46, 81, 88,	37,
	0,	117, 32, 118, 34,  122, 42, 10, 9,	 7,	 3,	  123, 44, 77, 80,	86,
	33, 120, 38, 2,	  121, 40,	6,	1,	119, 36, 126, 50,  89, 39, 4,	125};

static const I8 rotatedIndex[4][64] = {
	{0,	 1,	 2,	 3,	 4,	 5,	 6,	 7,	 8,	 9,	 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
	{0, 8,	16, 24, 32, 40, 48, 56, 1, 9,  17, 25, 33, 41, 49, 57,
	 2, 10, 18, 26, 34, 42, 50, 58, 3, 11, 19, 27, 35, 43, 51, 59,
	 4, 12, 20, 28, 36, 44, 52, 60, 5, 13, 21, 29, 37, 45, 53, 61,
	 6, 14, 22, 30, 38, 46, 54, 62, 7, 15, 23, 31, 39, 47, 55, 63},
	{
		0,	8,	16, 24, 32, 40, 48, 56, 57, 1,	9,	17, 25, 33, 41, 49,
		50, 58, 2,	10, 18, 26, 34, 42, 43, 51, 59, 3,	11, 19, 27, 35,
		36, 44, 52, 60, 4,	12, 20, 28, 29, 37, 45, 53, 61, 5,	13, 21,
		22, 30, 38, 46, 54, 62, 6,	14, 15, 23, 31, 39, 47, 55, 63, 7,
	},
	{56, 48, 40, 32, 24, 16, 8,	 0,	 49, 41, 33, 25, 17, 9,	 1,	 57,
	 42, 34, 26, 18, 10, 2,	 58, 50, 35, 27, 19, 11, 3,	 59, 51, 43,
	 28, 20, 12, 4,	 60, 52, 44, 36, 21, 13, 5,	 61, 53, 45, 37, 29,
	 14, 6,	 62, 54, 46, 38, 30, 22, 7,	 63, 55, 47, 39, 31, 23, 15}};

int bitsCountOnes(U64 data) { return std::popcount(data); }

int bitsFirstOne(U64 data) { return std::countr_zero(data); }

int bitsLastOne(U64 data) { return 63 - std::countl_zero(data); }

int bitsRemoveFirstOne(U64 *data) {
	int i;

	assert(data != NULL);

	i = bitsFirstOne(*data);
	*data &= *data - 1;
	return i;
}

int bitsRemoveLastOne(U64 *data) {
	int i;

	assert(data != NULL);

	i = bitsLastOne(*data);
	if (i >= 0) {
		bitsClear(data, i);
	}
	return i;
}

void bitsHorizontalFlip(U64 *data) {
	U64 bb;

	assert(data != NULL);

	bb = ((*data & 0xF0F0F0F0F0F0F0F0LL) >> 4);
	*data = bb | ((*data & 0x0F0F0F0F0F0F0F0FLL) << 4);
	bb = ((*data & 0xCCCCCCCCCCCCCCCCLL) >> 2);
	*data = bb | ((*data & 0x3333333333333333LL) << 2);
	bb = ((*data & 0xAAAAAAAAAAAAAAAALL) >> 1);
	*data = bb | ((*data & 0x5555555555555555LL) << 1);
}

void bitsVerticalFlip(U64 *data) {
	U64 bb;

	assert(data != NULL);

	bb = ((*data & 0xFFFFFFFF00000000LL) >> 32);
	*data = bb | ((*data & 0x00000000FFFFFFFFLL) << 32);
	bb = ((*data & 0xFFFF0000FFFF0000LL) >> 16);
	*data = bb | ((*data & 0x0000FFFF0000FFFFLL) << 16);
	bb = ((*data & 0xFF00FF00FF00FF00LL) >> 8);
	*data = bb | ((*data & 0x00FF00FF00FF00FFLL) << 8);
}

void initBits(void) {
	static std::once_flag initialized;
	std::call_once(initialized, [] {
		int k, j;
		for (j = 0; j < 64; j++) {
			firstOneTable[invFirstOneTable[j]] = j;
			for (k = 0; k < 4; k++) {
				maskSet[j][k] = U64{1} << rotatedIndex[k][j];
				invMaskSet[j][k] = ~maskSet[j][k];
			}
		}
	});
}

void rotatedBitsClear(U64 *q, int i) {
	q[0] &= invMaskSet[i][0];
	q[1] &= invMaskSet[i][1];
	q[2] &= invMaskSet[i][2];
	q[3] &= invMaskSet[i][3];
}

void rotatedBitsSet(U64 *q, int i) {
	q[0] |= maskSet[i][0];
	q[1] |= maskSet[i][1];
	q[2] |= maskSet[i][2];
	q[3] |= maskSet[i][3];
}

void rotatedBitsCreateSet(U64 *set, U64 q) {
	int i, j;

	for (j = 0; j < 4; j++) {
		set[j] = 0LL;
	}
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 4; j++) {
			if (bitsTest(q, i)) {
				rotatedBitsSet(set, i);
			}
		}
	}
}

int rotatedIndexFromRank(const U64 *o, int i) {
	int j;
	j = 8 * (i / 8);
	return (o[kRankIndex] >> j) & 0xFF;
}

int rotatedIndexFromFile(const U64 *o, int i) {
	int j;
	j = 8 * (rotatedIndex[kFileIndex][i] / 8);
	return (o[kFileIndex] >> j) & 0xFF;
}

int rotatedIndexFromDiag1(const U64 *o, int i) {
	int j;
	j = 8 * (rotatedIndex[kDiag1Index][i] / 8);
	return (o[kDiag1Index] >> j) & 0xFF;
}

int rotatedIndexFromDiag2(const U64 *o, int i) {
	int j;
	j = 8 * (rotatedIndex[kDiag2Index][i] / 8);
	return (o[kDiag2Index] >> j) & 0xFF;
}

void bitsClear(U64 *q, int i) { *q &= ~(U64{1} << i); }

void bitsSet(U64 *q, int i) { *q |= (U64{1} << i); }

bool bitsTest(U64 q, int i) {
	bool rc;
	rc = ((q & (U64{1} << i)) != 0LL);
	return rc;
}

U64 bitsMask(int i) { return (U64{1} << i); }

void rotatedBitsZeroOut(U64 *q) {
	int i;
	for (i = 0; i < 4; i++) {
		q[i] = 0x0LL;
	}
}

} // namespace morphy_chess
