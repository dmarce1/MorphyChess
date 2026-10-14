#include <cctype>
#include <charconv>
#include <fstream>
#include <iterator>
#include <utility>

#include <morphychess/pgn.hpp>

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
namespace {

bool isSpace(char value) {
	return std::isspace(static_cast<unsigned char>(value)) != 0;
}

int parseNumber(std::string_view text) {
	int value = -1;
	const auto parsed =
		std::from_chars(text.data(), text.data() + text.size(), value);
	return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()
			   ? value
			   : -1;
}

Result parseResult(std::string_view token) {
	if (token == "1-0") {
		return whiteWin;
	}
	if (token == "0-1") {
		return blackWin;
	}
	if (token == "1/2-1/2") {
		return draw;
	}
	if (token == "*") {
		return inProgress;
	}
	return invalid;
}

} // namespace

bool Pgn::load(const std::string &filename, Board &board) {
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		stream.clear();
		cursor = 0;
		resetMetadata();
		errorValue = true;
		return false;
	}
	return loadText(std::string(std::istreambuf_iterator<char>(input), {}),
					board);
}

bool Pgn::loadText(std::string text, Board &board) {
	stream = std::move(text);
	cursor = 0;
	errorValue = false;
	return next(board);
}

void Pgn::resetMetadata() {
	eventValue.clear();
	siteValue.clear();
	whiteNameValue.clear();
	blackNameValue.clear();
	ecoValue.clear();
	fenValue.clear();
	whiteEloValue = blackEloValue = yearValue = monthValue = dayValue =
		roundValue = -1;
	resultValue = invalid;
	movesValue.clear();
}

bool Pgn::skipIgnored() {
	// Only the main line is replayed. Count nested variations so their moves,
	// result markers, and annotations never affect the caller's position.
	int variationDepth = 0;
	while (cursor < stream.size()) {
		const char value = stream[cursor];
		if (isSpace(value)) {
			++cursor;
		} else if (value == '{') {
			const auto end = stream.find('}', cursor + 1);
			if (end == std::string::npos) {
				errorValue = true;
				return false;
			}
			cursor = end + 1;
		} else if (value == ';' || value == '%') {
			const auto end = stream.find('\n', cursor + 1);
			cursor = end == std::string::npos ? stream.size() : end + 1;
		} else if (value == '(') {
			++variationDepth;
			++cursor;
		} else if (value == ')') {
			if (variationDepth == 0) {
				errorValue = true;
				return false;
			}
			--variationDepth;
			++cursor;
		} else if (variationDepth != 0) {
			++cursor;
		} else {
			break;
		}
	}
	if (variationDepth != 0) {
		errorValue = true;
		return false;
	}
	return true;
}

bool Pgn::readTag() {
	// Consume quoted values before looking for ']' so escaped quotes and
	// bracket characters inside metadata cannot terminate a tag early.
	++cursor;
	while (cursor < stream.size() && isSpace(stream[cursor])) {
		++cursor;
	}
	const auto start = cursor;
	while (cursor < stream.size() && !isSpace(stream[cursor]) &&
		   stream[cursor] != '"') {
		++cursor;
	}
	const std::string name = stream.substr(start, cursor - start);
	while (cursor < stream.size() && isSpace(stream[cursor])) {
		++cursor;
	}
	if (name.empty() || cursor == stream.size() || stream[cursor++] != '"') {
		return false;
	}
	std::string value;
	bool closed = false;
	while (cursor < stream.size()) {
		const char next = stream[cursor++];
		if (next == '"') {
			closed = true;
			break;
		}
		if (next == '\\') {
			if (cursor == stream.size()) {
				return false;
			}
			value += stream[cursor++];
		} else {
			value += next;
		}
	}
	while (cursor < stream.size() && isSpace(stream[cursor])) {
		++cursor;
	}
	if (!closed || cursor == stream.size() || stream[cursor++] != ']') {
		return false;
	}
	processTag(name, value);
	return true;
}

