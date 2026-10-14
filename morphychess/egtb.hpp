#pragma once
#include <morphychess/board.hpp>
namespace morphy_chess {
inline constexpr int threeManPositionCount = 64 * 63 * 62;
bool egtbIndexToBoard(Board *board, int thirdMan, int index, int sideToMove);
int threeManEgtbIndex(const Board &board, int thirdMan, int pieceColor);
} // namespace morphy_chess
