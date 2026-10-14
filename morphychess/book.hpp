#pragma once

#include <array>
#include <random>
#include <stddef.h>
#include <string>
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

// One reached position, keyed by hash and side to move. Outcome counters
// describe the player who made the preceding move; ECO is null terminated.
class BookEntry {
public:
	std::uint64_t key() const noexcept { return keyValue; }
	int wins() const noexcept { return winsValue; }
	int losses() const noexcept { return lossesValue; }
	int draws() const noexcept { return drawsValue; }
	int color() const noexcept { return colorValue; }
	int rating() const noexcept { return ratingValue; }
	char const *eco() const noexcept { return ecoValue.data(); }

private:
	friend class Book;
	std::uint64_t keyValue = 0;
	int winsValue = 0;
	int lossesValue = 0;
	int drawsValue = 0;
	int colorValue = black;
	std::array<char, 4> ecoValue{};
	int ratingValue = kInitialBookRating;
};

// Sorted in-memory opening positions with an explicit legacy binary codec.
// Public operations are used by the stopped-search controller; this class
// does not synchronize concurrent mutation. An empty Move means no book hit.
class Book {
public:
	Book() = default;
	~Book();
	Book(Book const &) = delete;
	Book &operator=(Book const &) = delete;

	bool open(std::string const &filename = kBookName);
	bool close();
	// A failed save preserves the destination and the loaded filename/dirty
	// state. A preexisting sibling .tmp file or symlink destination is refused.
	bool save(std::string const &filename);
	int digest(std::string const &filename);
	Move findMove(Board &board);
	void learning(Board const &board, int computerColor, Result result,
				  int opponentRating);
	void showBookMoves(Board &board) const;
	BookEntry const *findEntry(Board const &board) const;
	size_t entryCount() const noexcept { return entries.size(); }
	std::string const &lastEco() const noexcept { return lastEcoValue; }

private:
	bool addEntry(Board const &board, std::string const &eco);
	size_t search(Board const &board) const;
	static bool entryLess(BookEntry const &left, BookEntry const &right);

	std::vector<BookEntry> entries;
	std::string filenameValue;
	std::string lastEcoValue;
	std::mt19937 randomGenerator{std::random_device{}()};
	bool dirty = false;
};

} // namespace morphy_chess