void Pgn::processTag(std::string_view name, const std::string &value) {
	if (name == "Event") {
		eventValue = value;
	} else if (name == "Site") {
		siteValue = value;
	} else if (name == "White") {
		whiteNameValue = value;
	} else if (name == "Black") {
		blackNameValue = value;
	} else if (name == "ECO") {
		ecoValue = value.substr(0, 3);
	} else if (name == "FEN") {
		fenValue = value;
	} else if (name == "WhiteElo") {
		whiteEloValue = parseNumber(value);
	} else if (name == "BlackElo") {
		blackEloValue = parseNumber(value);
	} else if (name == "Round") {
		roundValue = parseNumber(value);
	} else if (name == "Result") {
		resultValue = parseResult(value);
	} else if (name == "Date" && value.size() == 10 && value[4] == '.' &&
			   value[7] == '.') {
		yearValue = parseNumber(std::string_view(value).substr(0, 4));
		monthValue = parseNumber(std::string_view(value).substr(5, 2));
		dayValue = parseNumber(std::string_view(value).substr(8, 2));
	}
}

bool Pgn::next(Board &board) {
	if (errorValue || !skipIgnored() || cursor == stream.size()) {
		return false;
	}
	resetMetadata();
	bool sawTags = false;
	while (cursor < stream.size() && stream[cursor] == '[') {
		sawTags = true;
		if (!readTag() || !skipIgnored()) {
			errorValue = true;
			return false;
		}
	}
	// Parse transactionally: a malformed later move must leave the caller's
	// board intact. The parser's own metadata may describe the failed game.
	Board parsedBoard;
	if (!fenValue.empty() && !parsedBoard.loadFen(fenValue.c_str())) {
		errorValue = true;
		return false;
	}
	while (cursor < stream.size()) {
		if (!skipIgnored()) {
			return false;
		}
		if (cursor == stream.size() || stream[cursor] == '[') {
			break;
		}
		// NAGs terminate at the first non-digit, even without whitespace:
		// e4$1e5 is two moves and one annotation. See bibliography.md [pgn].
		if (stream[cursor] == '$') {
			const auto digits = ++cursor;
			while (cursor < stream.size() && stream[cursor] >= '0' &&
				   stream[cursor] <= '9') {
				++cursor;
			}
			const int nag = parseNumber(
				std::string_view(stream).substr(digits, cursor - digits));
			if (nag < 0 || nag > 255) {
				errorValue = true;
				return false;
			}
			continue;
		}
		// The unfinished-game marker is also a standalone PGN token.
		if (stream[cursor] == '*') {
			++cursor;
			resultValue = inProgress;
			board = parsedBoard;
			return true;
		}
		const auto start = cursor;
		while (cursor < stream.size() && !isSpace(stream[cursor]) &&
			   stream[cursor] != '{' && stream[cursor] != ';' &&
			   stream[cursor] != '(' && stream[cursor] != ')' &&
			   stream[cursor] != '[' && stream[cursor] != '$' &&
			   stream[cursor] != '*') {
			++cursor;
		}
		std::string token = stream.substr(start, cursor - start);
		if (token.empty()) {
			errorValue = true;
			return false;
		}
		if (const Result result = parseResult(token); result != invalid) {
			resultValue = result;
			board = parsedBoard;
			return true;
		}
		if (token == "e.p.") {
			continue;
		}
		const auto period = token.find_last_of('.');
		if (period != std::string::npos) {
			token.erase(0, period + 1);
		}
		while (!token.empty() && (token.back() == '!' || token.back() == '?')) {
			token.pop_back();
		}
		if (token.empty()) {
			continue;
		}
		if (token.size() > 16 || movesValue.size() >= maxHalfMoves) {
			errorValue = true;
			return false;
		}
		const Move move = parsedBoard.makeTextMove(token.c_str());
		if (move.isEmpty()) {
			errorValue = true;
			return false;
		}
		movesValue.push_back(move);
	}
	if (!sawTags && movesValue.empty()) {
		return false;
	}
	board = parsedBoard;
	return true;
}

} // namespace morphy_chess
