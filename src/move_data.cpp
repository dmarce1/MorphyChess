#include <morphychess/debug.hpp>
#include <morphychess/move_data.hpp>
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

static U64 wpAttacks[64];
static U64 bpAttacks[64];
static U64 nAttacks[64];
static U64 kAttacks[64];
static U64 rankAttacks[64][256];
static U64 fileAttacks[64][256];
static U64 diag1Attacks[64][256];
static U64 diag2Attacks[64][256];

static U64 genWhitePawnAttacks(int i);
static U64 genBlackPawnAttacks(int i);
static U64 genKingAttacks(int i);
static U64 genKnightAttacks(int i);

U64 whitePawnAttacks(int i) { return wpAttacks[i]; }

U64 blackPawnAttacks(int i) { return bpAttacks[i]; }

U64 kingAttacks(int i) { return kAttacks[i]; }

U64 knightAttacks(int i) { return nAttacks[i]; }

U64 rankMoves(const U64 *o, int i) {
	return rankAttacks[i][rotatedIndexFromRank(o, i)];
}

U64 fileMoves(const U64 *o, int i) {
	return fileAttacks[i][rotatedIndexFromFile(o, i)];
}

U64 diag1Moves(const U64 *o, int i) {
	return diag1Attacks[i][rotatedIndexFromDiag1(o, i)];
}

U64 diag2Moves(const U64 *o, int i) {
	return diag2Attacks[i][rotatedIndexFromDiag2(o, i)];
}

U64 bishopAttacks(const U64 *o, int i) {

	U64 u, q;
	q = diag1Attacks[i][rotatedIndexFromDiag1(o, i)];
	u = diag2Attacks[i][rotatedIndexFromDiag2(o, i)];
	return q | u;
}

U64 queenAttacks(const U64 *bb, int i) {
	assertRange(0, i, 63);

	return bishopAttacks(bb, i) | rookAttacks(bb, i);
}

U64 rookAttacks(const U64 *o, int i) {
	U64 u, q;
	q = rankAttacks[i][rotatedIndexFromRank(o, i)];
	u = fileAttacks[i][rotatedIndexFromFile(o, i)];
	return q | u;
}

