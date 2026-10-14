#include <hpx/modules/threading.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/node.hpp>
#include <morphychess/search.hpp>
#include <stdexcept>
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

Search::Search(Board *board, StaticEvaluator *evaluator, MemoryTable *table) :
	boardPtr(board),
	evaluatorPtr(evaluator),
	tablePtr(table),
	scoreGrain(evaluator->grainSize()) {}

void Search::setKillTime(std::time_t value) {
	killTime.store(value, std::memory_order_relaxed);
}

void Search::reconstructPrincipalVariation(Board *leafNode, Move *pv, int depth,
										   bool print) {
	int ply, upper, lower, guess;
	char moveStr[64];
	bool inTable;

	for (ply = 0; depth > 0 && ply < kMaxDepth; depth--) {
		inTable =
			tablePtr->read(*this, pv + ply, &lower, &upper, depth, -kInfty);
		if (!inTable) {
			break;
		}
		if (!inTable || (pv[ply]).isEmpty()) {
			if ((killTime != 0) && (killTime <= time(NULL))) {
				break;
			}
			guess = upper + lower;
			guess /= 2;
			guess /= scoreGrain;
			guess *= scoreGrain;
			serialTestDriver(pv + ply, depth, guess);
		}
		if ((pv[ply]).isEmpty()) {
			break;
		}
		(boardPtr)->moveToStr(moveStr, pv[ply]);
		if (print) {
			printf("%8s ", moveStr);
			fflush(stdout);
		}
		(boardPtr)->makeMove(pv[ply]);
		ply++;
		if ((boardPtr)->kingInCheck()) {
			depth++;
		}
		if ((boardPtr)->result() != inProgress) {
			break;
		}
	}
	(pv + ply)->setEmpty();
	if (leafNode != NULL) {
		*(leafNode) = *(boardPtr);
	}
	while (ply > 0) {
		(boardPtr)->undoMove();
		ply--;
	}
}

void Search::resetCounters() {
	nodesVisited = 0;
	tableHitCount = 0;
	nullHitCount = 0;
	lateMoveReductionCount = 0;
}

int Search::serialTestDriver(Move *move, int depth, int initialGuess) {
	bool testResult;
	int upperBound;
	int lowerBound;
	int testValue;
	int epsilon;
	bool timesup;

	upperBound = +kInfty;
	lowerBound = -kInfty;
	testValue = initialGuess;
	epsilon = scoreGrain / 2;

	(bestMoveValue).setEmpty();
	timesup = false;

	while ((upperBound > lowerBound) && !timesup) {
		testResult = test(testValue - epsilon, depth);
		if (stopped() || ((killTime != 0) && (killTime <= time(NULL)))) {
			(move)->setEmpty();
			timesup = true;
		} else if (testResult == true) {
			*move = bestMoveValue;
			lowerBound = testValue;
			testValue += scoreGrain;
		} else {
			testValue -= scoreGrain;
			upperBound = testValue;
		}
	}

	return lowerBound;
}

bool Search::nullTest(int testValue, int depth) {
	Move nullMove;
	bool testResult;
	int stm, totalPawns;
	if ((depth <= 1) || (boardPtr)->kingInCheck() ||
		(boardPtr)->positionIsRepeat() || nullVerification) {
		testResult = false;
	} else {
		stm = boardPtr->sideToMove();
		totalPawns = 1 * boardPtr->pieceCount(stm, pawn);
		totalPawns += 3 * boardPtr->pieceCount(stm, knight);
		totalPawns += 3 * boardPtr->pieceCount(stm, bishop);
		totalPawns += 5 * boardPtr->pieceCount(stm, rook);
		totalPawns += 9 * boardPtr->pieceCount(stm, queen);
		if (totalPawns >= 12) {
			(nullMove).setEmpty();
			(boardPtr)->makeMove(nullMove);
			currentPly++;
			testResult = !testChild(-testValue, depth - 1 - kNullReduction);
			currentPly--;
			(boardPtr)->undoMove();
		} else {
			testResult = false;
		}
	}
	if ((testResult == true) && (verifyNullMove == true)) {
		verifyNullMove = false;
		nullVerification = true;
		testResult = testChild(testValue, depth - 1);
		verifyNullMove = true;
	}
	nullVerification = false;
	return testResult;
}

bool Search::extensionNeeded() {
	Board *b;
	bool rc;
	Move move;
	int movingMan;
	b = boardPtr;
	move = b->lastMove();
	movingMan = b->pieceValue(move.to());
	if ((b)->kingInCheck()) {
		rc = true;
	} else if (move.promo() > pawn) {
		rc = true;
	} else if ((movingMan == +pawn) && (move.to() / 8 == 6)) {
		rc = true;
	} else if ((movingMan == -pawn) && (move.to() / 8 == 1)) {
		rc = true;
	} else {
		rc = false;
	}
	return rc;
}

