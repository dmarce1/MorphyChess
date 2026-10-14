#include <cmath>
#include <morphychess/bits.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/params.hpp>
#include <morphychess/static_evaluator.hpp>
#include <mutex>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

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

static int evaluateWhiteKingPawn(int pawnRank[2][10], int file);
static int evaluateBlackKingPawn(int pawnRank[2][10], int file);

constexpr int whitePawnSquareValue[64] = {
	0, 0,  0,  0,	0,	 0,	 0,	 0, 0, 0, 0,  -40, -40, 0,	0, 0,
	1, 2,  3,  -10, -10, 3,	 2,	 1, 2, 4, 6,  8,   8,	6,	4, 2,
	3, 6,  9,  12,	12,	 9,	 6,	 3, 4, 8, 12, 16,  16,	12, 8, 4,
	5, 10, 15, 20,	20,	 15, 10, 5, 0, 0, 0,  0,   0,	0,	0, 0};

constexpr int blackPawnSquareValue[64] = {
	0, 0, 0,  0,   0,	0,	0, 0, 5, 10, 15, 20,  20,  15, 10, 5,
	4, 8, 12, 16,  16,	12, 8, 4, 3, 6,	 9,	 12,  12,  9,  6,  3,
	2, 4, 6,  8,   8,	6,	4, 2, 1, 2,	 3,	 -10, -10, 3,  2,  1,
	0, 0, 0,  -40, -40, 0,	0, 0, 0, 0,	 0,	 0,	  0,   0,  0,  0};

constexpr int blackKingSquareValue[64] = {
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -20, -20, -20, -20,
	-20, -20, -20, -20, 0,	 20,  40,  -20, 0,	 -20, 40,  20};

constexpr int whiteKingSquareValue[64] = {
	0,	 20,  40,  -20, 0,	 -20, 40,  20,	-40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40,
	-40, -40, -40, -40, -20, -20, -20, -20, -20, -20, -20, -20};

constexpr int endgameKingSquareValue[64] = {
	0,	10, 20, 30, 30, 20, 10, 0,	10, 20, 30, 40, 40, 30, 20, 10,
	20, 30, 40, 50, 50, 40, 30, 20, 30, 40, 50, 60, 60, 50, 40, 30,
	30, 40, 50, 60, 60, 50, 40, 30, 20, 30, 40, 50, 50, 40, 30, 20,
	10, 20, 30, 40, 40, 30, 20, 10, 0,	10, 20, 30, 30, 20, 10, 0};

constexpr int whiteKnightSquareValue[64] = {
	-10, -30, -10, -10, -10, -10, -30, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -10, -10, -10, -10, -10, -10, -10,
};

constexpr int blackKnightSquareValue[64] = {
	-10, -10, -10, -10, -10, -10, -10, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -30, -10, -10, -10, -10, -30, -10};

constexpr int whiteBishopSquareValue[64] = {
	-10, -10, -20, -10, -10, -20, -10, -10, -10, 0,	  0,   0,	0,
	0,	 0,	  -10, -10, 0,	 5,	  5,   5,	5,	 0,	  -10, -10, 0,
	5,	 10,  10,  5,	0,	 -10, -10, 0,	5,	 10,  10,  5,	0,
	-10, -10, 0,   5,	5,	 5,	  5,   0,	-10, -10, 0,   0,	0,
	0,	 0,	  0,   -10, -10, -10, -10, -10, -10, -10, -10, -10,
};

