#include <morphychess/book.hpp>
#include <morphychess/pgn.hpp>
#include <morphychess/rating.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <utility>

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

// The original x86 book.bin contains 4096 fixed 32-byte records followed by
// a 32-bit entry count and four ECO bytes. Encode fields explicitly so class
// layout, padding, and native endianness do not become part of the file format.
constexpr size_t entryBytes = 32;
constexpr size_t bookBytes = kBookSize * entryBytes + 8;
using BookBytes = std::vector<unsigned char>;

std::uint64_t readInteger(const unsigned char *data, size_t count) {
	std::uint64_t value = 0;
	for (size_t byte = 0; byte < count; ++byte) {
		value |= std::uint64_t(data[byte]) << (8 * byte);
	}
	return value;
}

int readSigned(const unsigned char *data) {
	return std::bit_cast<std::int32_t>(
		static_cast<std::uint32_t>(readInteger(data, 4)));
}

void writeInteger(unsigned char *data, std::uint64_t value, size_t count) {
	for (size_t byte = 0; byte < count; ++byte) {
		data[byte] = static_cast<unsigned char>(value >> (8 * byte));
	}
}

int incrementCount(int value) {
	return value < std::numeric_limits<int>::max() ? value + 1 : value;
}

} // namespace

Book::~Book() {
	if (dirty && !filenameValue.empty()) {
		try {
			close();
		} catch (...) { /* Destruction cannot report an I/O failure. */
		}
	}
}

bool Book::entryLess(const BookEntry &left, const BookEntry &right) {
	if (left.colorValue != right.colorValue) {
		return left.colorValue < right.colorValue;
	}
	return left.keyValue < right.keyValue;
}

size_t Book::search(const Board &board) const {
	BookEntry target;
	target.colorValue = board.sideToMove();
	target.keyValue = board.positionKey();
	const auto found =
		std::lower_bound(entries.begin(), entries.end(), target, entryLess);
	if (found == entries.end() || found->keyValue != target.keyValue ||
		found->colorValue != target.colorValue) {
		return entries.size();
	}
	return static_cast<size_t>(found - entries.begin());
}

const BookEntry *Book::findEntry(const Board &board) const {
	const size_t index = search(board);
	return index == entries.size() ? nullptr : &entries[index];
}

bool Book::addEntry(const Board &board, const std::string &eco) {
	const auto found = std::find_if(
		entries.begin(), entries.end(), [&](const BookEntry &entry) {
			return entry.keyValue == board.positionKey() &&
				   entry.colorValue == board.sideToMove();
		});
	if (found != entries.end()) {
		if (found->ratingValue < kInitialBookRating + 300) {
			found->winsValue = incrementCount(found->winsValue);
		}
		return true;
	}
	if (entries.size() == kBookSize) {
		return false;
	}
	BookEntry entry;
	entry.keyValue = board.positionKey();
	entry.colorValue = board.sideToMove();
	entry.winsValue = 1;
	std::copy_n(eco.begin(), std::min<size_t>(3, eco.size()),
				entry.ecoValue.begin());
	entries.push_back(entry);
	return true;
}

int Book::digest(const std::string &filename) {
	Pgn pgn;
	Board board;
	if (!pgn.load(filename, board)) {
		return -1;
	}
	std::vector<BookEntry> previousEntries = std::move(entries);
	entries.clear();
	int openingCount = 0;
	do {
		while (board.halfMoveCount() > 0) {
			if (!addEntry(board, pgn.eco())) {
				entries = std::move(previousEntries);
				return -1;
			}
			board.undoMove();
		}
		++openingCount;
	} while (pgn.next(board));
	if (pgn.hasError()) {
		entries = std::move(previousEntries);
		return -1;
	}
	std::sort(entries.begin(), entries.end(), entryLess);
	lastEcoValue.clear();
	dirty = true;
	return openingCount;
}

bool Book::open(const std::string &filename) {
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		const auto pgnPath =
			std::filesystem::path(filename).parent_path() / "eco.pgn";
		Book replacement;
		if (replacement.digest(pgnPath.string()) < 0 ||
			!replacement.save(filename)) {
			return false;
		}
		entries = std::move(replacement.entries);
		filenameValue = std::move(replacement.filenameValue);
		lastEcoValue = std::move(replacement.lastEcoValue);
		dirty = false;
		return true;
	}
	BookBytes bytes(bookBytes, 0);
	input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
	if (!input || input.peek() != std::char_traits<char>::eof()) {
		return false;
	}
	const auto count = readInteger(bytes.data() + kBookSize * entryBytes, 4);
	if (count > kBookSize) {
		return false;
	}
	std::vector<BookEntry> loaded;
	loaded.reserve(count);
	for (size_t index = 0; index < count; ++index) {
		const auto *data = bytes.data() + entryBytes * index;
		BookEntry entry;
		entry.keyValue = readInteger(data, 8);
		entry.winsValue = readSigned(data + 8);
		entry.lossesValue = readSigned(data + 12);
		entry.drawsValue = readSigned(data + 16);
		entry.colorValue = readSigned(data + 20);
		std::copy_n(data + 24, 3, entry.ecoValue.begin());
		entry.ratingValue = readSigned(data + 28);
		if ((entry.colorValue != white && entry.colorValue != black) ||
			entry.winsValue < 0 || entry.lossesValue < 0 ||
			entry.drawsValue < 0) {
			return false;
		}
		loaded.push_back(entry);
	}
	std::sort(loaded.begin(), loaded.end(), entryLess);
	entries = std::move(loaded);
	filenameValue = filename;
	lastEcoValue.assign(reinterpret_cast<const char *>(
							bytes.data() + kBookSize * entryBytes + 4),
						3);
	lastEcoValue.resize(lastEcoValue.find('\0') == std::string::npos
							? 3
							: lastEcoValue.find('\0'));
	dirty = false;
	return true;
}

