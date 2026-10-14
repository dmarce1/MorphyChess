#pragma once

#include <array>

#include <morphychess/board.hpp>
#include <morphychess/history.hpp>
#include <morphychess/params.hpp>
#include <morphychess/search.hpp>

namespace morphy_chess {
enum class GenStage {
	start,
	table,
	promotions,
	losingCaptures,
	captures,
	noncaptures,
	end
};
inline constexpr int kNodeTerminal = 1;
inline constexpr int kNodeMovesAvailable = 2;
inline constexpr int kNodeNoMoreMoves = 3;

// Stateful, staged move picker for a single parent position. Successful next
// calls leave a legal child on Search's board; the caller must restore() before
// asking for another move. Use either the full or tactical picker per instance.
class Node {
public:
	Node(Search &search, Move move);
	int totalMoves() const;
	int doNextMove(Move &branch);
	int doNextNonquiescentMove(Move &branch);
	void restore();
	bool noMovesGenerated() const;
	bool okToReduce(int depth) const;
	bool moveIsFutile(int materialBalance, int testValue) const;

private:
	int captureMoveValue(Move move) const;
	std::array<Move, maxMoves> list{};
	std::array<int, maxMoves> scores{};
	GenStage mgStage{GenStage::start};
	Search &search;
	int totalMovesGenerated{};
	int movesInQueue{};
	Move killer{};
	Move tableMove{};
	bool firstNoncap{};
};
} // namespace morphy_chess
