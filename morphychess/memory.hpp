#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>

#include <hpx/synchronization/mutex.hpp>

#include <morphychess/board.hpp>

namespace morphy_chess {
class Search;
class MemoryEntry {
	friend class MemoryTable;

private:
	std::uint64_t key{};
	I32 score{};
	Move move{};
	bool nullMovesVerified{};
	bool touched{};
	int depth{-1};
	// The repetition key omits this clock, but search/evaluation depend on it.
	int fiftyMoveRule{};
};

// Transposition bounds for repeated threshold searches (bibliography.md
// [mtdf]). Each color has separate upper/lower stores: per-worker slots always
// take the latest result, while shared slots prefer deeper verified results.
// A worker's Search/threadNumber pair must not be used concurrently.
class MemoryTable {
public:
	explicit MemoryTable(int entryCount = kTableSize,
						 int lockCount = kTableLocksPerThread,
						 int threadCount = 1);
	MemoryTable(MemoryTable const &) = delete;
	MemoryTable &operator=(MemoryTable const &) = delete;
	void reset();
	void resetTouchedFlags();
	void killUntouchedEntries();
	void write(Search &search, Move move, bool testResult, int testScore,
			   int depth);
	bool read(Search &search, Move &move, int &lowerBound, int &upperBound,
			  int minDepth, int testScore);

private:
	std::vector<std::unique_lock<hpx::mutex>> lockAll();
	std::array<std::vector<MemoryEntry>, 2> upperBoundLocalStore;
	std::array<std::vector<MemoryEntry>, 2> lowerBoundLocalStore;
	std::array<std::vector<MemoryEntry>, 2> upperBoundGlobalStore;
	std::array<std::vector<MemoryEntry>, 2> lowerBoundGlobalStore;
	std::unique_ptr<hpx::mutex[]> locks;
	int entryCount;
	int lockCount;
	int threadCount;
	int localEntryCount;
};
} // namespace morphy_chess
