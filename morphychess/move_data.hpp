#pragma once

#include <array>
#include <cstdint>
#include <morphychess/bitboard.hpp>

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

// Immutable attack tables. Occupancy bytes use files for ranks and ranks for
// files/diagonals, matching RotatedBitboard's four occupancy indices.
class MoveData {
public:
	constexpr MoveData() noexcept;

	[[nodiscard]] constexpr Bitboard
	whitePawnAttacks(int square) const noexcept {
		return wpAttacks[square];
	}
	[[nodiscard]] constexpr Bitboard
	blackPawnAttacks(int square) const noexcept {
		return bpAttacks[square];
	}
	[[nodiscard]] constexpr Bitboard knightAttacks(int square) const noexcept {
		return nAttacks[square];
	}
	[[nodiscard]] constexpr Bitboard kingAttacks(int square) const noexcept {
		return kAttacks[square];
	}
	[[nodiscard]] constexpr Bitboard rankMoves(int square,
											   int occupancy) const noexcept {
		return rankAttacks[square][occupancy];
	}
	[[nodiscard]] constexpr Bitboard fileMoves(int square,
											   int occupancy) const noexcept {
		return fileAttacks[square][occupancy];
	}
	[[nodiscard]] constexpr Bitboard diag1Moves(int square,
												int occupancy) const noexcept {
		return diag1Attacks[square][occupancy];
	}
	[[nodiscard]] constexpr Bitboard diag2Moves(int square,
												int occupancy) const noexcept {
		return diag2Attacks[square][occupancy];
	}

private:
	constexpr auto makeLineAttacks() const noexcept {
		std::array<std::array<unsigned, 256>, 8> attacks{};
		for (int from = 0; from < 8; ++from) {
			for (int index = 0; index < 256; ++index) {
				unsigned moves = 0;
				for (int to = from + 1; to < 8; ++to) {
					moves |= 1U << to;
					if ((index & (1U << to)) != 0) {
						break;
					}
				}
				for (int to = from - 1; to >= 0; --to) {
					moves |= 1U << to;
					if ((index & (1U << to)) != 0) {
						break;
					}
				}
				attacks[from][index] = moves;
			}
		}
		return attacks;
	}

	constexpr auto makeSpreadMasks(int stride, int offset) const noexcept {
		std::array<std::uint64_t, 256> masks{};
		for (int index = 0; index < 256; ++index) {
			for (int rank = 0; rank < 8; ++rank) {
				if ((index & (1U << rank)) != 0) {
					masks[index] |= std::uint64_t{1}
									<< (offset + stride * rank);
				}
			}
		}
		return masks;
	}

	constexpr unsigned rankMask(int first, int last) const noexcept {
		return ((1U << (last + 1)) - 1) & ~((1U << first) - 1);
	}

	constexpr Bitboard genWhitePawnAttacks(int square) const noexcept {
		const int side = (square & 0x7) >> 2;
		const Bitboard mask = Bitboard::mask(square);
		return ((mask << 7) | (mask << 9)) & ~(side ? fileA : fileH);
	}
	constexpr Bitboard genBlackPawnAttacks(int square) const noexcept {
		const int side = (square & 0x7) >> 2;
		const Bitboard mask = Bitboard::mask(square);
		return ((mask >> 7) | (mask >> 9)) & ~(side ? fileA : fileH);
	}
	constexpr Bitboard genKingAttacks(int square) const noexcept {
		const int side = (square & 0x7) >> 2;
		Bitboard mask = Bitboard::mask(square);
		mask |= (mask >> 1) | (mask << 1);
		mask |= (mask >> 8) | (mask << 8);
		mask &= ~(side ? fileA : fileH);
		mask.clear(square);
		return mask;
	}
	constexpr Bitboard genKnightAttacks(int square) const noexcept {
		const int side = (square & 0x7) >> 2;
		const Bitboard mask = Bitboard::mask(square);
		return ((mask >> 6) | (mask >> 10) | (mask >> 15) | (mask >> 17) |
				(mask << 6) | (mask << 10) | (mask << 15) | (mask << 17)) &
			   ~(side ? (fileA | fileB) : (fileH | fileG));
	}

