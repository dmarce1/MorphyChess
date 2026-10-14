#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <stdexcept>

#include <morphychess/search.hpp>

namespace morphy_chess {
MemoryTable::MemoryTable(int requestedEntries, int requestedLocks,
						 int requestedThreads) :
	entryCount(requestedEntries / 8),
	lockCount(requestedLocks),
	threadCount(requestedThreads),
	localEntryCount(0) {
	// Eight arrays split capacity across two colors, two bound directions,
	// and local/global replacement policies. Local partitions round down.
	if (threadCount < 1 || entryCount < threadCount || lockCount < 1 ||
		lockCount > entryCount) {
		throw std::invalid_argument(
			"Invalid transposition table size, lock count, or thread count");
	}
	localEntryCount = entryCount / threadCount;
	locks = std::make_unique<hpx::mutex[]>(lockCount);
	for (int color = 0; color < 2; ++color) {
		upperBoundLocalStore[color].resize(entryCount);
		lowerBoundLocalStore[color].resize(entryCount);
		upperBoundGlobalStore[color].resize(entryCount);
		lowerBoundGlobalStore[color].resize(entryCount);
	}
}

std::vector<std::unique_lock<hpx::mutex>> MemoryTable::lockAll() {
	// Acquire stripes in one order so whole-table maintenance cannot deadlock
	// with another maintenance operation.
	std::vector<std::unique_lock<hpx::mutex>> guards;
	guards.reserve(lockCount);
	for (int index = 0; index < lockCount; ++index) {
		guards.emplace_back(locks[index]);
	}
	return guards;
}

void MemoryTable::reset() {
	const auto guards = lockAll();
	for (auto *store : {&upperBoundLocalStore, &lowerBoundLocalStore,
						&upperBoundGlobalStore, &lowerBoundGlobalStore}) {
		for (auto &color : *store) {
			std::fill(color.begin(), color.end(), MemoryEntry{});
		}
	}
}

void MemoryTable::resetTouchedFlags() {
	const auto guards = lockAll();
	for (auto *store : {&upperBoundLocalStore, &lowerBoundLocalStore,
						&upperBoundGlobalStore, &lowerBoundGlobalStore}) {
		for (auto &color : *store) {
			for (auto &entry : color) {
				entry.touched = false;
			}
		}
	}
}

void MemoryTable::killUntouchedEntries() {
	const auto guards = lockAll();
	for (auto *store : {&upperBoundLocalStore, &lowerBoundLocalStore,
						&upperBoundGlobalStore, &lowerBoundGlobalStore}) {
		for (auto &color : *store) {
			for (auto &entry : color) {
				if (!entry.touched) {
					entry = MemoryEntry{};
				}
			}
		}
	}
}

void MemoryTable::write(Search &search, Move move, bool testResult,
						int testScore, int depth) {
	// Repetition results depend on the path; mate scores depend on root ply.
	// Neither can be reused as an ordinary position-only cached bound here.
	if (search.boardPtr->positionIsRepeat() ||
		std::abs(testScore) >= kMinCheckmateValue) {
		return;
	}
	if (search.threadNumber < 0 || search.threadNumber >= threadCount) {
		throw std::out_of_range(
			"Search thread index exceeds transposition table partition count");
	}
	const std::uint64_t key = search.boardPtr->positionKey();
	const auto key32 = static_cast<U32>(key);
	const int globalIndex = key32 % entryCount;
	const int localIndex =
		search.threadNumber * localEntryCount + key32 % localEntryCount;
	const int colorIndex = search.boardPtr->sideToMove();
	const std::lock_guard<hpx::mutex> lock(locks[globalIndex % lockCount]);
	MemoryEntry entry;
	entry.key = key;
	// testScore lies between score grains. Shift to the proved side of the
	// threshold before storing its bound direction.
	entry.score = testScore +
				  (testResult ? search.scoreGrain / 2 : -search.scoreGrain / 2);
	entry.move = move;
	entry.depth = depth;
	entry.fiftyMoveRule = search.boardPtr->fiftyMoveRule();
	entry.touched = true;
	entry.nullMovesVerified = search.verifyNullMove;
	auto &localStore = testResult ? lowerBoundLocalStore : upperBoundLocalStore;
	auto &globalStore =
		testResult ? lowerBoundGlobalStore : upperBoundGlobalStore;
	localStore[colorIndex][localIndex] = entry;
	if (entry.nullMovesVerified &&
		depth >= globalStore[colorIndex][globalIndex].depth) {
		globalStore[colorIndex][globalIndex] = entry;
	}
}

bool MemoryTable::read(Search &search, Move &move, int &lowerBound,
					   int &upperBound, int minDepth, int) {
	upperBound = kInfty;
	lowerBound = -kInfty;
	move.setEmpty();
	if (search.boardPtr->positionIsRepeat()) {
		return false;
	}
	if (search.threadNumber < 0 || search.threadNumber >= threadCount) {
		throw std::out_of_range(
			"Search thread index exceeds transposition table partition count");
	}
	const std::uint64_t key = search.boardPtr->positionKey();
	const auto key32 = static_cast<U32>(key);
	const int globalIndex = key32 % entryCount;
	const int localIndex =
		search.threadNumber * localEntryCount + key32 % localEntryCount;
	const int colorIndex = search.boardPtr->sideToMove();
	const std::lock_guard<hpx::mutex> lock(locks[globalIndex % lockCount]);
	const int fiftyMoveRule = search.boardPtr->fiftyMoveRule();
	const auto matches = [key, fiftyMoveRule](const MemoryEntry &entry) {
		return entry.key == key && entry.depth >= 0 &&
			   entry.fiftyMoveRule == fiftyMoveRule;
	};
	auto *upper = &upperBoundGlobalStore[colorIndex][globalIndex];
	if (!matches(*upper)) {
		upper = &upperBoundLocalStore[colorIndex][localIndex];
	}
	bool found = false;
	int upperDepth = -1;
	if (matches(*upper) &&
		!(search.verifyNullMove && !upper->nullMovesVerified)) {
		upper->touched = true;
		move = upper->move;
		upperDepth = upper->depth;
		if (minDepth <= upper->depth) {
			found = true;
			upperBound = upper->score;
		}
	}
	auto *lower = &lowerBoundGlobalStore[colorIndex][globalIndex];
	if (!matches(*lower)) {
		lower = &lowerBoundLocalStore[colorIndex][localIndex];
	}
	if (matches(*lower) &&
		!(search.verifyNullMove && !lower->nullMovesVerified)) {
		lower->touched = true;
		if (upperDepth <= lower->depth || move.isEmpty()) {
			move = lower->move;
		}
		if (minDepth <= lower->depth) {
			found = true;
			lowerBound = lower->score;
		}
	}
	// A shallower match can still supply a move-ordering hint, even when its
	// score is not deep enough to narrow the caller's bounds.
	return found && !move.isEmpty();
}
} // namespace morphy_chess
