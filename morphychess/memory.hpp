#pragma once

#include <morphychess/board.hpp>

#include <hpx/synchronization/mutex.hpp>

#include <array>
#include <memory>
#include <mutex>
#include <vector>

namespace morphy_chess {
class Search;
class MemoryEntry {
	friend class MemoryTable;

private:
	std::uint64_t key{};
	I32 score{};
	Move move{};
	int depth{-1};
	bool nullMovesVerified{};
	bool touched{};
};

class MemoryTable {
public:
	explicit MemoryTable(int entryCount = kTableSize,
						 int lockCount = kTableLocksPerThread,
						 int threadCount = 1);
	MemoryTable(const MemoryTable &) = delete;
	MemoryTable &operator=(const MemoryTable &) = delete;
	void reset();
	void resetTouchedFlags();
	void killUntouchedEntries();
	void write(Search &search, Move move, bool testResult, int testScore,
			   int depth);
	bool read(Search &search, Move *move, int *lowerBound, int *upperBound,
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