void initMoveData(void) {
	static std::once_flag initialized;
	std::call_once(initialized, [] {
		U8 byte;
		int from, to, rank, file, i, j;
		unsigned int index;
		U64 q;
		U64 oMasks[256][4];
		U64 *o;

		initBits();

		for (from = 0; from < 64; from++) {
			wpAttacks[from] = genWhitePawnAttacks(from);
			bpAttacks[from] = genBlackPawnAttacks(from);
			kAttacks[from] = genKingAttacks(from);
			nAttacks[from] = genKnightAttacks(from);
		}
		for (index = 0; index < 256; index++) {
			o = oMasks[index];
			for (j = 0; j < 4; j++) {
				o[j] = 0LL;
			}
			for (i = 0; i < 8; i++) {
				if ((index & (1 << i)) != 0) {
					for (j = 0; j < 8; j++) {
						rotatedBitsSet(o, 8 * i + j);
					}
				}
			}
		}

		for (from = 0; from < 8; from++) {
			for (index = 0; index < 256; index++) {
				byte = index;
				q = 0LL;
				if (from < 7) {
					for (to = from + 1; to <= 7; to++) {
						bitsSet(&q, to);
						if ((byte & (1 << to)) != 0) {
							break;
						}
					}
				}
				if (from > 0) {
					for (to = from - 1; to >= 0; to--) {
						bitsSet(&q, to);
						if ((byte & (1 << to)) != 0) {
							break;
						}
					}
				}
				for (rank = 0; rank < 8; rank++) {
					rankAttacks[8 * rank + from][index] = q << (8 * rank);
				}
			}
		}
		for (from = 0; from < 64; from += 8) {
			for (index = 0; index < 256; index++) {
				byte = index;
				q = 0LL;
				rank = from / 8;
				if (rank < 7) {
					for (to = from + 8; to < 64; to += 8) {
						bitsSet(&q, to);
						if ((byte & (1 << (to / 8))) != 0) {
							break;
						}
					}
				}
				if (rank > 0) {
					for (to = from - 8; to >= 0; to -= 8) {
						bitsSet(&q, to);
						if ((byte & (1 << (to / 8))) != 0) {
							break;
						}
					}
				}
				for (file = 0; file < 8; file++) {
					fileAttacks[8 * rank + file][index] = q << file;
				}
			}
		}
		for (from = 0; from < 64; from++) {
			for (i = 0; i < 256; i++) {
				o = oMasks[i];
				index = rotatedIndexFromDiag1(o, from);
				q = 0LL;
				if (((from / 8) < 7) && ((from % 8) < 7)) {
					for (to = from + 9; to < 64; to += 9) {
						bitsSet(&q, to);
						if (bitsTest(o[0], to)) {
							break;
						} else if (!(((to / 8) < 7) && ((to % 8) < 7))) {
							break;
						}
					}
				}
				if (((from / 8) > 0) && ((from % 8) > 0)) {
					for (to = from - 9; to >= 0; to -= 9) {
						bitsSet(&q, to);
						if (bitsTest(o[0], to)) {
							break;
						} else if (!(((to / 8) > 0) && ((to % 8) > 0))) {
							break;
						}
					}
				}
				diag1Attacks[from][index] = q;
			}
		}
		for (from = 0; from < 64; from++) {
			for (i = 0; i < 256; i++) {
				o = oMasks[i];
				index = rotatedIndexFromDiag2(o, from);
				q = 0LL;
				if (((from / 8) < 7) && ((from % 8) > 0)) {
					for (to = from + 7; to < 64; to += 7) {
						bitsSet(&q, to);
						if (bitsTest(o[0], to)) {
							break;
						} else if (!(((to / 8) < 7) && ((to % 8) > 0))) {
							break;
						}
					}
				}
				if (((from / 8) > 0) && ((from % 8) < 7)) {
					for (to = from - 7; to >= 0; to -= 7) {
						bitsSet(&q, to);
						if (bitsTest(o[0], to)) {
							break;
						} else if (!(((to / 8) > 0) && ((to % 8) < 7))) {
							break;
						}
					}
				}
				diag2Attacks[from][index] = q;
			}
		}
		return;
	});
}

static U64 genWhitePawnAttacks(int i) {
	const int side = (i & 0x7) >> 2;

	assertRange(0, i, 63);

	U64 bb = (U64)0x00000000000000001LL << i;
	bb = (bb << 7) | (bb << 9);
	bb &= ~(side * fileA);
	bb &= ~((1 - side) * fileH);
	bitsClear(&bb, i);
	return bb;
}

static U64 genBlackPawnAttacks(int i) {
	const int side = (i & 0x7) >> 2;
	U64 bb = (U64)0x00000000000000001LL << i;

	assertRange(0, i, 63);

	bb = (bb >> 7) | (bb >> 9);
	bb &= ~(side * fileA);
	bb &= ~((1 - side) * fileH);
	bitsClear(&bb, i);
	return bb;
}

static U64 genKingAttacks(int i) {
	const int side = (i & 0x7) >> 2;
	U64 bb = (U64)0x00000000000000001LL << i;

	assertRange(0, i, 63);

	bb |= (bb >> 1) | (bb << 1);
	bb |= (bb >> 8) | (bb << 8);
	bb &= ~(side * fileA);
	bb &= ~((1 - side) * fileH);
	bitsClear(&bb, i);
	return bb;
}

static U64 genKnightAttacks(int i) {
	const int side = (i & 0x7) >> 2;
	U64 bb = (U64)0x00000000000000001LL << i;
	U64 bb1;

	assertRange(0, i, 63);

	bb1 = (bb >> 6);
	bb1 |= (bb >> 10);
	bb1 |= (bb >> 15);
	bb1 |= (bb >> 17);
	bb1 |= (bb << 6);
	bb1 |= (bb << 10);
	bb1 |= (bb << 15);
	bb1 |= (bb << 17);
	bb1 &= ~(side * (fileA | fileB));
	bb1 &= ~((1 - side) * (fileH | fileG));
	bitsClear(&bb1, i);
	return bb1;
}

} // namespace morphy_chess
