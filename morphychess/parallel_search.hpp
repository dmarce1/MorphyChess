#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

#include <hpx/modules/futures.hpp>
#include <hpx/synchronization/mutex.hpp>

#include <morphychess/board.hpp>
#include <morphychess/book.hpp>
#include <morphychess/search.hpp>

namespace morphy_chess {
// Score is relative to the root side to move. Depth zero denotes a book or
// fallback move; otherwise depth identifies the deepest completed iteration.
class SearchResult {
public:
	SearchResult() = default;
	SearchResult(Move move, int score, int depth, int nodes) :
		selectedMove(move),
		evaluation(score),
		completedDepth(depth),
		visitedNodes(nodes) {}
	Move move() const { return selectedMove; }
	int score() const { return evaluation; }
	int depth() const { return completedDepth; }
	int nodes() const { return visitedNodes; }

private:
	Move selectedMove{};
	int evaluation{};
	int completedDepth{};
	int visitedNodes{};
};

// Coordinate independent null-window root searches over score thresholds.
// Workers own board/search state and share evaluation/transposition caches;
// the scheduler combines their answers into bounds for each search depth.
// This is the engine's bound-splitting policy, not subtree splitting.
class ParallelSearch {
public:
	ParallelSearch(Board &board, StaticEvaluator &evaluator, MemoryTable &table,
				   int workerCount = defaultThreadCount);
	~ParallelSearch();
	ParallelSearch(const ParallelSearch &) = delete;
	ParallelSearch &operator=(const ParallelSearch &) = delete;
	SearchResult run(int depth, double seconds = 0.0, int initialGuess = 0);
	void start(int depth, double seconds = 0.0);
	bool ready() const;
	bool running() const { return active.load(); }
	SearchResult finish();
	SearchResult stop();
	void resetMemory();
	void setUseBook(bool value);
	void setContemptFactor(int value);
	void setComputerColor(int value);
	// Handlers run on search tasks; they must not call blocking lifecycle
	// methods.
	void setProgressHandler(std::function<void(const SearchResult &)> handler);
	Book &openingBook() { return book; }
	const Book &openingBook() const { return book; }
	void setLearning(bool value);
	void triggerLearning(int computerSide, Result result);
	const EvaluatorParameters &parameters() const { return evalParams; }

private:
	enum class Stage { waiting, searching, finalSearching, complete };
	SearchResult runImpl(int depth, double seconds, int initialGuess);
	void prepare(int depth, double seconds, int initialGuess);
	void worker(int index);
	bool nextSearch(int index, int &testValue, int &depth,
					std::vector<SearchResult> &notifications);
	void removeOutliers(int depth, int workerIndex);
	void requestStop();
	Board &rootBoard;
	StaticEvaluator &evaluator;
	MemoryTable &table;
	std::vector<std::unique_ptr<Board>> boards;
	std::vector<std::unique_ptr<Search>> searches;
	std::vector<int> depths;
	std::vector<int> testValues;
	// Scheduler bounds are inter-grain thresholds; a gap of one grain pins
	// down a score. Entry zero supplies the initial guess for depth one.
	std::array<int, kMaxDepth> upperBounds{};
	std::array<int, kMaxDepth> lowerBounds{};
	std::array<Stage, kMaxDepth> stages{};
	std::array<Move, kMaxDepth> bestMoves{};
	hpx::mutex mutex;
	hpx::mutex progressMutex;
	hpx::future<SearchResult> pending;
	std::atomic<bool> active{false};
	std::atomic<bool> stopRequested{false};
	bool searchDone{};
	bool useBook{true};
	int workerCount;
	int maximumDepth{};
	int completedDepth{};
	int nodeCount{};
	int grainSize{kDefaultGrainSize};
	int contemptFactor{};
	int computerColor{white};
	bool timed{};
	std::chrono::steady_clock::time_point deadline;
	std::function<void(const SearchResult &)> progressHandler;
	Book book;
	bool learningOn{};
	std::vector<Board> learningPositions;
	EvaluatorParameters evalParams{defaultEvaluatorParameters};
};
} // namespace morphy_chess
