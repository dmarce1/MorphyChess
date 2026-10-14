#pragma once
#include <morphychess/board.hpp>

namespace morphy_chess {
// Ordered distinct-square placements for two kings and one extra piece.
// This counts placement indices, not legal positions or solved game values;
// side to move is supplied separately to loadEgtbIndex.
inline constexpr int threeManPositionCount = 64 * 63 * 62;
} // namespace morphy_chess
