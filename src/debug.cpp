#include <array>
#include <cstdio>
#include <cstdlib>

#include <morphychess/bits.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/zobrist.hpp>

namespace morphy_chess {
namespace {
thread_local const Board *registeredBoard = nullptr;
}
void debugRegisterBoard(const Board *board) { registeredBoard = board; }
void assertCondition([[maybe_unused]] bool condition,
					 [[maybe_unused]] const std::source_location &location) {
#ifndef NDEBUG
	if (!condition) {
		std::fprintf(stderr, "Assertion failed at %s:%u in %s\n",
					 location.file_name(), location.line(),
					 location.function_name());
		if (registeredBoard) {
			registeredBoard->toStream(stderr);
		}
		std::abort();
	}
#endif
}
void Board::assertTranspositionKeyConsistency(
	[[maybe_unused]] const std::source_location &location) const {
#ifndef NDEBUG
	assertCondition(computePositionKey() == positionKey(), location);
	assertCondition(computePawnKey() == pawnKey(), location);
#endif
}
void Board::assertBoardConsistency(
	[[maybe_unused]] const std::source_location &location) const {
#ifndef NDEBUG
	// Reconstruct redundant caches from the square array rather than checking
	// them against each other, then independently recompute the hash keys.
	std::array<std::array<int, 7>, 2> counts{};
	std::array<std::array<Bitboard, 7>, 2> pieces{};
	RotatedBitboard occupied;
	for (int square = 0; square < 64; ++square) {
		const int value = pieceValue(square);
		if (value == empty) {
			continue;
		}
		const int color = value > 0 ? white : black;
		const int piece = std::abs(value);
		assertCondition(piece >= pawn && piece <= king, location);
		++counts[color][piece];
		++counts[color][0];
		pieces[color][piece].set(square);
		pieces[color][0].set(square);
		occupied.set(square);
	}
	for (int color = black; color <= white; ++color) {
		for (int piece = empty; piece <= king; ++piece) {
			assertCondition(counts[color][piece] == pieceCount(color, piece),
							location);
			assertCondition(pieces[color][piece] == pieceBoard(color, piece),
							location);
		}
	}
	for (int index = 0; index < 4; ++index) {
		assertCondition(occupied[index] == occupiedSquares()[index], location);
	}
	assertCondition(fiftyMoveRule() >= 0, location);
	assertTranspositionKeyConsistency(location);
#endif
}
} // namespace morphy_chess
