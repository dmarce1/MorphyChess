#pragma once

#include <array>

namespace morphy_chess {
class MaterialValues {
public:
	constexpr explicit MaterialValues(std::array<int, 7> pieceValues) :
		values(pieceValues) {}
	constexpr int operator[](int piece) const { return values[piece]; }

private:
	std::array<int, 7> values;
};
} // namespace morphy_chess
