#include <morphychess/node.hpp>
#include <morphychess/search.hpp>

namespace morphy_chess {
int Search::quiescentValue(int guess) {
	if (boardPtr->kingInCheckmate(white)) {
		return -evaluatorPtr->checkmateValue(currentPly);
	}
	if (boardPtr->kingInCheckmate(black)) {
		return evaluatorPtr->checkmateValue(currentPly);
	}
	if (boardPtr->kingInStalemate(boardPtr->sideToMove()) ||
		boardPtr->isDraw()) {
		return 0;
	}
	const int startingPly = currentPly;
	const int originalGuess = guess;
	if (boardPtr->sideToMove() == black) {
		guess = -guess;
	}
	guess = (guess / scoreGrain) * scoreGrain;
	int score = guess - scoreGrain / 2;
	try {
		if (quiescentTest(score)) {
			score += scoreGrain;
			while (score < kInfty && quiescentTest(score)) {
				score += scoreGrain;
			}
			score -= scoreGrain;
		} else {
			score -= scoreGrain;
			while (score > -kInfty && !quiescentTest(score)) {
				score -= scoreGrain;
			}
		}
	} catch (const SearchCancelled &) {
		while (currentPly > startingPly) {
			boardPtr->undoMove();
			--currentPly;
		}
		return originalGuess;
	}
	score += scoreGrain / 2;
	return boardPtr->sideToMove() == black ? -score : score;
}

bool Search::quiescentTest(int testValue) {
	checkKill();
	if (boardPtr->isDraw()) {
		++nodesVisited;
		return 0 > testValue;
	}
	const bool inCheck = boardPtr->kingInCheck();
	if (currentPly >= kMaxDepth) {
		++nodesVisited;
		return evaluatorPtr->test(boardPtr, testValue);
	}
	if (!inCheck && evaluatorPtr->test(boardPtr, testValue)) {
		++nodesVisited;
		return true;
	}
	Node node(this, Move{});
	Move currentMove;
	bool anyMove = false;
	while ((inCheck ? node.doNextMove(&currentMove)
					: node.doNextNonquiescentMove(&currentMove)) ==
		   kNodeMovesAvailable) {
		anyMove = true;
		const bool testResult = !quiescentTest(-testValue);
		node.restore();
		if (testResult) {
			return true;
		}
	}
	if (inCheck && !anyMove) {
		return -evaluatorPtr->checkmateValue(currentPly) > testValue;
	}
	return false;
}
} // namespace morphy_chess
