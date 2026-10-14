#pragma once
#include <morphychess/integers.hpp>
namespace morphy_chess {
class PawnStatistics {
public:
	constexpr PawnStatistics(U64 lanes = 0, U64 moves = 0,
							 U64 weak = 0) noexcept :
		lanesValue(lanes), movesValue(moves), weakValue(weak) {}
	constexpr U64 lanes() const noexcept { return lanesValue; }
	constexpr U64 moves() const noexcept { return movesValue; }
	constexpr U64 weak() const noexcept { return weakValue; }

private:
	U64 lanesValue;
	U64 movesValue;
	U64 weakValue;
};
} // namespace morphy_chess
