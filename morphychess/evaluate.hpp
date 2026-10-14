#pragma once
#include <morphychess/board.hpp>

namespace morphy_chess {
// Compatibility initialization hook for Board's fixed material evaluator;
// its tables are constexpr, so no runtime initialization is required.
void initEvaluator();
} // namespace morphy_chess
