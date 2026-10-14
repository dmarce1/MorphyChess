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

static std::array<Bitboard, 64> wpAttacks;
static std::array<Bitboard, 64> bpAttacks;
static std::array<Bitboard, 64> nAttacks;
static std::array<Bitboard, 64> kAttacks;
static std::array<std::array<Bitboard, 256>, 64> rankAttacks;
static std::array<std::array<Bitboard, 256>, 64> fileAttacks;
static std::array<std::array<Bitboard, 256>, 64> diag1Attacks;
static std::array<std::array<Bitboard, 256>, 64> diag2Attacks;

static Bitboard genWhitePawnAttacks(int i);
static Bitboard genBlackPawnAttacks(int i);
static Bitboard genKingAttacks(int i);
static Bitboard genKnightAttacks(int i);

Bitboard whitePawnAttacks(int i) { return wpAttacks[i]; }

Bitboard blackPawnAttacks(int i) { return bpAttacks[i]; }

Bitboard kingAttacks(int i) { return kAttacks[i]; }

Bitboard knightAttacks(int i) { return nAttacks[i]; }

Bitboard RotatedBitboard::rankMoves(int i) const {
	return rankAttacks[i][indexFromRank(i)];
}

Bitboard RotatedBitboard::fileMoves(int i) const {
	return fileAttacks[i][indexFromFile(i)];
}

Bitboard RotatedBitboard::diag1Moves(int i) const {
	return diag1Attacks[i][indexFromDiag1(i)];
}

Bitboard RotatedBitboard::diag2Moves(int i) const {
	return diag2Attacks[i][indexFromDiag2(i)];
}

Bitboard RotatedBitboard::bishopAttacks(int i) const {

	Bitboard u, q;
	q = diag1Attacks[i][indexFromDiag1(i)];
	u = diag2Attacks[i][indexFromDiag2(i)];
	return q | u;
}

Bitboard RotatedBitboard::queenAttacks(int i) const {
	assertRange(0, i, 63);

	return bishopAttacks(i) | rookAttacks(i);
}

Bitboard RotatedBitboard::rookAttacks(int i) const {
	Bitboard u, q;
	q = rankAttacks[i][indexFromRank(i)];
	u = fileAttacks[i][indexFromFile(i)];
	return q | u;
}

void initMoveData(void) {
	static std::once_flag initialized;
	std::call_once(initialized, [] {
		U8 byte;
		int from, to, rank, file, i, j;
		unsigned int index;
		Bitboard q;
		std::array<RotatedBitboard, 256> oMasks;

		initBits();

		for (from = 0; from < 64; from++) {
			wpAttacks[from] = genWhitePawnAttacks(from);
			bpAttacks[from] = genBlackPawnAttacks(from);
			kAttacks[from] = genKingAttacks(from);
			nAttacks[from] = genKnightAttacks(from);
		}
		for (index = 0; index < 256; index++) {
			auto &o = oMasks[index];
			o.reset();
			for (i = 0; i < 8; i++) {
				if ((index & (1 << i)) != 0) {
					for (j = 0; j < 8; j++) {
						o.set(8 * i + j);
					}
				}
			}
		}

		for (from = 0; from < 8; from++) {
			for (index = 0; index < 256; index++) {
				byte = index;
				q = Bitboard{};
				if (from < 7) {
					for (to = from + 1; to <= 7; to++) {
						q.set(to);
						if ((byte & (1 << to)) != 0) {
							break;
						}
					}
				}
				if (from > 0) {
					for (to = from - 1; to >= 0; to--) {
						q.set(to);
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
				q = Bitboard{};
				rank = from / 8;
				if (rank < 7) {
					for (to = from + 8; to < 64; to += 8) {
						q.set(to);
						if ((byte & (1 << (to / 8))) != 0) {
							break;
						}
					}
				}
				if (rank > 0) {
					for (to = from - 8; to >= 0; to -= 8) {
						q.set(to);
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
				const auto &o = oMasks[i];
				index = o.indexFromDiag1(from);
				q = Bitboard{};
				if (((from / 8) < 7) && ((from % 8) < 7)) {
					for (to = from + 9; to < 64; to += 9) {
						q.set(to);
						if (o[0].test(to)) {
							break;
						} else if (!(((to / 8) < 7) && ((to % 8) < 7))) {
							break;
						}
					}
				}
				if (((from / 8) > 0) && ((from % 8) > 0)) {
					for (to = from - 9; to >= 0; to -= 9) {
						q.set(to);
						if (o[0].test(to)) {
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
				const auto &o = oMasks[i];
				index = o.indexFromDiag2(from);
				q = Bitboard{};
				if (((from / 8) < 7) && ((from % 8) > 0)) {
					for (to = from + 7; to < 64; to += 7) {
						q.set(to);
						if (o[0].test(to)) {
							break;
						} else if (!(((to / 8) < 7) && ((to % 8) > 0))) {
							break;
						}
					}
				}
				if (((from / 8) > 0) && ((from % 8) < 7)) {
					for (to = from - 7; to >= 0; to -= 7) {
						q.set(to);
						if (o[0].test(to)) {
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

static Bitboard genWhitePawnAttacks(int i) {
	const int side = (i & 0x7) >> 2;

	assertRange(0, i, 63);

	Bitboard bb = Bitboard::mask(i);
	bb = (bb << 7) | (bb << 9);
	bb &= ~(side ? fileA : fileH);
	bb.clear(i);
	return bb;
}

static Bitboard genBlackPawnAttacks(int i) {
	const int side = (i & 0x7) >> 2;
	Bitboard bb = Bitboard::mask(i);

	assertRange(0, i, 63);

	bb = (bb >> 7) | (bb >> 9);
	bb &= ~(side ? fileA : fileH);
	bb.clear(i);
	return bb;
}

static Bitboard genKingAttacks(int i) {
	const int side = (i & 0x7) >> 2;
	Bitboard bb = Bitboard::mask(i);

	assertRange(0, i, 63);

	bb |= (bb >> 1) | (bb << 1);
	bb |= (bb >> 8) | (bb << 8);
	bb &= ~(side ? fileA : fileH);
	bb.clear(i);
	return bb;
}

static Bitboard genKnightAttacks(int i) {
	const int side = (i & 0x7) >> 2;
	Bitboard bb = Bitboard::mask(i);
	Bitboard bb1;

	assertRange(0, i, 63);

	bb1 = (bb >> 6);
	bb1 |= (bb >> 10);
	bb1 |= (bb >> 15);
	bb1 |= (bb >> 17);
	bb1 |= (bb << 6);
	bb1 |= (bb << 10);
	bb1 |= (bb << 15);
	bb1 |= (bb << 17);
	bb1 &= ~(side ? (fileA | fileB) : (fileH | fileG));
	bb1.clear(i);
	return bb1;
}

} // namespace morphy_chess
