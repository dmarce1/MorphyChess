#pragma once

#include <array>
#include <limits>

#include <morphychess/board.hpp>
#include <morphychess/params.hpp>

namespace morphy_chess {
// Compile-time default bonuses grow as 4^(depth-1), saturating below INT_MAX
// so score accumulation can be made safe by one global halving.
class HistoryIncrements {
public:
	constexpr HistoryIncrements() {
		static_assert(kDefaultHistoryBranchFactor == 4.0);
		constexpr int maximumIncrement = std::numeric_limits<int>::max() / 2;
		int increment = 1;
		for (int depth = 1; depth < kMaxDepth; ++depth) {
			increments[depth] = increment;
			increment = increment > maximumIncrement / 4 ? maximumIncrement
														 : increment * 4;
		}
	}
	constexpr int operator[](int depth) const { return increments[depth]; }
	constexpr std::array<int, kMaxDepth> const &values() const {
		return increments;
	}

private:
	std::array<int, kMaxDepth> increments{};
};

// Per-worker move ordering learned from quiet cutoffs. The from/to history
// table is shared across plies and colors, while two killer slots are per ply.
// See bibliography.md [history_heuristic].
class History {
public:
	explicit History(double branchFactor = kDefaultHistoryBranchFactor);
	void reset();
	void update(Move move, bool test, int depth, int ply, bool isNonCap);
	int moveHistory(Move move, int ply = 0) const;
	Move killer1(int ply) const;
	Move killer2(int ply) const;

private:
	void halveScores();
	std::array<std::array<int, 64>, 64> scores{};
	std::array<int, kMaxDepth> updateIncrements{};
	std::array<Move, kMaxDepth> firstKillers{};
	std::array<Move, kMaxDepth> secondKillers{};
};
} // namespace morphy_chess
