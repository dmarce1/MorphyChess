#pragma once

#include <array>
#include <limits>
#include <morphychess/board.hpp>
#include <morphychess/params.hpp>

namespace morphy_chess {
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
	constexpr const std::array<int, kMaxDepth> &values() const {
		return increments;
	}

private:
	std::array<int, kMaxDepth> increments{};
};

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
