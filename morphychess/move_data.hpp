#pragma once

#include <array>
#include <morphychess/bits.hpp>
#include <morphychess/integers.hpp>
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

Bitboard whitePawnAttacks(int);
Bitboard blackPawnAttacks(int);
Bitboard knightAttacks(int);
Bitboard bishopAttacks(const std::array<Bitboard, 4> &, int);
Bitboard rookAttacks(const std::array<Bitboard, 4> &, int);
Bitboard queenAttacks(const std::array<Bitboard, 4> &, int);
Bitboard rankMoves(const std::array<Bitboard, 4> &, int);
Bitboard fileMoves(const std::array<Bitboard, 4> &, int);
Bitboard diag1Moves(const std::array<Bitboard, 4> &, int);
Bitboard diag2Moves(const std::array<Bitboard, 4> &, int);
Bitboard kingAttacks(int);
void initMoveData(void);

} // namespace morphy_chess
