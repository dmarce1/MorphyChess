#include <morphychess/history.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace morphy_chess {
namespace {
constexpr HistoryIncrements defaultIncrements;
} // namespace

History::History(double branchFactor) {
	if (!std::isfinite(branchFactor) || branchFactor <= 0.0) {
		throw std::invalid_argument(
			"History branch factor must be positive and finite");
	}
	if (branchFactor == kDefaultHistoryBranchFactor) {
		updateIncrements = defaultIncrements.values();
	} else {
		for (int depth = 0; depth < kMaxDepth; ++depth) {
			const double increment = std::pow(branchFactor, depth - 1);
			constexpr int maximumIncrement =
				std::numeric_limits<int>::max() / 2;
			updateIncrements[depth] = increment >= maximumIncrement
										  ? maximumIncrement
										  : static_cast<int>(increment);
		}
	}
	reset();
}

void History::reset() {
	for (auto &row : scores) {
		row.fill(0);
	}
	firstKillers.fill(Move{});
	secondKillers.fill(Move{});
}

void History::halveScores() {
	for (auto &row : scores) {
		for (int &score : row) {
			score /= 2;
		}
	}
}

void History::update(Move move, bool test, int depth, int ply, bool isNonCap) {
	if (!test || move.isEmpty() || !isNonCap) {
		return;
	}
	if (depth < 1 || ply < 0 || ply >= kMaxDepth) {
		throw std::out_of_range(
			"History depth and ply must be within the search bounds");
	}
	const int increment = updateIncrements[std::min(depth, kMaxDepth - 1)];
	int &score = scores.at(move.from()).at(move.to());
	if (score > std::numeric_limits<int>::max() - increment) {
		halveScores();
	}
	score += increment;
	if (move != firstKillers[ply]) {
		secondKillers[ply] = firstKillers[ply];
		firstKillers[ply] = move;
	}
}

Move History::killer1(int ply) const {
	return ply >= 0 && ply < kMaxDepth ? firstKillers[ply] : Move{};
}

Move History::killer2(int ply) const {
	return ply >= 0 && ply < kMaxDepth ? secondKillers[ply] : Move{};
}

int History::moveHistory(Move move, int) const {
	return scores.at(move.from()).at(move.to());
}
} // namespace morphy_chess