constexpr int blackBishopSquareValue[64] = {
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

int StaticEvaluator::materialScore(Board *board, int *whiteMat, int *blackMat) {
	int i, totalMat, winnersPawns, difMat, score, nBonus, rBonus, qBonus;
	I64 bonus;
	int totalPawns;
	*whiteMat = 0;
	*blackMat = 0;
	for (i = pawn; i <= queen; i++) {
		*whiteMat += board->pieceCount(white, i) * materialValues[i];
		*blackMat += board->pieceCount(black, i) * materialValues[i];
	}
	difMat = *whiteMat - *blackMat;
	totalMat = *whiteMat + *blackMat;
	winnersPawns = board->pieceCount((difMat > 0) ? white : black, pawn);
	bonus =
		static_cast<I64>(difMat) * (startingMaterial - totalMat) * winnersPawns;
	bonus /= (startingMaterial - 1600) * (winnersPawns + 1);

	totalPawns =
		board->pieceCount(white, pawn) + board->pieceCount(black, pawn);
	nBonus = 2 * totalPawns - 16;
	qBonus = totalPawns;
	qBonus += board->pieceCount(white, bishop);
	qBonus += board->pieceCount(white, knight);
	qBonus += board->pieceCount(black, bishop);
	qBonus += board->pieceCount(black, knight);
	rBonus = 32 - 2 * totalPawns;
	if (board->pieceCount(white, knight) >= 1) {
		bonus += nBonus;
		if (board->pieceCount(white, knight) > 1) {
			bonus -= 11;
		}
	}
	if (board->pieceCount(black, knight) >= 1) {
		bonus -= nBonus;
		if (board->pieceCount(black, knight) > 1) {
			bonus += 11;
		}
	}
	if (board->pieceCount(white, bishop) > 1) {
		bonus += 13;
	}
	if (board->pieceCount(black, bishop) > 1) {
		bonus -= 13;
	}
	if (board->pieceCount(white, queen) > 0) {
		bonus += qBonus;
	}
	if (board->pieceCount(black, queen) > 0) {
		bonus -= qBonus;
	}
	bonus += board->pieceCount(white, rook) * rBonus;
	bonus -= board->pieceCount(black, rook) * rBonus;

	score = difMat;
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
	if (board->sideToMove() == black) {
		score = -score;
	}
	return score;
}

int StaticEvaluator::materialBalance(Board *board) {
	int whiteMat, blackMat;
	return materialScore(board, &whiteMat, &blackMat);
}

int StaticEvaluator::nonMaterialScore(Board *board, int whiteMat,
									  int blackMat) {
	int score;
	int square;
	int file;
	Bitboard pieces;
	Bitboard mask;
	Bitboard test;

	score = evaluatePawnsAndKings(board, whiteMat, blackMat);

	pieces = board->pieceBoard(white, knight);
	while (pieces) {
		square = pieces.removeFirstOne();
		score +=
			(knightSquareValueWeight * whiteKnightSquareValue[square]) / 100;
	}
	pieces = board->pieceBoard(white, bishop);
	while (pieces) {
		square = pieces.removeFirstOne();
		score +=
			(bishopSquareValueWeight * whiteBishopSquareValue[square]) / 100;
	}
	pieces = board->pieceBoard(white, rook);
	while (pieces) {
		square = pieces.removeFirstOne();
		file = square % 8;
		mask = fileA << file;
		test = board->pieceBoard(white, pawn) & mask;
		if (!test) {
			test = board->pieceBoard(black, pawn) & mask;
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

	pieces = board->pieceBoard(black, knight);
	while (pieces) {
		square = pieces.removeFirstOne();
		score -=
			(knightSquareValueWeight * blackKnightSquareValue[square]) / 100;
	}
	pieces = board->pieceBoard(black, bishop);
	while (pieces) {
		square = pieces.removeFirstOne();
		score -=
			(bishopSquareValueWeight * blackBishopSquareValue[square]) / 100;
	}
	pieces = board->pieceBoard(black, rook);
	while (pieces) {
		square = pieces.removeFirstOne();
		file = square % 8;
		mask = fileA << file;
		test = board->pieceBoard(black, pawn) & mask;
		if (!test) {
			test = board->pieceBoard(white, pawn) & mask;
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
	if (score > 0) {
		if (2 * board->fiftyMoveRule() > score) {
			score /= 2;
		} else {
			score -= board->fiftyMoveRule();
		}
	} else if (score < 0) {
		if (2 * board->fiftyMoveRule() < score) {
			score /= 2;
		} else {
			score += board->fiftyMoveRule();
		}
	}
	if (board->sideToMove() == black) {
		score = -score;
	}
	return score;
}

int StaticEvaluator::checkmateValue(int ply) const {
	int score;
	score = kMinCheckmateValue + 5 * (kMaxDepth - ply);
	score /= scoreGrain;
	score *= scoreGrain;
	return score;
}

bool StaticEvaluator::test(Board *board, int testValue) {
	int score, materialScore;
	int whiteMat, blackMat;
	bool testResult;
	materialScore = this->materialScore(board, &whiteMat, &blackMat);
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

bool StaticEvaluator::readPawnTable(U32 key, int *score) {
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
		*score = pawnTable[tableIndex].score;
		rc = true;
	} else {
		rc = false;
	}
	/****************************************************/

	/****************************************************/
	return rc;
}

void StaticEvaluator::resetPawnTable() {
	std::vector<std::unique_lock<hpx::mutex>> guards;
	guards.reserve(kPawnTableLocks);
	for (auto &mutex : locks) {
		guards.emplace_back(mutex);
	}
	for (auto &entry : pawnTable) {
		entry = PawnTableEntry{};
	}
}

int StaticEvaluator::evaluatePawnsAndKings(Board *board, int whiteMat,
										   int blackMat) {
	int i;
	int score;
	int kingScore;
	int square;
	int pawnRank[2][10];
	int rank;
	int file;
	U32 key;
	Bitboard pawns;
	for (i = 0; i < 10; i++) {
		pawnRank[white][i] = 7;
		pawnRank[black][i] = 0;
	}
	key = board->pawnKey() & 0xFFFFFFFF;
	if (whiteMat < kEndgameDefinition) {
		key ^= 0xFFFF0000;
	}
	if (blackMat < kEndgameDefinition) {
		key ^= 0x0000FFFF;
	}
	if (!readPawnTable(key, &score)) {
		pawns = board->pieceBoard(white, pawn);
		while (pawns) {
			square = pawns.removeFirstOne();
			rank = square / 8;
			file = square % 8;
			if (rank < pawnRank[white][file + 1]) {
				pawnRank[white][file + 1] = rank;
			}
		}
		pawns = board->pieceBoard(black, pawn);
		while (pawns) {
			square = pawns.removeFirstOne();
			rank = square / 8;
			file = square % 8;
			if (rank > pawnRank[black][file + 1]) {
				pawnRank[black][file + 1] = rank;
			}
		}
		score = 0;
		pawns = board->pieceBoard(white, pawn);
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
		pawns = board->pieceBoard(black, pawn);
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
		square = board->pieceBoard(white, king).firstOne();
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

		square = board->pieceBoard(black, king).firstOne();
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

static int evaluateWhiteKingPawn(int pawnRank[2][10], int file) {
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

static int evaluateBlackKingPawn(int pawnRank[2][10], int file) {
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
