#pragma once

#include <array>

namespace morphy_chess {
// Piece-type indexed scores (empty through king). Callers choose the scale
// and pass a nonnegative piece type, usually abs(Board::pieceValue(...)).
class MaterialValues {
public:
	constexpr explicit MaterialValues(std::array<int, 7> pieceValues) :
		values(pieceValues) {}
	constexpr int operator[](int piece) const { return values[piece]; }

private:
	std::array<int, 7> values;
};
} // namespace morphy_chess
