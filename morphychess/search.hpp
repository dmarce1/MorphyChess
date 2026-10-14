#pragma once

#include <array>
#include <atomic>
#include <ctime>

#include <morphychess/board.hpp>
#include <morphychess/history.hpp>
#include <morphychess/memory.hpp>
#include <morphychess/static_evaluator.hpp>

namespace morphy_chess {
class Node;
// One worker's mutable search state. Scores in threshold tests are relative
// to the side to move; recursive calls negate the threshold and their result.
// The boolean tests implement a null-window negamax search, with cached bounds
// and selective extensions/reductions. See bibliography.md [alpha_beta], [mtdf].
class Search {
	friend class Node;
	friend class MemoryTable;

public:
	Search(Board &board, StaticEvaluator &evaluator, MemoryTable &table);
	Search(Search const &) = delete;
	Search &operator=(Search const &) = delete;
	// Test whether the searched value exceeds an inter-grain threshold. A false
	// result can also mean cancellation; callers must inspect stopped().
	bool test(int testValue, int depth);
	bool quiescentTest(int testValue);
	// Unlike test(), this public evaluation returns a score from White's view.
	int quiescentValue(int guess);
	int serialTestDriver(Move &move, int depth, int initialGuess);
	void reconstructPrincipalVariation(Board *leafNode,
									   std::array<Move, kMaxDepth + 1> &pv,
									   int depth, bool print = false);
	void resetCounters();
	void requestStop();
	void clearStop();
	void setBoard(Board &board);
	void setKillTime(std::time_t killTime);
	void setGrainSize(int value);
	void setThreadNumber(int threadNumber);
	void setContemptFactor(int value);
	void setComputerColor(int color);
	void setBestMove(Move move) { bestMoveValue = move; }
	Board &board() const { return *boardPtr; }
	StaticEvaluator &evaluator() const { return evaluatorRef; }
	MemoryTable &table() const { return tableRef; }
	Move bestMove() const { return bestMoveValue; }
	int nodeCount() const { return nodesVisited; }
	int tableHits() const { return tableHitCount; }
	int nullHits() const { return nullHitCount; }
	int lateMoveReductions() const { return lateMoveReductionCount; }
	int grainSize() const { return scoreGrain; }
	int cutoffAtMoveNumber(int index) const { return cutoffs.at(index); }
	bool stopped() const {
		return stopRequested.load(std::memory_order_relaxed);
	}

private:
	class SearchCancelled {};
	bool testChild(int testValue, int depth);
	bool nullTest(int testValue, int depth);
	bool extensionNeeded();
	void checkKill();
	// Borrowed, non-null board; setBoard() can rebind it.
	Board *boardPtr;
	StaticEvaluator &evaluatorRef;
	History history;
	MemoryTable &tableRef;
	bool verifyNullMove{true};
	std::atomic<bool> stopRequested{false};
	Move bestMoveValue{};
	int nodesVisited{};
	// Number of moves made by this search, including speculative null moves.
	int currentPly{};
	int scoreGrain;
	int tableHitCount{};
	int pollFrequency{kDefaultPollFrequency};
	int pollsSinceYield{};
	int threadNumber{};
	std::atomic<std::time_t> killTime{0};
	int nullHitCount{};
	int lateMoveReductionCount{};
	std::array<int, maxMoves> cutoffs{};
	int contemptFactor{};
	int computerColor{white};
	bool nullVerification{};
};
} // namespace morphy_chess
