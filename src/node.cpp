#include <array>
#include <morphychess/debug.hpp>
#include <morphychess/history.hpp>
#include <morphychess/integers.hpp>
#include <morphychess/node.hpp>
#include <morphychess/params.hpp>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>

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

int Node::totalMoves() const { return totalMovesGenerated; }

Node::Node(Search &searchValue, Move move) :
	search(searchValue) {
	firstNoncap = false;
	mgStage = GenStage::start;
	totalMovesGenerated = 0;
	search.nodesVisited++;
	tableMove = move;
	movesInQueue = 0;
	(killer).setEmpty();
}

int Node::doNextMove(Move &branch) {
	Move move;
	int i, rc, bestScore, score, bestI;

	assert(totalMovesGenerated < maxMoves);
	bestI = 0;
	rc = kNodeMovesAvailable;
	if (mgStage == GenStage::noncaptures) {
		firstNoncap = false;
	}
tryAgain:
	switch (mgStage) {
	case GenStage::start:
		if (!(tableMove).isEmpty()) {
			move = tableMove;
			mgStage = GenStage::table;
			break;
		}
		[[fallthrough]];
	case GenStage::table:
		mgStage = GenStage::promotions;
		movesInQueue = (search.boardPtr)->genPseudolegalPromotions(list.data());
		[[fallthrough]];
	case GenStage::promotions:
		if (movesInQueue > 0) {
			movesInQueue--;
			move = list[movesInQueue];
			break;
		}
		movesInQueue = (search.boardPtr)->genPseudolegalCaptures(list.data());
		mgStage = GenStage::captures;
		for (i = 0; i < movesInQueue; i++) {
			scores[i] = captureMoveValue(list[i]);
		}
		[[fallthrough]];
	case GenStage::captures:
		if (movesInQueue > 0) {
			bestScore = -kInfty;
			for (i = 0; i < movesInQueue; i++) {
				score = scores[i];
				assert(score > -kInfty);
				if (score > bestScore) {
					bestScore = score;
					bestI = i;
				}
			}
			if (bestScore >= 0) {
				movesInQueue--;
				move = list[bestI];
				list[bestI] = list[movesInQueue];
				scores[bestI] = scores[movesInQueue];
				break;
			}
		}
		mgStage = GenStage::losingCaptures;
		move = search.history.killer1(search.currentPly);
		if ((move.promo() == 0)) {
			if (!(move == tableMove)) {
				if (!(search.boardPtr)->moveIsCapture(move)) {
					if ((search.boardPtr)->isPseudoLegalMove(move)) {
						killer = move;
						break;
					}
				}
			}
		}
		move = search.history.killer2(search.currentPly);
		if ((move.promo() == 0)) {
			if (!(move == tableMove)) {
				if (!(search.boardPtr)->moveIsCapture(move)) {
					if ((search.boardPtr)->isPseudoLegalMove(move)) {
						killer = move;
						break;
					}
				}
			}
		}
		[[fallthrough]];
	case GenStage::losingCaptures:
		if (movesInQueue > 0) {
			bestScore = -kInfty;
			for (i = 0; i < movesInQueue; i++) {
				score = scores[i];
				assert(score > -kInfty);
				if (score > bestScore) {
					bestScore = score;
					bestI = i;
				}
			}
			movesInQueue--;
			move = list[bestI];
			list[bestI] = list[movesInQueue];
			scores[bestI] = scores[movesInQueue];
			break;
		}
		movesInQueue =
			(search.boardPtr)->genPseudolegalNoncaptures(list.data());
		mgStage = GenStage::noncaptures;
		firstNoncap = true;
		[[fallthrough]];
	case GenStage::noncaptures:
		if (movesInQueue > 0) {
			bestScore = -kInfty;
			for (i = 0; i < movesInQueue; i++) {
				score = search.history.moveHistory(list[i], search.currentPly);
				assert(score > -kInfty);
				if (score > bestScore) {
					if (!(killer == list[i])) {
						bestScore = score;
						bestI = i;
					}
				}
			}
			if (bestScore > -kInfty) {
				movesInQueue--;
				move = list[bestI];
				list[bestI] = list[movesInQueue];
				break;
			}
		} else {
			mgStage = GenStage::end;
		}
		[[fallthrough]];
	case GenStage::end:
		(move).setEmpty();
		rc = kNodeNoMoreMoves;
		break;
	}
	branch = move;
	if (rc == kNodeMovesAvailable) {
		if (((move == tableMove)) && (mgStage != GenStage::table)) {
			goto tryAgain;
		} else if (!(search.boardPtr)->makeMove(move)) {
			goto tryAgain;
		} else {
			search.currentPly++;
			totalMovesGenerated++;
		}
	} else if (totalMovesGenerated == 0) {
		rc = kNodeTerminal;
	}
	return rc;
}

