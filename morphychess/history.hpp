#pragma once

#include <array>
#include <morphychess/board.hpp>
#include <morphychess/params.hpp>

namespace morphy_chess {
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
