#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>

#include <morphychess/parallel_search.hpp>

namespace morphy_chess {

// CECP/XBoard command state machine; see bibliography.md [xboard_protocol].
// The protocol task owns the live board and joins searches before changing it.
// Worker progress callbacks only publish output through emit().
class Xboard {
public:
	explicit Xboard(int workers = defaultThreadCount, bool useBook = true,
					std::string bookPath = kBookName,
					std::ostream &output = std::cout,
					int memoryEntries = kTableSize);
	~Xboard();
	Xboard(const Xboard &) = delete;
	Xboard &operator=(const Xboard &) = delete;

	bool processCommand(const std::string &command);
	void pollSearch();
	int run(int inputFd = 0);
	const Board &board() const noexcept { return currentBoard; }
	bool quitting() const noexcept { return quitRequested; }
	bool hasDrawOffer() const noexcept { return drawOffered; }
	bool searching() const noexcept { return searchMode != SearchMode::none; }

private:
	enum class SearchMode { none, move, ponder, analysis };
	void emit(const std::string &text);
	SearchResult stopSearch();
	void applySearchResult(const SearchResult &result);
	void reportResult();
	void positionChanged(bool keepDrawOffer = false);
	void showBookMoves();
	void showProgress(const SearchResult &result);
	void sendIcsSeeks();
	double searchSeconds() const;
	void invalidArgument(const std::string &command);

	Board currentBoard;
	StaticEvaluator evaluator;
	MemoryTable memoryTable;
	ParallelSearch parallelSearch;
	std::ostream &output;
	hpx::mutex outputMutex;
	std::atomic<bool> posting{false};
	std::chrono::steady_clock::time_point searchStarted;
	SearchMode searchMode = SearchMode::none;
	bool useBook = true;
	bool icsActive = false;
	bool forceMode = true;
	bool analyzeMode = false;
	bool ponderEnabled = true;
	bool ponderCompleted = false;
	bool analysisCompleted = false;
	bool paused = false;
	bool quitRequested = false;
	bool illegalPosition = false;
	bool drawOffered = false;
	int computerColor = black;
	int depthLimit = kMaxSearchDepth;
	int engineRating = 0;
	int opponentRating = 0;
	int contemptFactor = 0;
	int lastDrawOffer = -kDrawOfferFreq;
	int movesPerControl = 0;
	double remainingSeconds = 300.0;
	double opponentSeconds = 300.0;
	double fixedMoveSeconds = 0.0;
	double incrementSeconds = 0.0;
	Result gameResult = inProgress;
	std::string opponentName;
	std::string lastOpponentName;
};

} // namespace morphy_chess