int Node::doNextNonquiescentMove(Move &branch) {
	Move move;
	int i, rc, bestScore, score, bestI;

	assert(totalMovesGenerated < maxMoves);
	bestI = 0;
	rc = kNodeMovesAvailable;
tryAgain:
	switch (mgStage) {
	case GenStage::start:
		movesInQueue = (search.boardPtr)->genPseudolegalPromotions(list.data());
		mgStage = GenStage::promotions;
		[[fallthrough]];
	case GenStage::promotions:
		if (movesInQueue > 0) {
			movesInQueue--;
			move = list[movesInQueue];
			break;
		}
		movesInQueue = (search.boardPtr)->genPseudolegalCaptures(list.data());
		mgStage = GenStage::captures;
		for (i = 0; i < movesInQueue; i++) {
			scores[i] = captureMoveValue(list[i]);
		}
		[[fallthrough]];
	case GenStage::captures:
		if (movesInQueue > 0) {
			bestScore = -kInfty;
			for (i = 0; i < movesInQueue; i++) {
				score = scores[i];
				assert(score > -kInfty);
				if (score > bestScore) {
					bestScore = score;
					bestI = i;
				}
			}
			movesInQueue--;
			move = list[bestI];
			list[bestI] = list[movesInQueue];
			scores[bestI] = scores[movesInQueue];
			break;
		}
		mgStage = GenStage::end;
		[[fallthrough]];
	case GenStage::end:
		(move).setEmpty();
		rc = kNodeNoMoreMoves;
		break;
	default:
		throw std::logic_error(
			"Cannot mix normal and quiescent move generation on one node");
	}
	branch = move;
	if (rc == kNodeMovesAvailable) {
		if ((search.boardPtr)->makeMove(move)) {
			search.currentPly++;
			totalMovesGenerated++;
		} else {
			goto tryAgain;
		}
	}
	return rc;
}

void Node::restore() {
	search.currentPly--;
	(search.boardPtr)->undoMove();
}

int Node::captureMoveValue(Move move) const {
	const Board &board = *search.boardPtr;
	static const std::array<int, 7> pvals = {0,	  100, 300,	  300,
											 500, 900, kInfty};
	int targetPiece;
	int capturingPiece;
	int value;
	targetPiece = abs(board.pieceValue(move.to()));
	capturingPiece = abs(board.pieceValue(move.from()));
	value = pvals[targetPiece];
	if (board.squareUnderAttackBy(move.to(), colorFlip(board.sideToMove()))) {
		value -= pvals[capturingPiece];
	} else {
		value -= pvals[capturingPiece] / 10;
	}
	return value;
}

bool Node::noMovesGenerated() const { return (bool)(totalMovesGenerated == 0); }

bool Node::okToReduce(int depth) const {
	bool rc;
#ifdef LATE_MOVE_REDUCTION_OFF
	return false;
#endif
	if (mgStage != GenStage::noncaptures) {
		rc = false;
	} else if (firstNoncap) {
		rc = false;
	} else if (depth < kReductionLimit) {
		rc = false;
	} else if (totalMovesGenerated <= kFullDepthMoves) {
		rc = false;
	} else if ((search.boardPtr)->kingInCheck()) {
		rc = false;
	} else {
		rc = true;
	}
	return rc;
}

bool Node::moveIsFutile(int materialBalance, int testValue) const {
	bool rc;
	Move move;
	int moveMaterialGain;
	int lastCap;
	const Board &b = *search.boardPtr;
	static const std::array<int, 7> pvals = {0,	  100, 325,	  325,
											 500, 900, kInfty};

	if ((search.boardPtr)->kingInCheck()) {
		rc = false;
	} else {
		move = b.lastMove();
		lastCap = b.mostRecentCapture();
		moveMaterialGain = pvals[lastCap];
		if (move.promo() > pawn) {
			moveMaterialGain += pvals[move.promo()] - pvals[pawn];
		}
		if (materialBalance + kFutilityMargin + moveMaterialGain >= testValue) {
			rc = false;
		} else {
			rc = true;
		}
	}

	return rc;
}

} // namespace morphy_chess
