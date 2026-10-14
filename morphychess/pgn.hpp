#pragma once

#include <stddef.h>
#include <string>
#include <string_view>
#include <vector>

#include <morphychess/board.hpp>

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

namespace morphy_chess {

// Stateful PGN main-line reader; annotations and variations are skipped.
// next() distinguishes ordinary EOF from malformed input through hasError().
// SAN, tags, and FEN follow the formats described in bibliography.md [pgn],
// with permissive import handling (for example, an omitted final result).
class Pgn {
public:
	bool load(const std::string &filename, Board &board);
	bool loadText(std::string text, Board &board);
	bool next(Board &board);

	const std::string &event() const noexcept { return eventValue; }
	const std::string &site() const noexcept { return siteValue; }
	const std::string &whiteName() const noexcept { return whiteNameValue; }
	const std::string &blackName() const noexcept { return blackNameValue; }
	const std::string &eco() const noexcept { return ecoValue; }
	int whiteElo() const noexcept { return whiteEloValue; }
	int blackElo() const noexcept { return blackEloValue; }
	int year() const noexcept { return yearValue; }
	int month() const noexcept { return monthValue; }
	int day() const noexcept { return dayValue; }
	int round() const noexcept { return roundValue; }
	Result result() const noexcept { return resultValue; }
	const std::vector<Move> &moves() const noexcept { return movesValue; }
	size_t moveCount() const noexcept { return movesValue.size(); }
	bool hasError() const noexcept { return errorValue; }

private:
	void resetMetadata();
	bool skipIgnored();
	bool readTag();
	void processTag(std::string_view name, const std::string &value);

	std::string stream;
	size_t cursor = 0;
	std::string eventValue;
	std::string siteValue;
	std::string whiteNameValue;
	std::string blackNameValue;
	std::string ecoValue;
	std::string fenValue;
	int whiteEloValue = -1;
	int blackEloValue = -1;
	int yearValue = -1;
	int monthValue = -1;
	int dayValue = -1;
	int roundValue = -1;
	Result resultValue = invalid;
	std::vector<Move> movesValue;
	bool errorValue = false;
};

} // namespace morphy_chess
