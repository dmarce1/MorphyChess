#pragma once
#include <morphychess/bitboard.hpp>

namespace morphy_chess {
// Lightweight holder for caller-defined pawn lane, move, and weakness masks.
// This type computes no statistics itself; StaticEvaluator currently derives
// its pawn features directly from Board rather than consuming these masks.
class PawnStatistics {
public:
	constexpr PawnStatistics(Bitboard lanes = {}, Bitboard moves = {},
							 Bitboard weak = {}) noexcept :
		lanesValue(lanes), movesValue(moves), weakValue(weak) {}
	constexpr Bitboard lanes() const noexcept { return lanesValue; }
	constexpr Bitboard moves() const noexcept { return movesValue; }
	constexpr Bitboard weak() const noexcept { return weakValue; }

private:
	Bitboard lanesValue;
	Bitboard movesValue;
	Bitboard weakValue;
};
} // namespace morphy_chess
