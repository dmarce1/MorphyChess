#include <morphychess/bits.hpp>
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

static std::array<std::array<Bitboard, 4>, 64> maskSet;
static std::array<std::array<Bitboard, 4>, 64> invMaskSet;

static constexpr std::array<std::array<std::int8_t, 64>, 4> rotatedIndex = {
	{{0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
	  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
	 {0, 8,	 16, 24, 32, 40, 48, 56, 1, 9,	17, 25, 33, 41, 49, 57,
	  2, 10, 18, 26, 34, 42, 50, 58, 3, 11, 19, 27, 35, 43, 51, 59,
	  4, 12, 20, 28, 36, 44, 52, 60, 5, 13, 21, 29, 37, 45, 53, 61,
	  6, 14, 22, 30, 38, 46, 54, 62, 7, 15, 23, 31, 39, 47, 55, 63},
	 {
		 0,	 8,	 16, 24, 32, 40, 48, 56, 57, 1,	 9,	 17, 25, 33, 41, 49,
		 50, 58, 2,	 10, 18, 26, 34, 42, 43, 51, 59, 3,	 11, 19, 27, 35,
		 36, 44, 52, 60, 4,	 12, 20, 28, 29, 37, 45, 53, 61, 5,	 13, 21,
		 22, 30, 38, 46, 54, 62, 6,	 14, 15, 23, 31, 39, 47, 55, 63, 7,
	 },
	 {56, 48, 40, 32, 24, 16, 8,  0,  49, 41, 33, 25, 17, 9,  1,  57,
	  42, 34, 26, 18, 10, 2,  58, 50, 35, 27, 19, 11, 3,  59, 51, 43,
	  28, 20, 12, 4,  60, 52, 44, 36, 21, 13, 5,  61, 53, 45, 37, 29,
	  14, 6,  62, 54, 46, 38, 30, 22, 7,  63, 55, 47, 39, 31, 23, 15}}};

void initBits(void) {
	static std::once_flag initialized;
	std::call_once(initialized, [] {
		int k, j;
		for (j = 0; j < 64; j++) {
			for (k = 0; k < 4; k++) {
				maskSet[j][k] = Bitboard::mask(rotatedIndex[k][j]);
				invMaskSet[j][k] = ~maskSet[j][k];
			}
		}
	});
}

RotatedBitboard::RotatedBitboard() { initBits(); }

RotatedBitboard::RotatedBitboard(Bitboard value) :
	RotatedBitboard() {
	assign(value);
}

void RotatedBitboard::clear(int i) {
	boards[0] &= invMaskSet[i][0];
	boards[1] &= invMaskSet[i][1];
	boards[2] &= invMaskSet[i][2];
	boards[3] &= invMaskSet[i][3];
}

void RotatedBitboard::set(int i) {
	boards[0] |= maskSet[i][0];
	boards[1] |= maskSet[i][1];
	boards[2] |= maskSet[i][2];
	boards[3] |= maskSet[i][3];
}

void RotatedBitboard::assign(Bitboard q) {
	reset();
	while (q) {
		const int square = q.removeFirstOne();
		set(square);
	}
}

int RotatedBitboard::indexFromRank(int i) const {
	int j;
	j = 8 * (i / 8);
	return static_cast<int>((boards[kRankIndex] >> j).value() & 0xFF);
}

int RotatedBitboard::indexFromFile(int i) const {
	int j;
	j = 8 * (rotatedIndex[kFileIndex][i] / 8);
	return static_cast<int>((boards[kFileIndex] >> j).value() & 0xFF);
}

int RotatedBitboard::indexFromDiag1(int i) const {
	int j;
	j = 8 * (rotatedIndex[kDiag1Index][i] / 8);
	return static_cast<int>((boards[kDiag1Index] >> j).value() & 0xFF);
}

int RotatedBitboard::indexFromDiag2(int i) const {
	int j;
	j = 8 * (rotatedIndex[kDiag2Index][i] / 8);
	return static_cast<int>((boards[kDiag2Index] >> j).value() & 0xFF);
}

void RotatedBitboard::reset() {
	int i;
	for (i = 0; i < 4; i++) {
		boards[i] = Bitboard{};
	}
}

} // namespace morphy_chess
