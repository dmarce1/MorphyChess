#pragma once

#include <array>
#include <morphychess/bitboard.hpp>
#include <morphychess/params.hpp>

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

void initBits(void);

class RotatedBitboard {
public:
	RotatedBitboard();
	explicit RotatedBitboard(Bitboard value);
	void assign(Bitboard value);
	void set(int square);
	void clear(int square);
	void reset();
	Bitboard operator[](int index) const noexcept { return boards[index]; }
	const std::array<Bitboard, 4> &rotations() const noexcept { return boards; }
	int indexFromRank(int square) const;
	int indexFromFile(int square) const;
	int indexFromDiag1(int square) const;
	int indexFromDiag2(int square) const;
	Bitboard rankMoves(int square) const;
	Bitboard fileMoves(int square) const;
	Bitboard diag1Moves(int square) const;
	Bitboard diag2Moves(int square) const;
	Bitboard bishopAttacks(int square) const;
	Bitboard rookAttacks(int square) const;
	Bitboard queenAttacks(int square) const;

private:
	std::array<Bitboard, 4> boards{};
};

} // namespace morphy_chess