bool Book::save(const std::string &filename) {
	BookBytes bytes(bookBytes, 0);
	for (size_t index = 0; index < entries.size(); ++index) {
		const auto &entry = entries[index];
		auto *data = bytes.data() + entryBytes * index;
		writeInteger(data, entry.keyValue, 8);
		writeInteger(data + 8, static_cast<std::uint32_t>(entry.winsValue), 4);
		writeInteger(data + 12, static_cast<std::uint32_t>(entry.lossesValue),
					 4);
		writeInteger(data + 16, static_cast<std::uint32_t>(entry.drawsValue),
					 4);
		writeInteger(data + 20, static_cast<std::uint32_t>(entry.colorValue),
					 4);
		std::copy(entry.ecoValue.begin(), entry.ecoValue.end(), data + 24);
		writeInteger(data + 28, static_cast<std::uint32_t>(entry.ratingValue),
					 4);
	}
	writeInteger(bytes.data() + kBookSize * entryBytes, entries.size(), 4);
	std::copy_n(lastEcoValue.begin(), std::min<size_t>(3, lastEcoValue.size()),
				bytes.begin() + kBookSize * entryBytes + 4);
	std::ofstream output(filename, std::ios::binary | std::ios::trunc);
	if (!output) {
		return false;
	}
	output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
	output.close();
	if (!output) {
		return false;
	}
	filenameValue = filename;
	dirty = false;
	return true;
}

bool Book::close() {
	return filenameValue.empty() ? !dirty : save(filenameValue);
}

Move Book::findMove(Board &board) {
	if (board.halfMoveCount() != 0 && findEntry(board) == nullptr) {
		return {};
	}
	std::array<Move, maxMoves> moves;
	std::vector<Move> choices;
	std::vector<size_t> indices;
	std::vector<double> weights;
	const int count = board.genPseudolegalMoves(moves.data());
	for (int index = 0; index < count; ++index) {
		if (!board.makeMove(moves[index])) {
			continue;
		}
		const size_t bookIndex = search(board);
		board.undoMove();
		if (bookIndex < entries.size()) {
			choices.push_back(moves[index]);
			indices.push_back(bookIndex);
			weights.push_back(entries[bookIndex].winsValue);
		}
	}
	if (choices.empty()) {
		return {};
	}
	if (std::accumulate(weights.begin(), weights.end(), 0.0) == 0.0) {
		std::fill(weights.begin(), weights.end(), 1.0);
	}
	std::discrete_distribution<size_t> distribution(weights.begin(),
													weights.end());
	const size_t choice = distribution(randomGenerator);
	lastEcoValue = entries[indices[choice]].eco();
	return choices[choice];
}

void Book::learning(const Board &finalBoard, int computerColor, Result result,
					int opponentRating) {
	if (result != whiteWin && result != blackWin && result != draw) {
		return;
	}
	Board board = finalBoard;
	while (board.halfMoveCount() > 0) {
		board.undoMove();
		const size_t index = search(board);
		if (index == entries.size() || board.sideToMove() == computerColor) {
			continue;
		}
		auto &entry = entries[index];
		const bool won = (computerColor == white && result == whiteWin) ||
						 (computerColor == black && result == blackWin);
		const int score = result == draw ? 0 : (won ? 1 : -1);
		const long long gameCount =
			1LL + entry.winsValue + entry.lossesValue + entry.drawsValue;
		if (score > 0) {
			entry.winsValue = incrementCount(entry.winsValue);
		} else if (score < 0) {
			entry.lossesValue = incrementCount(entry.lossesValue);
		} else {
			entry.drawsValue = incrementCount(entry.drawsValue);
		}
		const int ratingGames =
			computerColor == white
				? 100
				: static_cast<int>(std::min<long long>(
					  gameCount, std::numeric_limits<int>::max()));
		entry.ratingValue = ratingComputeNewRating(
			entry.ratingValue, opponentRating, score, ratingGames);
		dirty = true;
	}
	if (dirty && !filenameValue.empty()) {
		close();
	}
}

void Book::showBookMoves(Board &board) const {
	struct Choice {
		Move move;
		long long games;
	};
	std::array<Move, maxMoves> moves;
	std::vector<Choice> choices;
	const int count = board.genPseudolegalMoves(moves.data());
	for (int index = 0; index < count; ++index) {
		if (!board.makeMove(moves[index])) {
			continue;
		}
		const auto *entry = findEntry(board);
		board.undoMove();
		if (entry != nullptr) {
			choices.push_back(
				{moves[index],
				 0LL + entry->wins() + entry->losses() + entry->draws()});
		}
	}
	if (choices.empty()) {
		std::printf("\tNo book moves found\n\tLast book entry was ECO %s\n\n",
					lastEcoValue.c_str());
		return;
	}
	std::sort(choices.begin(), choices.end(),
			  [](const Choice &left, const Choice &right) {
				  return left.games > right.games;
			  });
	const double total = std::accumulate(
		choices.begin(), choices.end(), 0.0,
		[](double sum, const Choice &choice) { return sum + choice.games; });
	std::printf("\tBook moves\n");
	for (size_t index = 0; index < choices.size(); ++index) {
		char text[256]{};
		board.moveToStr(text, choices[index].move);
		const double chance =
			total > 0 ? choices[index].games / total : 1.0 / choices.size();
		std::printf("\t%2zu. %10s %3i%%\n", index + 1, text,
					static_cast<int>(100 * chance + 0.5));
	}
	std::printf("\n");
}

} // namespace morphy_chess
