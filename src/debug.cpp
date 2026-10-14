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
void assertTranspositionKeyConsistency(
	[[maybe_unused]] const Board *board,
	[[maybe_unused]] const std::source_location &location) {
#ifndef NDEBUG
	assertCondition(board != nullptr, location);
	assertCondition(computePositionKey(board) == board->positionKey(),
					location);
	assertCondition(computePawnKey(board) == board->pawnKey(), location);
#endif
}
void assertBoardConsistency(
	[[maybe_unused]] const Board *board,
	[[maybe_unused]] const std::source_location &location) {
#ifndef NDEBUG
	assertCondition(board != nullptr, location);
	int counts[2][7]{};
	Bitboard pieces[2][7]{};
	Bitboard occupied[4]{};
	for (int square = 0; square < 64; ++square) {
		const int value = board->pieceValue(square);
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
		rotatedBitsSet(occupied, square);
	}
	for (int color = black; color <= white; ++color) {
		for (int piece = empty; piece <= king; ++piece) {
			assertCondition(counts[color][piece] ==
								board->pieceCount(color, piece),
							location);
			assertCondition(pieces[color][piece] ==
								board->pieceBoard(color, piece),
							location);
		}
	}
	for (int index = 0; index < 4; ++index) {
		assertCondition(occupied[index] == board->occupiedSquares()[index],
						location);
	}
	assertCondition(board->fiftyMoveRule() >= 0, location);
	assertTranspositionKeyConsistency(board, location);
#endif
}
} // namespace morphy_chess
