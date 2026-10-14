#include <array>
#include <cstddef>

#include <gtest/gtest.h>

#include <morphychess/bits.hpp>
#include <morphychess/move_data.hpp>

namespace morphy_chess {
namespace {
static_assert(moveData.knightAttacks(0) == Bitboard{0x20400});
static_assert(whitePawnAttacks(8) == Bitboard::mask(17));
static_assert(blackPawnAttacks(55) == Bitboard::mask(46));
static_assert(kingAttacks(27).countOnes() == 8);
static_assert(moveData.rankMoves(27, 0).countOnes() == 7);
static_assert(moveData.fileMoves(27, 0).countOnes() == 7);
static_assert(moveData.diag1Moves(27, 0).countOnes() == 7);
static_assert(moveData.diag2Moves(27, 0).countOnes() == 6);
static_assert(moveData.fileMoves(27, 0x20).test(43));
static_assert(!moveData.fileMoves(27, 0x20).test(51));

constexpr bool supportsRotatedConstantEvaluation() {
	RotatedBitboard board(Bitboard::mask(0) | Bitboard::mask(63));
	if (board[kRankIndex].countOnes() != 2 ||
		board[kFileIndex].countOnes() != 2 ||
		board[kDiag1Index].countOnes() != 2 ||
		board[kDiag2Index].countOnes() != 2) {
		return false;
	}
	board.clear(63);
	board.set(43);
	if (!board.rookAttacks(27).test(43) || board.rookAttacks(27).test(51)) {
		return false;
	}
	board.assign(Bitboard{});
	if (board.rookAttacks(27).countOnes() != 14 ||
		board.bishopAttacks(27).countOnes() != 13 ||
		board.queenAttacks(27).countOnes() != 27) {
		return false;
	}
	board.set(0);
	board.reset();
	return !board[kRankIndex] && !board[kFileIndex] && !board[kDiag1Index] &&
		   !board[kDiag2Index];
}
static_assert(supportsRotatedConstantEvaluation());

struct Direction {
	int rank;
	int file;
};

constexpr std::array<Direction, 2> whitePawnDirections{{{1, -1}, {1, 1}}};
constexpr std::array<Direction, 2> blackPawnDirections{{{-1, -1}, {-1, 1}}};
constexpr std::array<Direction, 8> knightDirections{
	{{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}}};
constexpr std::array<Direction, 8> kingDirections{
	{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}}};

bool isOnBoard(int rank, int file) {
	return rank >= 0 && rank < 8 && file >= 0 && file < 8;
}

template <size_t count>
Bitboard expectedLeaperAttacks(int square,
							   std::array<Direction, count> const &directions) {
	Bitboard attacks;
	for (Direction const direction : directions) {
		int const rank = square / 8 + direction.rank;
		int const file = square % 8 + direction.file;
		if (isOnBoard(rank, file)) {
			attacks.set(8 * rank + file);
		}
	}
	return attacks;
}

Bitboard expectedRay(int square, Bitboard occupancy, Direction direction) {
	Bitboard attacks;
	int rank = square / 8 + direction.rank;
	int file = square % 8 + direction.file;
	while (isOnBoard(rank, file)) {
		int const target = 8 * rank + file;
		attacks.set(target);
		if (occupancy.test(target)) {
			break;
		}
		rank += direction.rank;
		file += direction.file;
	}
	return attacks;
}

void expectSlidingAttacks(int square, Bitboard occupancy) {
	RotatedBitboard const rotated(occupancy);
	Bitboard const rank = expectedRay(square, occupancy, {0, -1}) |
						  expectedRay(square, occupancy, {0, 1});
	Bitboard const file = expectedRay(square, occupancy, {-1, 0}) |
						  expectedRay(square, occupancy, {1, 0});
	Bitboard const diag1 = expectedRay(square, occupancy, {-1, -1}) |
						   expectedRay(square, occupancy, {1, 1});
	Bitboard const diag2 = expectedRay(square, occupancy, {-1, 1}) |
						   expectedRay(square, occupancy, {1, -1});
	EXPECT_EQ(rotated.rankMoves(square), rank);
	EXPECT_EQ(rotated.fileMoves(square), file);
	EXPECT_EQ(rotated.diag1Moves(square), diag1);
	EXPECT_EQ(rotated.diag2Moves(square), diag2);
	EXPECT_EQ(rotated.rookAttacks(square), rank | file);
	EXPECT_EQ(rotated.bishopAttacks(square), diag1 | diag2);
	EXPECT_EQ(rotated.queenAttacks(square), rank | file | diag1 | diag2);
}
} // namespace

TEST(MoveData, LeapersMatchCoordinatesWithoutExplicitInitialization) {
	for (int square = 0; square < 64; ++square) {
		SCOPED_TRACE(square);
		EXPECT_EQ(whitePawnAttacks(square),
				  expectedLeaperAttacks(square, whitePawnDirections));
		EXPECT_EQ(blackPawnAttacks(square),
				  expectedLeaperAttacks(square, blackPawnDirections));
		EXPECT_EQ(knightAttacks(square),
				  expectedLeaperAttacks(square, knightDirections));
		EXPECT_EQ(kingAttacks(square),
				  expectedLeaperAttacks(square, kingDirections));
	}
}

TEST(MoveData, SlidingAttacksMatchCoordinatesForEveryLineOccupancy) {
	for (int pattern = 0; pattern < 256; ++pattern) {
		SCOPED_TRACE(pattern);
		Bitboard rankStripes;
		Bitboard fileStripes;
		for (int rank = 0; rank < 8; ++rank) {
			for (int file = 0; file < 8; ++file) {
				if ((pattern & (1 << rank)) != 0) {
					rankStripes.set(8 * rank + file);
				}
				if ((pattern & (1 << file)) != 0) {
					fileStripes.set(8 * rank + file);
				}
			}
		}
		for (int square = 0; square < 64; ++square) {
			SCOPED_TRACE(square);
			// Rank stripes enumerate all file and diagonal occupancy bytes;
			// file stripes enumerate all rank occupancy bytes. Occupied squares
			// outside each ray also check that wrapped diagonals are excluded.
			{
				SCOPED_TRACE("rank stripes");
				expectSlidingAttacks(square, rankStripes);
			}
			{
				SCOPED_TRACE("file stripes");
				expectSlidingAttacks(square, fileStripes);
			}
		}
	}
}

TEST(MoveData, SlidingAttacksMatchCoordinatesForMixedOccupancy) {
	constexpr std::array<Bitboard, 5> occupancies{
		Bitboard{0x0123456789ABCDEFULL}, Bitboard{0x55AA55AA55AA55AAULL},
		Bitboard{0x8040201008040201ULL}, Bitboard{0x0102040810204080ULL},
		Bitboard{0x7EFFBDBBFFFF7E7FULL}};
	for (Bitboard occupancy : occupancies) {
		SCOPED_TRACE(occupancy.value());
		for (int square = 0; square < 64; ++square) {
			SCOPED_TRACE(square);
			expectSlidingAttacks(square, occupancy);
		}
	}
}
} // namespace morphy_chess