	inline static constexpr Bitboard fileA{0x0101010101010101ULL};
	inline static constexpr Bitboard fileB{0x0202020202020202ULL};
	inline static constexpr Bitboard fileG{0x4040404040404040ULL};
	inline static constexpr Bitboard fileH{0x8080808080808080ULL};
	std::array<Bitboard, 64> wpAttacks{};
	std::array<Bitboard, 64> bpAttacks{};
	std::array<Bitboard, 64> nAttacks{};
	std::array<Bitboard, 64> kAttacks{};
	std::array<std::array<Bitboard, 256>, 64> rankAttacks{};
	std::array<std::array<Bitboard, 256>, 64> fileAttacks{};
	std::array<std::array<Bitboard, 256>, 64> diag1Attacks{};
	std::array<std::array<Bitboard, 256>, 64> diag2Attacks{};
};

constexpr MoveData::MoveData() noexcept {
	const auto lineAttacks = makeLineAttacks();
	const auto fileMasks = makeSpreadMasks(8, 0);
	const auto diag1Masks = makeSpreadMasks(9, 0);
	const auto diag2Masks = makeSpreadMasks(7, 7);
	const auto *files = fileMasks.data();
	const auto *diagonals1 = diag1Masks.data();
	const auto *diagonals2 = diag2Masks.data();

	for (int from = 0; from < 64; ++from) {
		wpAttacks[from] = genWhitePawnAttacks(from);
		bpAttacks[from] = genBlackPawnAttacks(from);
		nAttacks[from] = genKnightAttacks(from);
		kAttacks[from] = genKingAttacks(from);

		const int rank = from / 8;
		const int file = from % 8;
		const int diag1Offset = file - rank;
		const int diag2Offset = file + rank - 7;
		const unsigned diag1Ranks =
			rankMask(diag1Offset < 0 ? -diag1Offset : 0,
					 diag1Offset > 0 ? 7 - diag1Offset : 7);
		const unsigned diag2Ranks =
			rankMask(diag2Offset > 0 ? diag2Offset : 0,
					 diag2Offset < 0 ? 7 + diag2Offset : 7);
		const auto *rankLine = lineAttacks[file].data();
		const auto *verticalLine = lineAttacks[rank].data();
		auto *ranks = rankAttacks[from].data();
		auto *fileTable = fileAttacks[from].data();
		auto *diag1Table = diag1Attacks[from].data();
		auto *diag2Table = diag2Attacks[from].data();

		for (int index = 0; index < 256; ++index) {
			ranks[index] =
				Bitboard{std::uint64_t{rankLine[index]} << (8 * rank)};
			const unsigned moves = verticalLine[index];
			fileTable[index] = Bitboard{files[moves] << file};
			// Clip the rank byte before spreading it so wrapped diagonal
			// bits never cross a board edge.
			const auto diag1 = diagonals1[moves & diag1Ranks];
			const auto diag2 = diagonals2[moves & diag2Ranks];
			diag1Table[index] =
				Bitboard{diag1Offset >= 0 ? diag1 << diag1Offset
										  : diag1 >> -diag1Offset};
			diag2Table[index] =
				Bitboard{diag2Offset >= 0 ? diag2 << diag2Offset
										  : diag2 >> -diag2Offset};
		}
	}
}

// One shared entity across translation units; no automatic table copies.
inline constexpr MoveData moveData{};

constexpr Bitboard whitePawnAttacks(int square) noexcept {
	return moveData.whitePawnAttacks(square);
}
constexpr Bitboard blackPawnAttacks(int square) noexcept {
	return moveData.blackPawnAttacks(square);
}
constexpr Bitboard knightAttacks(int square) noexcept {
	return moveData.knightAttacks(square);
}
constexpr Bitboard kingAttacks(int square) noexcept {
	return moveData.kingAttacks(square);
}

// Retained for callers that explicitly initialized the old runtime tables.
constexpr void initMoveData(void) noexcept {}

} // namespace morphy_chess
