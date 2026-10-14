#include <array>
#include <cstdio>
#include <gtest/gtest.h>
#include <morphychess/bits.hpp>
#include <morphychess/board.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/egtb.hpp>
#include <morphychess/evaluate.hpp>
#include <morphychess/move_data.hpp>
#include <morphychess/pawn_statistics.hpp>
#include <morphychess/see.hpp>
#include <morphychess/zobrist.hpp>
namespace morphy_chess {
TEST(Bits, CountsScansAndRemovesEdgeBits) {
	initBits();
	EXPECT_EQ(bitsCountOnes(~U64{0}), 64);
	EXPECT_EQ(bitsFirstOne(0), 64);
	EXPECT_EQ(bitsLastOne(0), -1);
	U64 value = 1;
	EXPECT_EQ(bitsRemoveLastOne(&value), 0);
	EXPECT_EQ(value, 0);
	value = U64{1} << 63;
	EXPECT_EQ(bitsRemoveFirstOne(&value), 63);
	EXPECT_EQ(value, 0);
	for (int square = 0; square < 64; ++square) {
		bitsSet(&value, square);
		EXPECT_TRUE(bitsTest(value, square));
		EXPECT_EQ(bitsMask(square), U64{1} << square);
		bitsClear(&value, square);
		EXPECT_EQ(value, 0);
	}
}
TEST(Bits, FlipsAndRotatedBoardsPreserveOccupancy) {
	initBits();
	U64 value = 1;
	bitsHorizontalFlip(&value);
	EXPECT_EQ(value, U64{1} << 7);
	bitsVerticalFlip(&value);
	EXPECT_EQ(value, U64{1} << 63);
	std::array<U64, 4> rotated{};
	rotatedBitsCreateSet(rotated.data(), (U64{1} << 63) | 1);
	for (U64 bits : rotated) {
		EXPECT_EQ(bitsCountOnes(bits), 2);
	}
	rotatedBitsClear(rotated.data(), 63);
	for (U64 bits : rotated) {
		EXPECT_EQ(bitsCountOnes(bits), 1);
	}
	rotatedBitsZeroOut(rotated.data());
	for (U64 bits : rotated) {
		EXPECT_EQ(bits, 0);
	}
}
TEST(Bits, FlipsMapEverySquareAndReverseRankOrder) {
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			const U64 original = U64{1} << (8 * rank + file);
			U64 vertical = original;
			bitsVerticalFlip(&vertical);
			EXPECT_EQ(vertical, U64{1} << (8 * (7 - rank) + file));
			bitsVerticalFlip(&vertical);
			EXPECT_EQ(vertical, original);
			U64 horizontal = original;
			bitsHorizontalFlip(&horizontal);
			EXPECT_EQ(horizontal, U64{1} << (8 * rank + 7 - file));
		}
	}
	U64 ranks = 0x0123456789ABCDEFULL;
	bitsVerticalFlip(&ranks);
	EXPECT_EQ(ranks, 0xEFCDAB8967452301ULL);
	U64 emptyBoard = 0;
	bitsVerticalFlip(&emptyBoard);
	EXPECT_EQ(emptyBoard, 0);
	U64 fullBoard = ~U64{0};
	bitsVerticalFlip(&fullBoard);
	EXPECT_EQ(fullBoard, ~U64{0});
}
TEST(Bits, RotatedBoardsMapEverySquareAndOverwritePreviousOccupancy) {
	initBits();
	std::array<U64, 4> rotated{};
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			rotated.fill(~U64{0});
			rotatedBitsCreateSet(rotated.data(), U64{1} << (8 * rank + file));
			EXPECT_EQ(rotated[kRankIndex], U64{1} << (8 * rank + file));
			EXPECT_EQ(rotated[kFileIndex], U64{1} << (8 * file + rank));
			EXPECT_EQ(rotated[kDiag1Index],
					  U64{1} << (8 * ((file - rank + 8) % 8) + rank));
			EXPECT_EQ(rotated[kDiag2Index],
					  U64{1} << (8 * ((7 - file - rank + 8) % 8) + rank));
		}
	}
	rotatedBitsCreateSet(rotated.data(), ~U64{0});
	for (U64 bits : rotated) {
		EXPECT_EQ(bits, ~U64{0});
	}
	rotatedBitsCreateSet(rotated.data(), 0);
	for (U64 bits : rotated) {
		EXPECT_EQ(bits, 0);
	}
}
TEST(MoveData, LeapersAndSlidingBlockers) {
	initMoveData();
	EXPECT_EQ(knightAttacks(kA1), (U64{1} << 10) | (U64{1} << 17));
	EXPECT_EQ(bitsCountOnes(kingAttacks(27)), 8);
	EXPECT_EQ(whitePawnAttacks(8), U64{1} << 17);
	EXPECT_EQ(blackPawnAttacks(55), U64{1} << 46);
	std::array<U64, 4> occupancy{};
	EXPECT_EQ(bitsCountOnes(rookAttacks(occupancy.data(), 27)), 14);
	EXPECT_EQ(bitsCountOnes(bishopAttacks(occupancy.data(), 27)), 13);
	rotatedBitsSet(occupancy.data(), 43);
	EXPECT_TRUE(bitsTest(rookAttacks(occupancy.data(), 27), 43));
	EXPECT_FALSE(bitsTest(rookAttacks(occupancy.data(), 27), 51));
	EXPECT_EQ(queenAttacks(occupancy.data(), 27),
			  bishopAttacks(occupancy.data(), 27) |
				  rookAttacks(occupancy.data(), 27));
}
TEST(Zobrist, KeysMatchIncrementalStateAndInitializationIsStable) {
	Board board;
	const U64 initial = board.positionKey();
	EXPECT_EQ(initial, computePositionKey(&board));
	initZobristKeys();
	EXPECT_EQ(initial, computePositionKey(&board));
	ASSERT_TRUE(board.makeMove(Move(12, 28)));
	EXPECT_NE(initial, board.positionKey());
	EXPECT_EQ(board.positionKey(), computePositionKey(&board));
	EXPECT_EQ(board.pawnKey(), computePawnKey(&board));
	EXPECT_TRUE(board.undoMove());
	EXPECT_EQ(initial, board.positionKey());
}
TEST(Evaluation, MaterialBalanceAndStaticExchangeRestoreBoard) {
	Board board;
	EXPECT_EQ(evaluate(&board), 0);
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/Q7/K7 w - - 0 1"));
	EXPECT_GT(evaluate(&board), 0);
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/q7/7K w - - 0 1"));
	EXPECT_LT(evaluate(&board), 0);
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/q7/R6K w - - 0 1"));
	const U64 original = board.positionKey();
	EXPECT_EQ(see(&board, Move(0, 8), 0), 900);
	EXPECT_EQ(board.positionKey(), original);
	EXPECT_EQ(board.halfMoveCount(), 0);
}
TEST(PawnStatistics, EncapsulatedValueFields) {
	constexpr PawnStatistics statistics(3, 5, 8);
	static_assert(statistics.lanes() == 3);
	EXPECT_EQ(statistics.moves(), 5);
	EXPECT_EQ(statistics.weak(), 8);
}
TEST(Egtb, ThreePieceIndexRoundTripAndValidation) {
	Board board;
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/R7/K7 w - - 0 1"));
	const int index = threeManEgtbIndex(board, rook, white);
	ASSERT_GE(index, 0);
	Board restored;
	ASSERT_TRUE(egtbIndexToBoard(&restored, rook, index, white));
	EXPECT_EQ(restored.positionKey(), board.positionKey());
	EXPECT_EQ(threeManEgtbIndex(restored, rook, white), index);
	EXPECT_FALSE(egtbIndexToBoard(&restored, rook, -1, white));
	EXPECT_FALSE(egtbIndexToBoard(&restored, king, index, white));
	EXPECT_EQ(threeManEgtbIndex(Board{}, rook, white), -1);
}
TEST(Debug, ConsistentBoardAndReadableStream) {
	Board board;
	assertBoardConsistency(&board);
	assertTranspositionKeyConsistency(&board);
	FILE *stream = std::tmpfile();
	ASSERT_NE(stream, nullptr);
	board.toStream(stream);
	EXPECT_GT(std::ftell(stream), 0);
	std::fclose(stream);
}
} // namespace morphy_chess
