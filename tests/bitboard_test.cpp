#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <gtest/gtest.h>

#include <morphychess/bitboard.hpp>

namespace morphy_chess {
namespace {
constexpr bool supportsConstantEvaluation() {
	Bitboard board;
	board.set(0);
	board.set(63);
	if (board.countOnes() != 2 || board.firstOne() != 0 ||
		board.lastOne() != 63 || !board.test(63)) {
		return false;
	}
	board.clear(0);
	board.verticalFlip();
	board.horizontalFlip();
	if (board != Bitboard::mask(0)) {
		return false;
	}
	board <<= 63;
	board >>= 62;
	board |= Bitboard::mask(0);
	board ^= Bitboard::mask(1);
	board &= Bitboard::mask(0);
	if (board.removeFirstOne() != 0 || board.removeLastOne() != -1 || board) {
		return false;
	}
	board = Bitboard::mask(63);
	return board.removeLastOne() == 63 && board.removeFirstOne() == 64 &&
		   board.value() == 0;
}

static_assert(sizeof(Bitboard) == sizeof(std::uint64_t));
static_assert(sizeof(Bitboard) == 8);
static_assert(std::is_trivially_copyable_v<Bitboard>);
static_assert(std::is_standard_layout_v<Bitboard>);
static_assert(!std::is_convertible_v<std::uint64_t, Bitboard>);
static_assert(!std::is_convertible_v<Bitboard, std::uint64_t>);
static_assert(!std::is_convertible_v<Bitboard, bool>);
static_assert(supportsConstantEvaluation());
} // namespace

TEST(Bitboard, ExplicitRawValuesAndBooleanState) {
	constexpr std::array<std::uint64_t, 5> values{
		0, 1, std::uint64_t{1} << 63, 0x0123456789ABCDEFULL,
		std::numeric_limits<std::uint64_t>::max()};
	EXPECT_EQ(Bitboard{}.value(), std::uint64_t{0});
	for (std::uint64_t value : values) {
		const Bitboard board{value};
		EXPECT_EQ(board.value(), value);
		EXPECT_EQ(static_cast<bool>(board), value != 0);
		EXPECT_EQ(!board, value == 0);
		EXPECT_EQ(board, Bitboard{value});
		EXPECT_NE(board, ~board);
	}
}

TEST(Bitboard, ScansAndRemovesEverySquareAndEmptyBoard) {
	Bitboard empty;
	EXPECT_EQ(empty.countOnes(), 0);
	EXPECT_EQ(empty.firstOne(), 64);
	EXPECT_EQ(empty.lastOne(), -1);
	EXPECT_EQ(empty.removeFirstOne(), 64);
	EXPECT_EQ(empty.removeLastOne(), -1);
	EXPECT_EQ(empty, Bitboard{});
	for (int square = 0; square < 64; ++square) {
		Bitboard board = Bitboard::mask(square);
		EXPECT_EQ(board.countOnes(), 1);
		EXPECT_EQ(board.firstOne(), square);
		EXPECT_EQ(board.lastOne(), square);
		EXPECT_EQ(board.removeFirstOne(), square);
		EXPECT_EQ(board, Bitboard{});
		board.set(square);
		EXPECT_EQ(board.removeLastOne(), square);
		EXPECT_EQ(board, Bitboard{});
	}
	Bitboard ascending = ~Bitboard{};
	Bitboard descending = ascending;
	for (int square = 0; square < 64; ++square) {
		EXPECT_EQ(ascending.countOnes(), 64 - square);
		EXPECT_EQ(descending.countOnes(), 64 - square);
		EXPECT_EQ(ascending.removeFirstOne(), square);
		EXPECT_EQ(descending.removeLastOne(), 63 - square);
	}
	EXPECT_EQ(ascending, Bitboard{});
	EXPECT_EQ(descending, Bitboard{});
}

TEST(Bitboard, SetAndClearPreserveOtherSquares) {
	Bitboard board;
	for (int square = 0; square < 64; ++square) {
		board.set(square);
		board.set(square);
		EXPECT_TRUE(board.test(square));
		EXPECT_EQ(board.countOnes(), square + 1);
		EXPECT_EQ(Bitboard::mask(square).value(), std::uint64_t{1} << square);
	}
	EXPECT_EQ(board, ~Bitboard{});
	for (int square = 0; square < 64; ++square) {
		board.clear(square);
		board.clear(square);
		EXPECT_FALSE(board.test(square));
		EXPECT_EQ(board.countOnes(), 63 - square);
	}
	EXPECT_EQ(board, Bitboard{});
}

TEST(Bitboard, BitwiseOperationsAndCompoundAssignments) {
	const Bitboard lhs{0xF00FF00F0000FFFFULL};
	const Bitboard rhs{0x0FF00FF00000FFFFULL};
	EXPECT_EQ((lhs & rhs).value(), 0x000000000000FFFFULL);
	EXPECT_EQ((lhs | rhs).value(), 0xFFFFFFFF0000FFFFULL);
	EXPECT_EQ((lhs ^ rhs).value(), 0xFFFFFFFF00000000ULL);
	EXPECT_EQ((~lhs).value(), 0x0FF00FF0FFFF0000ULL);
	Bitboard copy = lhs;
	copy &= rhs;
	EXPECT_EQ(copy, lhs & rhs);
	copy = lhs;
	copy |= rhs;
	EXPECT_EQ(copy, lhs | rhs);
	copy = lhs;
	copy ^= rhs;
	EXPECT_EQ(copy, lhs ^ rhs);
	EXPECT_EQ(lhs.value(), 0xF00FF00F0000FFFFULL);
	EXPECT_EQ(rhs.value(), 0x0FF00FF00000FFFFULL);
}

TEST(Bitboard, ShiftsRetainUnsignedBoundaryBehavior) {
	constexpr std::uint64_t raw = 0x8123456789ABCDEFULL;
	const Bitboard board{raw};
	for (int shift = 0; shift < 64; ++shift) {
		EXPECT_EQ((board << shift).value(), raw << shift);
		EXPECT_EQ((board >> shift).value(), raw >> shift);
		Bitboard left = board;
		Bitboard right = board;
		left <<= shift;
		right >>= shift;
		EXPECT_EQ(left, board << shift);
		EXPECT_EQ(right, board >> shift);
	}
	EXPECT_EQ((Bitboard::mask(63) << 1), Bitboard{});
	EXPECT_EQ((Bitboard::mask(0) >> 1), Bitboard{});
	EXPECT_EQ((Bitboard::mask(63) >> 63), Bitboard::mask(0));
}
} // namespace morphy_chess