bool Search::testChild(int testValue, int depth) {
	bool testResult;
	bool extension;
	Move currentMove;
	Move bestMove;
	int branchCount;
	int nodeStatus;
	int lowerBound, upperBound, drawScore;

	drawScore = contemptFactor;
	if (boardPtr->sideToMove() != computerColor) {
		drawScore = -drawScore;
	}
	if ((boardPtr)->isDraw()) {
		testResult = (bool)(testValue < drawScore);
		nodesVisited++;
	} else {
		extension = extensionNeeded();
		if (extension) {
			depth++;
		}
		if ((depth <= 0) || (currentPly >= kMaxDepth - 1)) {
			testResult = quiescentTest(testValue);
		} else {
			tablePtr->read(*this, &bestMove, &lowerBound, &upperBound, depth,
						   testValue);
			if (upperBound < testValue) {
				testResult = false;
				tableHitCount++;
				nodesVisited++;
			} else if (lowerBound > testValue) {
				testResult = true;
				tableHitCount++;
				nodesVisited++;
			} else {
				testResult = nullTest(testValue, depth);
				if (testResult == true) {
					nodesVisited++;
					nullHitCount++;
				} else {
					Node node(this, bestMove);
					nodeStatus = (node).doNextMove(&currentMove);
					if (nodeStatus == kNodeMovesAvailable) {
						branchCount = 0;
						do {
							if (extension || !(node).okToReduce(depth)) {
								testResult = !testChild(-testValue, depth - 1);
							} else {
								lateMoveReductionCount++;
								testResult = !testChild(-testValue, depth - 2);
								if (testResult == true) {
									testResult =
										!testChild(-testValue, depth - 1);
								}
							}
							(node).restore();
							if (testResult == true) {
								bestMove = currentMove;
								break;
							}
							branchCount++;
							nodeStatus = (node).doNextMove(&currentMove);
						} while (nodeStatus == kNodeMovesAvailable);
					} else {
						assert(nodeStatus == kNodeTerminal);
						if ((boardPtr)->kingInCheck()) {
							if (testValue <
								-(evaluatorPtr)->checkmateValue(currentPly)) {
								testResult = true;
							} else {
								testResult = false;
							}
						} else if (testValue > drawScore) {
							testResult = false;
						} else {
							testResult = true;
						}
					}
					history.update(bestMove, testResult, depth, currentPly,
								   (boardPtr->pieceValue(bestMove.to()) == 0)
									   ? true
									   : false);
					tablePtr->write(*this, bestMove, testResult, testValue,
									depth);
				}
			}
		}
	}
	checkKill();
	return testResult;
}

bool Search::test(int testValue, int depth) {
	bool testResult;
	Move currentMove;
	int branchCount;
	int nodeStatus;
	int lowerBound, upperBound;

	assert(depth >= 1);
	verifyNullMove = true;
	currentPly = 0;
	(bestMoveValue).setEmpty();
	testResult = false;
	try {
		checkKill();
		tablePtr->read(*this, &bestMoveValue, &lowerBound, &upperBound, depth,
					   testValue);
		if ((upperBound < testValue) && !(bestMoveValue).isEmpty()) {
			nodesVisited++;
			tableHitCount++;
		} else if ((lowerBound > testValue) && !(bestMoveValue).isEmpty()) {
			testResult = true;
			nodesVisited++;
			tableHitCount++;
		} else {
			Node node(this, bestMoveValue);
			nodeStatus = (node).doNextMove(&currentMove);
			assertBoardConsistency(boardPtr);
			if (nodeStatus != kNodeMovesAvailable) {
				const int terminalScore =
					boardPtr->kingInCheck()
						? -evaluatorPtr->checkmateValue(currentPly)
						: (boardPtr->sideToMove() == computerColor
							   ? contemptFactor
							   : -contemptFactor);
				return terminalScore > testValue;
			}
			branchCount = 0;
			do {
				if (!(node).okToReduce(depth)) {
					testResult = !testChild(-testValue, depth - 1);
				} else {
					lateMoveReductionCount++;
					testResult = !testChild(-testValue, depth - 2);
					if (testResult == true) {
						testResult = !testChild(-testValue, depth - 1);
					}
				}
				(node).restore();
				if (testResult == true) {
					cutoffs[branchCount]++;
					bestMoveValue = currentMove;
					break;
				}
				branchCount++;
				nodeStatus = (node).doNextMove(&currentMove);
			} while (nodeStatus == kNodeMovesAvailable);
			history.update(
				bestMoveValue, testResult, depth, currentPly,
				(boardPtr->pieceValue(bestMoveValue.to()) == 0) ? true : false);
			tablePtr->write(*this, bestMoveValue, testResult, testValue, depth);
		}
	} catch (const SearchCancelled &) {
		bestMoveValue.setEmpty();
		testResult = false;
		verifyNullMove = true;
		nullVerification = false;
		while (currentPly > 0) {
			(boardPtr)->undoMove();
			currentPly--;
		}
	}
	return testResult;
}

void Search::checkKill() {
	// Search workers must cooperate with timer and command tasks even with one
	// OS thread.
	if (++pollsSinceYield >= pollFrequency) {
		pollsSinceYield = 0;
		hpx::this_thread::yield();
	}
	const auto deadline = killTime.load(std::memory_order_relaxed);
	if (stopRequested.load(std::memory_order_relaxed) ||
		(deadline != 0 && deadline <= std::time(nullptr))) {
		stopRequested.store(true, std::memory_order_relaxed);
		throw SearchCancelled{};
	}
}

void Search::requestStop() {
	stopRequested.store(true, std::memory_order_relaxed);
}
void Search::clearStop() {
	stopRequested.store(false, std::memory_order_relaxed);
}
void Search::setGrainSize(int value) {
	if (value < 2) {
		throw std::invalid_argument("Search score grain must be at least two");
	}
	scoreGrain = value;
}
void Search::setThreadNumber(int value) { threadNumber = value; }

void Search::setBoard(Board *value) { boardPtr = value; }

void Search::setContemptFactor(int value) {
	contemptFactor = (value / scoreGrain) * scoreGrain;
}

void Search::setComputerColor(int value) { computerColor = value; }

} // namespace morphy_chess
