#include <array>
#include <morphychess/board.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/zobrist.hpp>
#include <mutex>
#include <stdlib.h>

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

static std::array<std::array<std::array<std::uint64_t, 64>, 6>, 2> keys;
static std::array<std::uint64_t, 8> epKeys;
static std::array<std::uint64_t, 2> kingCastleKeys;
static std::array<std::uint64_t, 2> queenCastleKeys;

static std::uint64_t rand64(void);

void initZobristKeys(void) {
	static std::once_flag initialized;
	std::call_once(initialized, [] {
		int i, j, k;
		srand(0x33333333);
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 6; j++) {
				for (k = 0; k < 64; k++) {
					keys[i][j][k] = rand64();
				}
			}
		}
		for (i = 0; i < 8; i++) {
			epKeys[i] = rand64();
		}
		for (i = 0; i < 2; i++) {
			kingCastleKeys[i] = rand64();
			queenCastleKeys[i] = rand64();
		}
	});
}

std::uint64_t zobristKey(int piece, int square) {
	int pieceIndex, color;
	std::uint64_t key;

	if (piece == empty) {
		key = 0LL;
	} else {
		pieceIndex = abs(piece) - 1;
		color = (1 - (abs(piece) / piece)) >> 1;
		key = keys[color][pieceIndex][square];
	}
	return key;
}

std::uint64_t zobristKingSideCastleKey(int color) {
	assertRange(0, color, 1);
	return kingCastleKeys[color];
}

std::uint64_t zobristQueenSideCastleKey(int color) {
	assertRange(0, color, 1);
	return queenCastleKeys[color];
}

std::uint64_t zobristEpKey(int square) {
	assertRange(0, square, 64);
	return epKeys[square & 0x7];
}

std::uint64_t computePositionKey(const Board &board) {
	std::uint64_t key;
	int i;

	key = 0LL;
	for (i = 0; i < 64; i++) {
		key ^= zobristKey(board.pieceValue(i), i);
	}
	if (board.flags().whiteKingSide() == 0) {
		key ^= zobristKingSideCastleKey(white);
	}
	if (board.flags().whiteQueenSide() == 0) {
		key ^= zobristQueenSideCastleKey(white);
	}
	if (board.flags().blackKingSide() == 0) {
		key ^= zobristKingSideCastleKey(black);
	}
	if (board.flags().blackQueenSide() == 0) {
		key ^= zobristQueenSideCastleKey(black);
	}
	if (board.flags().epCaptureSquare() != 0) {
		key ^= zobristEpKey(board.flags().epCaptureSquare());
	}
	//	key ^= zobrist_repeat_key( board.flags().repeat_count() );

	return key;
}

std::uint64_t computePawnKey(const Board &board) {
	std::uint64_t key;
	int i;

	key = 0LL;
	for (i = 0; i < 64; i++) {
		if ((abs(board.pieceValue(i)) == pawn) ||
			(abs(board.pieceValue(i)) == king)) {
			key ^= zobristKey(board.pieceValue(i), i);
		}
	}
	if (board.flags().epCaptureSquare() != 0) {
		key ^= zobristEpKey(board.flags().epCaptureSquare());
	}

	return key;
}

static std::uint64_t rand64(void) {
	std::uint64_t n;
	int n1, n2, n3, n4;
	n1 = rand() & 0xFFFF;
	n2 = rand() & 0xFFFF;
	n3 = rand() & 0xFFFF;
	n4 = rand() & 0xFFFF;
	n = (std::uint64_t)n1 | ((std::uint64_t)n2 << 16) |
		((std::uint64_t)n3 << 32) | ((std::uint64_t)n4 << 48);
	return n;
}

std::uint64_t zobristRepeatKey(int repeatCount) {
	std::uint64_t q;
	if (repeatCount != 0) {
		q = (std::uint64_t)repeatCount;
	} else {
		q = 0;
	}
	return q;
}

} // namespace morphy_chess
