#pragma once

#include <morphychess/board.hpp>
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

inline constexpr auto kRemoveRepeatKey = ~(0x0000000000000003LL);

U64 zobristKey(int, int);
U64 zobristRepeatKey(int);
U64 zobristKingSideCastleKey(int);
U64 zobristQueenSideCastleKey(int);
U64 zobristEpKey(int);
U64 computePositionKey(const Board *);
U64 computePawnKey(const Board *);
void initZobristKeys(void);

} // namespace morphy_chess
