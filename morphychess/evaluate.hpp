#pragma once
#include <morphychess/board.hpp>
namespace morphy_chess {
void initEvaluator();
int evaluate(const Board &board);
} // namespace morphy_chess
