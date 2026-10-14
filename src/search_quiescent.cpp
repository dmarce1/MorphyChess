#include <morphychess/node.hpp>
#include <morphychess/search.hpp>

namespace morphy_chess {
// Convert a series of threshold tests back to a quantized score. The public
// guess/result use White's perspective; quiescentTest uses the side to move.
int Search::quiescentValue(int guess) {
	if (boardPtr->kingInCheckmate(white)) {
		return -evaluatorRef.checkmateValue(currentPly);
	}
	if (boardPtr->kingInCheckmate(black)) {
		return evaluatorRef.checkmateValue(currentPly);
	}
	if (boardPtr->kingInStalemate(boardPtr->sideToMove()) ||
		boardPtr->isDraw()) {
		return 0;
	}
	int const startingPly = currentPly;
	int const originalGuess = guess;
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
	} catch (SearchCancelled const &) {
		while (currentPly > startingPly) {
			boardPtr->undoMove();
			--currentPly;
		}
		return originalGuess;
	}
	score += scoreGrain / 2;
	return boardPtr->sideToMove() == black ? -score : score;
}

// Extend the horizon through captures/promotions so a static evaluation is
// not taken halfway through an exchange. In check, search all legal evasions
// and disallow stand-pat. See bibliography.md [quiescence].
bool Search::quiescentTest(int testValue) {
	checkKill();
	if (boardPtr->isDraw()) {
		++nodesVisited;
		return 0 > testValue;
	}
	bool const inCheck = boardPtr->kingInCheck();
	if (currentPly >= kMaxDepth) {
		++nodesVisited;
		if (!inCheck && boardPtr->kingInStalemate(boardPtr->sideToMove())) {
			return 0 > testValue;
		}
		return evaluatorRef.test(*boardPtr, testValue);
	}
	if (!inCheck && evaluatorRef.test(*boardPtr, testValue)) {
		++nodesVisited;
		// Stand-pat presumes at least one legal move exists. A stalemate is
		// a draw even when the static material/position score exceeds the test.
		if (boardPtr->kingInStalemate(boardPtr->sideToMove())) {
			return 0 > testValue;
		}
		return true;
	}
	Node node(*this, Move{});
	Move currentMove;
	bool anyMove = false;
	while ((inCheck ? node.doNextMove(currentMove)
					: node.doNextNonquiescentMove(currentMove)) ==
		   kNodeMovesAvailable) {
		anyMove = true;
		bool const testResult = !quiescentTest(-testValue);
		node.restore();
		if (testResult) {
			return true;
		}
	}
	if (inCheck && !anyMove) {
		return -evaluatorRef.checkmateValue(currentPly) > testValue;
	}
	// A legal tactical move already rules out stalemate. Only an empty
	// tactical list needs the additional full legal-move probe on this path.
	if (!inCheck && !anyMove &&
		boardPtr->kingInStalemate(boardPtr->sideToMove())) {
		return 0 > testValue;
	}
	return false;
}
} // namespace morphy_chess
