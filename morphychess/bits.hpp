#pragma once

#include <array>
#include <cassert>
#include <cstdint>

#include <morphychess/bitboard.hpp>
#include <morphychess/move_data.hpp>

namespace morphy_chess {

/*********************************************************************************
MorphyChess 1.0.5 Copyright (C) 2008 Dominic C. Marcello
(dmarcello@phys.lsu.edu)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see [http://www.gnu.org/licenses/].
**********************************************************************************/

inline constexpr auto kRankIndex = 0;
inline constexpr auto kFileIndex = 1;
inline constexpr auto kDiag1Index = 2;
inline constexpr auto kDiag2Index = 3;

// Immutable masks for four occupancy representations. Each relevant line
// becomes a byte, enabling lookup by its 8-bit occupancy rather than scanning
// sliding-piece rays at runtime. See bibliography.md [rotated_bitboards].
class BitboardMasks {
public:
	constexpr BitboardMasks() noexcept {
		for (int square = 0; square < 64; ++square) {
			int const rank = square / 8;
			int const file = square % 8;
			// Diagonal bytes wrap modulo eight; MoveData clips their attacks to
			// the actual diagonal so unrelated wrapped squares cannot be reached.
			rotatedIndices[square] = {
				static_cast<std::int8_t>(square),
				static_cast<std::int8_t>(rank + 8 * file),
				static_cast<std::int8_t>(rank + 8 * ((file - rank + 8) % 8)),
				static_cast<std::int8_t>(rank + 8 * ((15 - file - rank) % 8))};
			for (int rotation = 0; rotation < 4; ++rotation) {
				masks[square][rotation] =
					Bitboard::mask(rotatedIndices[square][rotation]);
				inverseMasks[square][rotation] = ~masks[square][rotation];
			}
		}
	}

	[[nodiscard]] constexpr Bitboard mask(int square,
										  int rotation) const noexcept {
		return masks[square][rotation];
	}
	[[nodiscard]] constexpr Bitboard inverseMask(int square,
												 int rotation) const noexcept {
		return inverseMasks[square][rotation];
	}
	[[nodiscard]] constexpr int rotatedIndex(int square,
											 int rotation) const noexcept {
		return rotatedIndices[square][rotation];
	}

private:
	std::array<std::array<std::int8_t, 4>, 64> rotatedIndices{};
	std::array<std::array<Bitboard, 4>, 64> masks{};
	std::array<std::array<Bitboard, 4>, 64> inverseMasks{};
};

inline constexpr BitboardMasks bitboardMasks;

// Retained for callers that explicitly initialized the old runtime tables.
inline constexpr void initBits(void) noexcept {}

// All four views must change together. Index zero is ordinary occupancy;
// the other views are derived layouts, not separate pieces or colors.
class RotatedBitboard {
public:
	constexpr RotatedBitboard() noexcept = default;
	explicit constexpr RotatedBitboard(Bitboard value) noexcept {
		assign(value);
	}
	constexpr void assign(Bitboard value) noexcept {
		reset();
		while (value) {
			set(value.removeFirstOne());
		}
	}
	constexpr void set(int square) noexcept {
		for (int rotation = 0; rotation < 4; ++rotation) {
			boards[rotation] |= bitboardMasks.mask(square, rotation);
		}
	}
	constexpr void clear(int square) noexcept {
		for (int rotation = 0; rotation < 4; ++rotation) {
			boards[rotation] &= bitboardMasks.inverseMask(square, rotation);
		}
	}
	constexpr void reset() noexcept {
		for (auto &board : boards) {
			board = Bitboard{};
		}
	}
	constexpr Bitboard operator[](int index) const noexcept {
		return boards[index];
	}
	constexpr std::array<Bitboard, 4> const &rotations() const noexcept {
		return boards;
	}
	constexpr int indexFromRank(int square) const noexcept {
		int const shift = 8 * (square / 8);
		return static_cast<int>((boards[kRankIndex] >> shift).value() & 0xFF);
	}
	constexpr int indexFromFile(int square) const noexcept {
		return indexFromRotation(square, kFileIndex);
	}
	constexpr int indexFromDiag1(int square) const noexcept {
		return indexFromRotation(square, kDiag1Index);
	}
	constexpr int indexFromDiag2(int square) const noexcept {
		return indexFromRotation(square, kDiag2Index);
	}
	constexpr Bitboard rankMoves(int square) const noexcept {
		// Tables include the first blocker. The caller removes friendly pieces
		// or intersects with enemy pieces to obtain quiet moves or captures.
		return moveData.rankMoves(square, indexFromRank(square));
	}
	constexpr Bitboard fileMoves(int square) const noexcept {
		return moveData.fileMoves(square, indexFromFile(square));
	}
	constexpr Bitboard diag1Moves(int square) const noexcept {
		return moveData.diag1Moves(square, indexFromDiag1(square));
	}
	constexpr Bitboard diag2Moves(int square) const noexcept {
		return moveData.diag2Moves(square, indexFromDiag2(square));
	}
	constexpr Bitboard bishopAttacks(int square) const noexcept {
		return diag1Moves(square) | diag2Moves(square);
	}
	constexpr Bitboard rookAttacks(int square) const noexcept {
		return rankMoves(square) | fileMoves(square);
	}
	constexpr Bitboard queenAttacks(int square) const noexcept {
		assert(square >= 0 && square < 64);
		return bishopAttacks(square) | rookAttacks(square);
	}

private:
	constexpr int indexFromRotation(int square, int rotation) const noexcept {
		int const shift =
			8 * (bitboardMasks.rotatedIndex(square, rotation) / 8);
		return static_cast<int>((boards[rotation] >> shift).value() & 0xFF);
	}

	std::array<Bitboard, 4> boards{};
};

} // namespace morphy_chess
