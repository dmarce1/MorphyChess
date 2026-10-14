#pragma once

#include <array>
#include <morphychess/board.hpp>
#include <random>
#include <stddef.h>
#include <string>
#include <vector>

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

class BookEntry {
public:
	U64 key() const noexcept { return keyValue; }
	int wins() const noexcept { return winsValue; }
	int losses() const noexcept { return lossesValue; }
	int draws() const noexcept { return drawsValue; }
	int color() const noexcept { return colorValue; }
	int rating() const noexcept { return ratingValue; }
	const char *eco() const noexcept { return ecoValue.data(); }

private:
	friend class Book;
	U64 keyValue = 0;
	int winsValue = 0;
	int lossesValue = 0;
	int drawsValue = 0;
	int colorValue = black;
	std::array<char, 4> ecoValue{};
	int ratingValue = kInitialBookRating;
};

class Book {
public:
	Book() = default;
	~Book();
	Book(const Book &) = delete;
	Book &operator=(const Book &) = delete;

	bool open(const std::string &filename = kBookName);
	bool close();
	bool save(const std::string &filename);
	int digest(const std::string &filename);
	Move findMove(Board &board);
	void learning(const Board &board, int computerColor, Result result,
				  int opponentRating);
	void showBookMoves(Board &board) const;
	const BookEntry *findEntry(const Board &board) const;
	size_t entryCount() const noexcept { return entries.size(); }
	const std::string &lastEco() const noexcept { return lastEcoValue; }

private:
	bool addEntry(const Board &board, const std::string &eco);
	size_t search(const Board &board) const;
	static bool entryLess(const BookEntry &left, const BookEntry &right);

	std::vector<BookEntry> entries;
	std::string filenameValue;
	std::string lastEcoValue;
	std::mt19937 randomGenerator{std::random_device{}()};
	bool dirty = false;
};

} // namespace morphy_chess
