#include <array>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <morphychess/bitboard.hpp>
#include <morphychess/bits.hpp>
#include <morphychess/board.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/egtb.hpp>
#include <morphychess/evaluate.hpp>
#include <morphychess/move_data.hpp>
#include <morphychess/pawn_statistics.hpp>
#include <morphychess/zobrist.hpp>
namespace morphy_chess {
TEST(Bits, CountsScansAndRemovesEdgeBits) {
	initBits();
	EXPECT_EQ((~Bitboard{}).countOnes(), 64);
	EXPECT_EQ(Bitboard{}.firstOne(), 64);
	EXPECT_EQ(Bitboard{}.lastOne(), -1);
	Bitboard value{1};
	EXPECT_EQ(value.removeLastOne(), 0);
	EXPECT_EQ(value, Bitboard{});
	value = Bitboard{1} << 63;
	EXPECT_EQ(value.removeFirstOne(), 63);
	EXPECT_EQ(value, Bitboard{});
	for (int square = 0; square < 64; ++square) {
		value.set(square);
		EXPECT_TRUE(value.test(square));
		EXPECT_EQ(Bitboard::mask(square), Bitboard{1} << square);
		value.clear(square);
		EXPECT_EQ(value, Bitboard{});
	}
}
TEST(Bits, FlipsAndRotatedBoardsPreserveOccupancy) {
	initBits();
	Bitboard value{1};
	value.horizontalFlip();
	EXPECT_EQ(value, Bitboard{1} << 7);
	value.verticalFlip();
	EXPECT_EQ(value, Bitboard{1} << 63);
	RotatedBitboard rotated((Bitboard{1} << 63) | Bitboard{1});
	for (Bitboard bits : rotated.rotations()) {
		EXPECT_EQ(bits.countOnes(), 2);
	}
	rotated.clear(63);
	for (Bitboard bits : rotated.rotations()) {
		EXPECT_EQ(bits.countOnes(), 1);
	}
	rotated.reset();
	for (Bitboard bits : rotated.rotations()) {
		EXPECT_EQ(bits, Bitboard{});
	}
}
TEST(Bits, FlipsMapEverySquareAndReverseRankOrder) {
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			const Bitboard original = Bitboard{1} << (8 * rank + file);
			Bitboard vertical = original;
			vertical.verticalFlip();
			EXPECT_EQ(vertical, Bitboard{1} << (8 * (7 - rank) + file));
			vertical.verticalFlip();
			EXPECT_EQ(vertical, original);
			Bitboard horizontal = original;
			horizontal.horizontalFlip();
			EXPECT_EQ(horizontal, Bitboard{1} << (8 * rank + 7 - file));
		}
	}
	Bitboard ranks{0x0123456789ABCDEFULL};
	ranks.verticalFlip();
	EXPECT_EQ(ranks, Bitboard{0xEFCDAB8967452301ULL});
	Bitboard emptyBoard;
	emptyBoard.verticalFlip();
	EXPECT_EQ(emptyBoard, Bitboard{});
	Bitboard fullBoard = ~Bitboard{0};
	fullBoard.verticalFlip();
	EXPECT_EQ(fullBoard, ~Bitboard{0});
}
TEST(Bits, RotatedBoardsMapEverySquareAndOverwritePreviousOccupancy) {
	initBits();
	RotatedBitboard rotated;
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			rotated.assign(~Bitboard{0});
			rotated.assign(Bitboard{1} << (8 * rank + file));
			EXPECT_EQ(rotated[kRankIndex], Bitboard{1} << (8 * rank + file));
			EXPECT_EQ(rotated[kFileIndex], Bitboard{1} << (8 * file + rank));
			EXPECT_EQ(rotated[kDiag1Index],
					  Bitboard{1} << (8 * ((file - rank + 8) % 8) + rank));
			EXPECT_EQ(rotated[kDiag2Index],
					  Bitboard{1} << (8 * ((7 - file - rank + 8) % 8) + rank));
		}
	}
	rotated.assign(~Bitboard{0});
	for (Bitboard bits : rotated.rotations()) {
		EXPECT_EQ(bits, ~Bitboard{0});
	}
	rotated.assign(Bitboard{});
	for (Bitboard bits : rotated.rotations()) {
		EXPECT_EQ(bits, Bitboard{});
	}
}
TEST(MoveData, LeapersAndSlidingBlockers) {
	initMoveData();
	EXPECT_EQ(knightAttacks(kA1), (Bitboard{1} << 10) | (Bitboard{1} << 17));
	EXPECT_EQ(kingAttacks(27).countOnes(), 8);
	EXPECT_EQ(whitePawnAttacks(8), Bitboard{1} << 17);
	EXPECT_EQ(blackPawnAttacks(55), Bitboard{1} << 46);
	RotatedBitboard occupancy;
	EXPECT_EQ(occupancy.rookAttacks(27).countOnes(), 14);
	EXPECT_EQ(occupancy.bishopAttacks(27).countOnes(), 13);
	occupancy.set(43);
	EXPECT_TRUE(occupancy.rookAttacks(27).test(43));
	EXPECT_FALSE(occupancy.rookAttacks(27).test(51));
	EXPECT_EQ(occupancy.queenAttacks(27),
			  occupancy.bishopAttacks(27) | occupancy.rookAttacks(27));
}
TEST(Zobrist, KeysMatchIncrementalStateAndInitializationIsStable) {
	Board board;
	const std::uint64_t initial = board.positionKey();
	EXPECT_EQ(initial, board.computePositionKey());
	initZobristKeys();
	EXPECT_EQ(initial, board.computePositionKey());
	ASSERT_TRUE(board.makeMove(Move(12, 28)));
	EXPECT_NE(initial, board.positionKey());
	EXPECT_EQ(board.positionKey(), board.computePositionKey());
	EXPECT_EQ(board.pawnKey(), board.computePawnKey());
	EXPECT_TRUE(board.undoMove());
	EXPECT_EQ(initial, board.positionKey());
}
TEST(Evaluation, MaterialBalanceAndStaticExchangeRestoreBoard) {
	Board board;
	EXPECT_EQ(board.evaluate(), 0);
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/Q7/K7 w - - 0 1"));
	EXPECT_GT(board.evaluate(), 0);
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/q7/7K w - - 0 1"));
	EXPECT_LT(board.evaluate(), 0);
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/q7/R6K w - - 0 1"));
	const std::uint64_t original = board.positionKey();
	EXPECT_EQ(board.see(Move(0, 8), 0), 900);
	EXPECT_EQ(board.positionKey(), original);
	EXPECT_EQ(board.halfMoveCount(), 0);
}
TEST(PawnStatistics, EncapsulatedValueFields) {
	constexpr PawnStatistics statistics(Bitboard{3}, Bitboard{5}, Bitboard{8});
	static_assert(statistics.lanes() == Bitboard{3});
	EXPECT_EQ(statistics.moves(), Bitboard{5});
	EXPECT_EQ(statistics.weak(), Bitboard{8});
}
TEST(Egtb, ThreePieceIndexRoundTripAndValidation) {
	Board board;
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/R7/K7 w - - 0 1"));
	const int index = board.threeManEgtbIndex(rook, white);
	ASSERT_GE(index, 0);
	Board restored;
	ASSERT_TRUE(restored.loadEgtbIndex(rook, index, white));
	EXPECT_EQ(restored.positionKey(), board.positionKey());
	EXPECT_EQ(restored.threeManEgtbIndex(rook, white), index);
	EXPECT_FALSE(restored.loadEgtbIndex(rook, -1, white));
	EXPECT_FALSE(restored.loadEgtbIndex(king, index, white));
	EXPECT_EQ(Board{}.threeManEgtbIndex(rook, white), -1);
}
TEST(Debug, ConsistentBoardAndReadableStream) {
	Board board;
	board.assertBoardConsistency();
	board.assertTranspositionKeyConsistency();
	const auto closeFile = [](FILE *file) { std::fclose(file); };
	std::unique_ptr<FILE, decltype(closeFile)> stream(std::tmpfile(), closeFile);
	ASSERT_NE(stream.get(), nullptr);
	board.toStream(stream.get());
	EXPECT_GT(std::ftell(stream.get()), 0);
}
} // namespace morphy_chess
