#include <array>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

#include <morphychess/bits.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/params.hpp>
#include <morphychess/static_evaluator.hpp>

namespace morphy_chess {

StaticEvaluator::StaticEvaluator(const EvaluatorParameters &params) {
	configure(params);
}

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

static int
evaluateWhiteKingPawn(const std::array<std::array<int, 10>, 2> &pawnRank,
					  int file);
static int
evaluateBlackKingPawn(const std::array<std::array<int, 10>, 2> &pawnRank,
					  int file);

// Piece-square tables use Board's a1=0, h8=63 indexing. Their entries are
// positional centipawn terms before the configurable percentage weights;
// material values are accounted for separately in materialScore().
constexpr std::array<int, 64> whitePawnSquareValue = {
	0, 0,  0,  0,	0,	 0,	 0,	 0, 0, 0, 0,  -40, -40, 0,	0, 0,
	1, 2,  3,  -10, -10, 3,	 2,	 1, 2, 4, 6,  8,   8,	6,	4, 2,
	3, 6,  9,  12,	12,	 9,	 6,	 3, 4, 8, 12, 16,  16,	12, 8, 4,
	5, 10, 15, 20,	20,	 15, 10, 5, 0, 0, 0,  0,   0,	0,	0, 0};

constexpr std::array<int, 64> blackPawnSquareValue = {
	0, 0, 0,  0,   0,	0,	0, 0, 5, 10, 15, 20,  20,  15, 10, 5,
	4, 8, 12, 16,  16,	12, 8, 4, 3, 6,	 9,	 12,  12,  9,  6,  3,
	2, 4, 6,  8,   8,	6,	4, 2, 1, 2,	 3,	 -10, -10, 3,  2,  1,
	0, 0, 0,  -40, -40, 0,	0, 0, 0, 0,	 0,	 0,	  0,   0,  0,  0};

constexpr std::array<int, 64> blackKingSquareValue = {
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -20, -20, -20, -20,
	-20, -20, -20, -20, 0,	 20,  40,  -20, 0,	 -20, 40,  20};

constexpr std::array<int, 64> whiteKingSquareValue = {
	0,	 20,  40,  -20, 0,	 -20, 40,  20,	-40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -20, -20, -20, -20, -20, -20, -20, -20};

constexpr std::array<int, 64> endgameKingSquareValue = {
	0,	10, 20, 30, 30, 20, 10, 0,	10, 20, 30, 40, 40, 30, 20, 10,
	20, 30, 40, 50, 50, 40, 30, 20, 30, 40, 50, 60, 60, 50, 40, 30,
	30, 40, 50, 60, 60, 50, 40, 30, 20, 30, 40, 50, 50, 40, 30, 20,
	10, 20, 30, 40, 40, 30, 20, 10, 0,	10, 20, 30, 30, 20, 10, 0};

constexpr std::array<int, 64> whiteKnightSquareValue = {
	-10, -30, -10, -10, -10, -10, -30, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -10, -10, -10, -10, -10, -10, -10,
};

constexpr std::array<int, 64> blackKnightSquareValue = {
	-10, -10, -10, -10, -10, -10, -10, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -30, -10, -10, -10, -10, -30, -10};

constexpr std::array<int, 64> whiteBishopSquareValue = {
	-10, -10, -20, -10, -10, -20, -10, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -10, -10, -10, -10, -10, -10, -10,
};

constexpr std::array<int, 64> blackBishopSquareValue = {
	-10, -10, -10, -10, -10, -10, -10, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -10, -20, -10, -10, -20, -10, -10,
};

void StaticEvaluator::setGrainSize(int value) {
	if (value < 2) {
		throw std::invalid_argument(
			"Evaluation score grain must be at least two");
	}
	scoreGrain = value;
}

void StaticEvaluator::configure(const EvaluatorParameters &params) {
	// Check the whole candidate before changing any member. Learning stores
	// doubles, but the playing evaluator deliberately truncates to integers.
	for (size_t index = 0; index < params.size(); ++index) {
		if (!std::isfinite(params[index]) ||
			std::abs(params[index]) > 10000.0 ||
			(index < 4 && params[index] < 1.0)) {
			throw std::invalid_argument(
				"Evaluator parameters must be finite, bounded, and have "
				"positive piece values");
		}
	}

	int i;

	materialValues[0] = 0;
	materialValues[pawn] = 100;
	for (i = knight; i <= queen; i++) {
		materialValues[i] = params[i - pawn - 1];
	}
	materialValues[king] = kInfty;
	maximumNonMaterialScore = params[4];
	pawnSquareValueWeight = params[5];
	doubledPawnPenalty = params[6];
	isolatedPawnPenalty = params[7];
	backwardsPawnPenalty = params[8];
	passedPawnBonus = params[9];
	kingSquareValueWeight = params[10];
	knightSquareValueWeight = params[11];
	bishopSquareValueWeight = params[12];
	rookSemiOpenFileBonus = params[13];
	rookOpenFileBonus = params[14];
	rookRank7Bonus = params[15];

	scoreGrain = kDefaultGrainSize;
	// Keep the lazy-evaluation bound on the same score lattice as the default
	// evaluator. The later positional clamp is what makes this a valid bound.
	maximumNonMaterialScore /= scoreGrain;
	maximumNonMaterialScore *= scoreGrain;

	if (maximumNonMaterialScore < 0.0) {
		maximumNonMaterialScore = 0.0;
	}

	resetPawnTable();
	startingMaterial = 16 * materialValues[pawn];
	startingMaterial += 4 * materialValues[knight];
	startingMaterial += 4 * materialValues[bishop];
	startingMaterial += 4 * materialValues[rook];
	startingMaterial += 2 * materialValues[queen];
	maxMaterial = startingMaterial / 2 - materialValues[queen];
	maxMaterialWoBonus = startingMaterial / 2 - 1600;
}

int StaticEvaluator::materialScore(const Board &board, int &whiteMat,
								   int &blackMat) {
	// Build a White-minus-Black score, then convert once to the side to move.
	// The returned raw material totals also select each king's endgame table.
	int i, totalMat, winnersPawns, difMat, score, nBonus, rBonus, qBonus;
	I64 bonus;
	int totalPawns;
	whiteMat = 0;
	blackMat = 0;
	for (i = pawn; i <= queen; i++) {
		whiteMat += board.pieceCount(white, i) * materialValues[i];
		blackMat += board.pieceCount(black, i) * materialValues[i];
	}
	difMat = whiteMat - blackMat;
	totalMat = whiteMat + blackMat;
	winnersPawns = board.pieceCount((difMat > 0) ? white : black, pawn);
	// Reward exchanging material while ahead when the stronger side still
	// has pawns. Normalize by initial non-pawn material and pawn count.
	bonus =
		static_cast<I64>(difMat) * (startingMaterial - totalMat) * winnersPawns;
	bonus /= (startingMaterial - 1600) * (winnersPawns + 1);

	totalPawns = board.pieceCount(white, pawn) + board.pieceCount(black, pawn);
	// Knights gain with more pawns; rooks gain as files open. These and the
	// bishop-pair/duplicate-knight adjustments are engine-specific heuristics.
	nBonus = 2 * totalPawns - 16;
	qBonus = totalPawns;
	qBonus += board.pieceCount(white, bishop);
	qBonus += board.pieceCount(white, knight);
	qBonus += board.pieceCount(black, bishop);
	qBonus += board.pieceCount(black, knight);
	rBonus = 32 - 2 * totalPawns;
	if (board.pieceCount(white, knight) >= 1) {
		bonus += nBonus;
		if (board.pieceCount(white, knight) > 1) {
			bonus -= 11;
		}
	}
	if (board.pieceCount(black, knight) >= 1) {
		bonus -= nBonus;
		if (board.pieceCount(black, knight) > 1) {
			bonus += 11;
		}
	}
	if (board.pieceCount(white, bishop) > 1) {
		bonus += 13;
	}
	if (board.pieceCount(black, bishop) > 1) {
		bonus -= 13;
	}
	if (board.pieceCount(white, queen) > 0) {
		bonus += qBonus;
	}
	if (board.pieceCount(black, queen) > 0) {
		bonus -= qBonus;
	}
	bonus += board.pieceCount(white, rook) * rBonus;
	bonus -= board.pieceCount(black, rook) * rBonus;

	score = difMat;
	// Saturate material before and after bonuses. With the default piece
	// values these caps keep evaluations below the search's mate-score range.
	if (score > maxMaterialWoBonus) {
		score = maxMaterialWoBonus;
	}
	if (score < -maxMaterialWoBonus) {
		score = -maxMaterialWoBonus;
	}
	score += bonus;

	if (score > maxMaterial) {
		score = maxMaterial;
	}
	if (score < -maxMaterial) {
		score = -maxMaterial;
	}
	if (board.sideToMove() == black) {
		score = -score;
	}
	return score;
}

int StaticEvaluator::materialBalance(const Board &board) {
	int whiteMat, blackMat;
	return materialScore(board, whiteMat, blackMat);
}

int StaticEvaluator::nonMaterialScore(const Board &board, int whiteMat,
									  int blackMat) {
	// Accumulate White-minus-Black position terms. Rook file bonuses depend
	// on both pawn sets, and the seventh-rank bonus uses each rook's own side.
	int score;
	int square;
	int file;
	Bitboard pieces;
	Bitboard mask;
	Bitboard test;

	score = evaluatePawnsAndKings(board, whiteMat, blackMat);

	pieces = board.pieceBoard(white, knight);
	while (pieces) {
		square = pieces.removeFirstOne();
		score +=
			(knightSquareValueWeight * whiteKnightSquareValue[square]) / 100;
	}
	pieces = board.pieceBoard(white, bishop);
	while (pieces) {
		square = pieces.removeFirstOne();
		score +=
			(bishopSquareValueWeight * whiteBishopSquareValue[square]) / 100;
	}
	pieces = board.pieceBoard(white, rook);
	while (pieces) {
		square = pieces.removeFirstOne();
		file = square % 8;
		mask = fileA << file;
		test = board.pieceBoard(white, pawn) & mask;
		if (!test) {
			test = board.pieceBoard(black, pawn) & mask;
			if (!test) {
				score += rookOpenFileBonus;
			} else {
				score += rookSemiOpenFileBonus;
			}
		}
		if (square / 8 == 6) {
			score += rookRank7Bonus;
		}
	}

	pieces = board.pieceBoard(black, knight);
	while (pieces) {
		square = pieces.removeFirstOne();
		score -=
			(knightSquareValueWeight * blackKnightSquareValue[square]) / 100;
	}
	pieces = board.pieceBoard(black, bishop);
	while (pieces) {
		square = pieces.removeFirstOne();
		score -=
			(bishopSquareValueWeight * blackBishopSquareValue[square]) / 100;
	}
	pieces = board.pieceBoard(black, rook);
	while (pieces) {
		square = pieces.removeFirstOne();
		file = square % 8;
		mask = fileA << file;
		test = board.pieceBoard(black, pawn) & mask;
		if (!test) {
			test = board.pieceBoard(white, pawn) & mask;
			if (!test) {
				score -= rookOpenFileBonus;
			} else {
				score -= rookSemiOpenFileBonus;
			}
		}
		if (square / 8 == 1) {
			score -= rookRank7Bonus;
		}
	}

	if (score > maximumNonMaterialScore) {
		score = maximumNonMaterialScore;
	} else if (score < -maximumNonMaterialScore) {
		score = -maximumNonMaterialScore;
	}
	// Dampen either side's positional advantage toward zero as the reversible
	// move clock grows. Halving small advantages prevents crossing zero and
	// preserves the bound used by test() for its material-only shortcut.
	const I64 doubledClock = 2 * static_cast<I64>(board.fiftyMoveRule());
	if (score > 0) {
		if (doubledClock > score) {
			score /= 2;
		} else {
			score -= board.fiftyMoveRule();
		}
	} else if (score < 0) {
		if (doubledClock > -score) {
			score /= 2;
		} else {
			score += board.fiftyMoveRule();
		}
	}
	if (board.sideToMove() == black) {
		score = -score;
	}
	return score;
}

int StaticEvaluator::checkmateValue(int ply) const {
	// Mate magnitude decreases with root-relative ply: prefer faster wins
	// and delay unavoidable losses. Round onto the search's score lattice.
	int score;
	score = kMinCheckmateValue + 5 * (kMaxDepth - ply);
	score /= scoreGrain;
	score *= scoreGrain;
	return score;
}

bool StaticEvaluator::test(const Board &board, int testValue) {
	// Boolean null-window evaluation: first try material +/- the largest
	// possible positional correction, computing positional features only if
	// that interval straddles the threshold. Search thresholds lie between
	// score-grain multiples, so a fully evaluated score must not equal them.
	int score, materialScore;
	int whiteMat, blackMat;
	bool testResult;
	materialScore = this->materialScore(board, whiteMat, blackMat);
	score = materialScore / scoreGrain;
	score *= scoreGrain;
	if ((score - maximumNonMaterialScore > testValue)) {
		testResult = true;
	} else if ((score + maximumNonMaterialScore < testValue)) {
		testResult = false;
	} else {
		score = materialScore + nonMaterialScore(board, whiteMat, blackMat);
		score /= scoreGrain;
		score *= scoreGrain;
		assert(score != testValue);
		if (score > testValue) {
			testResult = true;
		} else {
			testResult = false;
		}
	}
	return testResult;
}

void StaticEvaluator::writePawnTable(U32 key, int score) {
	// Direct-mapped cache with striped locks. Since the slot count is a
	// multiple of the lock count, all writers to a slot share the same lock.
	int tableIndex;
	int lockIndex;
#ifdef PAWN_TABLE_OFF
	return;
#endif

	tableIndex = key % kPawnTableSize;
	lockIndex = key % kPawnTableLocks;
	/****************************************************/
	const std::lock_guard<hpx::mutex> lock(locks[lockIndex]);
	/****************************************************/
	pawnTable[tableIndex].key = key;
	pawnTable[tableIndex].score = score;
	/****************************************************/

	/****************************************************/
}

bool StaticEvaluator::readPawnTable(U32 key, int &score) {
	// Compare the retained key, not just the slot index. Zero is reserved for
	// an empty entry; a real zero-key position is evaluated without caching.
	int tableIndex;
	int lockIndex;
	bool rc;

#ifdef PAWN_TABLE_OFF
	return false;
#endif
	tableIndex = key % kPawnTableSize;
	lockIndex = key % kPawnTableLocks;
	/****************************************************/
	const std::lock_guard<hpx::mutex> lock(locks[lockIndex]);
	/****************************************************/
	if ((pawnTable[tableIndex].key == key) && (key != 0)) {
		score = pawnTable[tableIndex].score;
		rc = true;
	} else {
		rc = false;
	}
	/****************************************************/

	/****************************************************/
	return rc;
}

void StaticEvaluator::resetPawnTable() {
	// Hold every stripe in a fixed order to exclude readers/writers while
	// invalidating scores after a parameter change.
	std::vector<std::unique_lock<hpx::mutex>> guards;
	guards.reserve(kPawnTableLocks);
	for (auto &mutex : locks) {
		guards.emplace_back(mutex);
	}
	for (auto &entry : pawnTable) {
		entry = PawnTableEntry{};
	}
}

int StaticEvaluator::evaluatePawnsAndKings(const Board &board, int whiteMat,
										   int blackMat) {
	// Files 1..8 hold each side's least advanced pawn; sentinel files 0 and 9
	// let the same neighbor-file tests handle edge pawns without branching.
	int i;
	int score;
	int kingScore;
	int square;
	std::array<std::array<int, 10>, 2> pawnRank;
	int rank;
	int file;
	U32 key;
	Bitboard pawns;
	for (i = 0; i < 10; i++) {
		pawnRank[white][i] = 7;
		pawnRank[black][i] = 0;
	}
	key = board.pawnKey() & 0xFFFFFFFF;
	// Board::pawnKey() includes kings and en passant as well as pawns. Add
	// the two material-phase flags because king placement scores use them;
	// the precise material totals do not otherwise enter this cached score.
	if (whiteMat < kEndgameDefinition) {
		key ^= 0xFFFF0000;
	}
	if (blackMat < kEndgameDefinition) {
		key ^= 0x0000FFFF;
	}
	if (!readPawnTable(key, score)) {
		pawns = board.pieceBoard(white, pawn);
		while (pawns) {
			square = pawns.removeFirstOne();
			rank = square / 8;
			file = square % 8;
			if (rank < pawnRank[white][file + 1]) {
				pawnRank[white][file + 1] = rank;
			}
		}
		pawns = board.pieceBoard(black, pawn);
		while (pawns) {
			square = pawns.removeFirstOne();
			rank = square / 8;
			file = square % 8;
			if (rank > pawnRank[black][file + 1]) {
				pawnRank[black][file + 1] = rank;
			}
		}
		score = 0;
		// Passed pawns have no enemy pawn ahead on their own/adjacent files;
		// doubled pawns have a friendly pawn behind on their own file. The
		// remaining rank tests are inexpensive pawn-weakness heuristics.
		pawns = board.pieceBoard(white, pawn);
		while (pawns) {
			square = pawns.removeFirstOne();
			score +=
				(pawnSquareValueWeight * whitePawnSquareValue[square]) / 100;
			rank = square / 8;
			file = square % 8;
			if ((pawnRank[black][file + 1 - 1] <= rank) &&
				(pawnRank[black][file + 1] <= rank) &&
				(pawnRank[black][file + 1 + 1] <= rank)) {
				score += passedPawnBonus * rank;
			}
			if (pawnRank[white][file + 1] < rank) {
				score -= doubledPawnPenalty;
			}
			if ((pawnRank[white][file + 1 - 1] == 7) &&
				(pawnRank[white][file + 1 + 1] == 7)) {
				score -= isolatedPawnPenalty;
			} else if ((pawnRank[white][file + 1 - 1] < rank) &&
					   (pawnRank[white][file + 1 + 1] < rank)) {
				score -= backwardsPawnPenalty;
			}
		}
		pawns = board.pieceBoard(black, pawn);
		while (pawns) {
			square = pawns.removeFirstOne();
			score -=
				(pawnSquareValueWeight * blackPawnSquareValue[square]) / 100;
			rank = square / 8;
			file = square % 8;
			if ((pawnRank[white][file + 1 - 1] >= rank) &&
				(pawnRank[white][file + 1] >= rank) &&
				(pawnRank[white][file + 1 + 1] >= rank)) {
				score -= passedPawnBonus * (7 - rank);
			}
			if (pawnRank[black][file + 1] > rank) {
				score += doubledPawnPenalty;
			}
			if ((pawnRank[black][file + 1 - 1] == 0) &&
				(pawnRank[black][file + 1 + 1] == 0)) {
				score += isolatedPawnPenalty;
			} else if ((pawnRank[black][file + 1 - 1] > rank) &&
					   (pawnRank[black][file + 1 + 1] > rank)) {
				score += backwardsPawnPenalty;
			}
		}

		kingScore = 0;
		// Use the opponent's material to choose active endgame king placement
		// versus middlegame shelter. Wing kings inspect the nearest three
		// files, giving the innermost shelter file half weight.
		square = board.pieceBoard(white, king).firstOne();
		if (blackMat < kEndgameDefinition) {
			kingScore += endgameKingSquareValue[square];
		} else {
			kingScore += whiteKingSquareValue[square];
			file = square % 8;
			if (file < 3) {
				kingScore += evaluateWhiteKingPawn(pawnRank, 0);
				kingScore += evaluateWhiteKingPawn(pawnRank, 1);
				kingScore += evaluateWhiteKingPawn(pawnRank, 2) / 2;
			} else if (file > 4) {
				kingScore += evaluateWhiteKingPawn(pawnRank, 7);
				kingScore += evaluateWhiteKingPawn(pawnRank, 6);
				kingScore += evaluateWhiteKingPawn(pawnRank, 5) / 2;
			}
		}

		square = board.pieceBoard(black, king).firstOne();
		if (whiteMat < kEndgameDefinition) {
			kingScore -= endgameKingSquareValue[square];
		} else {
			kingScore -= blackKingSquareValue[square];
			file = square % 8;
			if (file < 3) {
				kingScore -= evaluateBlackKingPawn(pawnRank, 0);
				kingScore -= evaluateBlackKingPawn(pawnRank, 1);
				kingScore -= evaluateBlackKingPawn(pawnRank, 2) / 2;
			} else if (file > 4) {
				kingScore -= evaluateBlackKingPawn(pawnRank, 7);
				kingScore -= evaluateBlackKingPawn(pawnRank, 6);
				kingScore -= evaluateBlackKingPawn(pawnRank, 5) / 2;
			}
		}
		kingScore = (kingSquareValueWeight * kingScore) / 100;
		score += kingScore;
		writePawnTable(key, score);
	}
	return score;
}

static int
evaluateWhiteKingPawn(const std::array<std::array<int, 10>, 2> &pawnRank,
					  int file) {
	// Penalize advanced/missing friendly shelter and approaching enemy
	// pawns; a file with no enemy pawn also permits heavier-piece attacks.
	int kingScore;
	kingScore = 0;
	if (pawnRank[white][file + 1] == 1) {
		// A pawn on its starting rank gives full king shelter.
	} else if (pawnRank[white][file + 1] == 2) {
		kingScore -= 10;
	} else if (pawnRank[white][file + 1] != 7) {
		kingScore -= 20;
	} else {
		kingScore -= 25;
	}
	if (pawnRank[black][file + 1] == 0) {
		kingScore -= 15;
	} else if (pawnRank[black][file + 1] == 2) {
		kingScore -= 10;
	} else if (pawnRank[black][file + 1] == 3) {
		kingScore -= 5;
	}
	return kingScore;
}

static int
evaluateBlackKingPawn(const std::array<std::array<int, 10>, 2> &pawnRank,
					  int file) {
	int kingScore;
	kingScore = 0;
	if (pawnRank[black][file + 1] == 6) {
		// A pawn on its starting rank gives full king shelter.
	} else if (pawnRank[black][file + 1] == 5) {
		kingScore -= 10;
	} else if (pawnRank[black][file + 1] != 0) {
		kingScore -= 20;
	} else {
		kingScore -= 25;
	}
	if (pawnRank[white][file + 1] == 7) {
		kingScore -= 15;
	} else if (pawnRank[white][file + 1] == 5) {
		kingScore -= 10;
	} else if (pawnRank[white][file + 1] == 4) {
		kingScore -= 5;
	}
	return kingScore;
}

} // namespace morphy_chess
