#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/threading.hpp>
#include <morphychess/parallel_search.hpp>
#include <mutex>
#include <stdexcept>

namespace morphy_chess {
ParallelSearch::ParallelSearch(Board &board, StaticEvaluator &evaluator,
							   MemoryTable &table, int workerCount) :
	rootBoard(board),
	evaluator(evaluator),
	table(table),
	workerCount(workerCount) {
	if (workerCount < 1) {
		throw std::invalid_argument("Search worker count must be positive");
	}
	depths.resize(workerCount, -1);
	testValues.resize(workerCount);
	for (int index = 0; index < workerCount; ++index) {
		boards.push_back(std::make_unique<Board>(board));
		searches.push_back(
			std::make_unique<Search>(*boards.back(), evaluator, table));
		searches.back()->setThreadNumber(index);
	}
}
ParallelSearch::~ParallelSearch() {
	try {
		stop();
	} catch (...) {
		// Futures are joined before any worker state is destroyed.
	}
}
void ParallelSearch::setUseBook(bool value) {
	stop();
	useBook = value;
}
void ParallelSearch::setContemptFactor(int value) {
	stop();
	if (contemptFactor == value) {
		return;
	}
	// Cached bounds include the utility assigned to a draw.
	table.reset();
	contemptFactor = value;
	for (auto &search : searches) {
		search->setContemptFactor(value);
	}
}
void ParallelSearch::setComputerColor(int value) {
	stop();
	if (computerColor == value) {
		return;
	}
	// Changing the engine side reverses the draw preference at a given
	// position.
	table.reset();
	computerColor = value;
	for (auto &search : searches) {
		search->setComputerColor(value);
	}
}
void ParallelSearch::setProgressHandler(
	std::function<void(const SearchResult &)> handler) {
	stop();
	progressHandler = std::move(handler);
}
void ParallelSearch::resetMemory() {
	stop();
	table.reset();
	evaluator.resetPawnTable();
}
void ParallelSearch::prepare(int depth, double seconds, int initialGuess) {
	if (depth < 1 || depth > kMaxSearchDepth || seconds < 0.0 ||
		!std::isfinite(seconds) ||
		std::abs(static_cast<long long>(initialGuess)) > kInfty - grainSize) {
		throw std::invalid_argument("Invalid search depth or time limit");
	}
	const auto now = std::chrono::steady_clock::now();
	const double maximumSeconds =
		std::chrono::duration<double>(
			std::chrono::steady_clock::time_point::max() - now)
			.count();
	if (seconds >= maximumSeconds) {
		throw std::invalid_argument(
			"Search time limit exceeds the clock range");
	}
	maximumDepth = depth;
	completedDepth = nodeCount = 0;
	searchDone = false;
	stopRequested.store(false);
	timed = seconds > 0.0;
	deadline =
		now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
				  std::chrono::duration<double>(seconds));
	upperBounds.fill(kInfty);
	lowerBounds.fill(-kInfty);
	stages.fill(Stage::waiting);
	bestMoves.fill(Move{});
	lowerBounds[0] = initialGuess - grainSize / 2;
	upperBounds[0] = initialGuess + grainSize / 2;
	std::fill(depths.begin(), depths.end(), -1);
	table.resetTouchedFlags();
	for (int index = 0; index < workerCount; ++index) {
		*boards[index] = rootBoard;
		searches[index]->clearStop();
		searches[index]->setKillTime(0);
		searches[index]->resetCounters();
	}
}
SearchResult ParallelSearch::run(int depth, double seconds, int initialGuess) {
	if (active.load() || pending.valid()) {
		throw std::logic_error("A search is already running");
	}
	prepare(depth, seconds, initialGuess);
	active.store(true);
	return runImpl(depth, seconds, initialGuess);
}
void ParallelSearch::start(int depth, double seconds) {
	stop();
	prepare(depth, seconds, 0);
	active.store(true);
	try {
		pending = hpx::async(
			[this, depth, seconds] { return runImpl(depth, seconds, 0); });
	} catch (...) {
		active.store(false);
		throw;
	}
}
bool ParallelSearch::ready() const {
	return pending.valid() && pending.is_ready();
}
SearchResult ParallelSearch::finish() {
	if (!pending.valid()) {
		return {};
	}
	return pending.get();
}
void ParallelSearch::requestStop() {
	stopRequested.store(true);
	std::lock_guard<hpx::mutex> guard(mutex);
	searchDone = true;
	for (auto &search : searches) {
		search->requestStop();
	}
}
SearchResult ParallelSearch::stop() {
	if (pending.valid()) {
		requestStop();
		return finish();
	}
	return {};
}
SearchResult ParallelSearch::runImpl(int, double, int) {
	std::vector<hpx::future<void>> workers;
	hpx::future<void> timer;
	std::atomic<bool> timerDone{false};
	try {
		// Reserve before spawning: every launched task must have a retained
		// future.
		workers.reserve(workerCount);
		Move fallback;
		std::array<Move, maxMoves> moves;
		const int moveCount = rootBoard.genPseudolegalMoves(moves.data());
		for (int index = 0; index < moveCount; ++index) {
			if (rootBoard.isLegalMove(moves[index])) {
				fallback = moves[index];
				break;
			}
		}
		if (fallback.isEmpty() || rootBoard.result() != inProgress) {
			active.store(false);
			return {};
		}
		if (useBook) {
			const Move bookMove = book.findMove(rootBoard);
			if (!bookMove.isEmpty() && rootBoard.isLegalMove(bookMove)) {
				active.store(false);
				return SearchResult(bookMove, 0, 0, 0);
			}
		}
		if (timed) {
			timer = hpx::async([this, &timerDone] {
				while (!timerDone.load(std::memory_order_relaxed)) {
					const auto now = std::chrono::steady_clock::now();
					if (now >= deadline) {
						requestStop();
						break;
					}
					hpx::this_thread::sleep_until(
						std::min(deadline, now + std::chrono::milliseconds(1)));
				}
			});
		}
		for (int index = 0; index < workerCount; ++index) {
			workers.push_back(hpx::async([this, index] {
				try {
					worker(index);
				} catch (...) {
					// Cancel siblings promptly, including when an earlier
					// future is still running.
					requestStop();
					throw;
				}
			}));
		}
		std::exception_ptr error;
		for (auto &future : workers) {
			try {
				future.get();
			} catch (...) {
				requestStop();
				if (!error) {
					error = std::current_exception();
				}
			}
		}
		timerDone.store(true, std::memory_order_relaxed);
		if (timer.valid()) {
			timer.get();
		}
		if (error) {
			std::rethrow_exception(error);
		}
		Move selected =
			completedDepth > 0 ? bestMoves[completedDepth] : fallback;
		if (selected.isEmpty() || !rootBoard.isLegalMove(selected)) {
			selected = fallback;
		}
		const int score = completedDepth > 0
							  ? lowerBounds[completedDepth] + grainSize / 2
							  : 0;
		if (learningOn) {
			Board leaf = rootBoard;
			std::array<Move, kMaxDepth + 1> variation;
			searches.front()->setBestMove(selected);
			searches.front()->reconstructPrincipalVariation(
				&leaf, variation, completedDepth, false);
			learningPositions.push_back(leaf);
		}
		active.store(false);
		return SearchResult(selected, score, completedDepth, nodeCount);
	} catch (...) {
		timerDone.store(true, std::memory_order_relaxed);
		requestStop();
		for (auto &future : workers) {
			if (future.valid()) {
				future.wait();
			}
		}
		if (timer.valid()) {
			timer.wait();
		}
		active.store(false);
		throw;
	}
}
// Retain the original bound-splitting search scheduler. The mutex protects
// scheduling metadata; each task searches its own board outside this lock.
bool ParallelSearch::nextSearch(int index, int &testValue, int &selectedDepth,
								std::vector<SearchResult> &notifications) {
	if (searchDone || stopRequested.load() ||
		(timed && std::chrono::steady_clock::now() >= deadline)) {
		return false;
	}
	for (int depth = 1; depth <= maximumDepth; ++depth) {
		bool found = false;
		if (stages[depth] == Stage::waiting) {
			testValue = (lowerBounds[depth - 1] + upperBounds[depth - 1]) / 2;
			testValue = testValue / grainSize * grainSize - grainSize / 2;
			stages[depth] = Stage::searching;
			found = true;
		} else if (stages[depth] == Stage::searching) {
			int high = lowerBounds[depth];
			int low = upperBounds[depth];
			for (int other = 0; other < workerCount; ++other) {
				if (other != index && depths[other] == depth) {
					high = std::max(high, testValues[other]);
					low = std::min(low, testValues[other]);
				}
			}
			high += grainSize;
			low -= grainSize;
			if (high < upperBounds[depth] && low > lowerBounds[depth]) {
				int guess =
					(lowerBounds[depth - 1] + upperBounds[depth - 1]) / 2;
				guess = guess / grainSize * grainSize - grainSize / 2;
				testValue = std::abs(high - guess) <= std::abs(low - guess)
								? high
								: low;
				found = true;
			} else if (high < upperBounds[depth]) {
				testValue = high;
				found = true;
			} else if (low > lowerBounds[depth]) {
				testValue = low;
				found = true;
			} else {
				stages[depth] = Stage::finalSearching;
			}
		}
		if (!found && stages[depth] == Stage::finalSearching &&
			upperBounds[depth] - lowerBounds[depth] <= grainSize) {
			stages[depth] = Stage::complete;
			completedDepth = std::max(completedDepth, depth);
			if (progressHandler) {
				notifications.emplace_back(bestMoves[depth],
										   lowerBounds[depth] + grainSize / 2,
										   depth, nodeCount);
			}
		}
		if (stages[depth] == Stage::complete &&
			(depth == maximumDepth ||
			 std::abs(lowerBounds[depth]) >= kMinCheckmateValue)) {
			searchDone = true;
			for (auto &search : searches) {
				search->requestStop();
			}
			return false;
		}
		if (found) {
			selectedDepth = depth;
			return true;
		}
	}
	return false;
}
void ParallelSearch::removeOutliers(int depth, int workerIndex) {
	for (int index = 0; index < workerCount; ++index) {
		if (index != workerIndex && depths[index] == depth &&
			(testValues[index] >= upperBounds[depth] ||
			 testValues[index] <= lowerBounds[depth])) {
			searches[index]->requestStop();
		}
	}
}
void ParallelSearch::worker(int index) {
	auto &search = *searches[index];
	std::unique_lock<hpx::mutex> guard(mutex);
	std::vector<SearchResult> notifications;
	notifications.reserve(maximumDepth);
	int testValue = 0;
	int depth = 0;
	while (true) {
		notifications.clear();
		const bool hasWork = nextSearch(index, testValue, depth, notifications);
		if (hasWork) {
			depths[index] = depth;
			testValues[index] = testValue;
			search.clearStop();
		}
		guard.unlock();
		if (!notifications.empty()) {
			// Call client code without the scheduling mutex. Serialize progress
			// events.
			const std::lock_guard<hpx::mutex> progressGuard(progressMutex);
			for (const auto &result : notifications) {
				progressHandler(result);
			}
		}
		if (!hasWork) {
			return;
		}
		const bool result = search.test(testValue, depth);
		guard.lock();
		nodeCount += search.nodeCount();
		search.resetCounters();
		depths[index] = -1;
		if (!search.stopped() && testValue < upperBounds[depth] &&
			testValue > lowerBounds[depth]) {
			if (result && !search.bestMove().isEmpty()) {
				lowerBounds[depth] = testValue;
				bestMoves[depth] = search.bestMove();
			} else if (!result) {
				upperBounds[depth] = testValue;
			}
		}
		removeOutliers(depth, index);
	}
}
} // namespace morphy_chess
