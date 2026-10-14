#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include <morphychess/board.hpp>
#include <morphychess/zobrist.hpp>

namespace morphy_chess {
namespace {
// Recorded from all 780 keys of the original runtime generator. Hash bytes
// in a fixed order to verify the complete table without duplicating it here.
constexpr std::uint64_t legacyKeyFingerprint() {
	std::uint64_t fingerprint = 14695981039346656037ULL;
	const auto append = [&](std::uint64_t key) {
		for (int byte = 0; byte < 8; ++byte) {
			fingerprint ^= (key >> (8 * byte)) & 0xff;
			fingerprint *= 1099511628211ULL;
		}
	};
	for (int color = 0; color < 2; ++color) {
		for (int piece = pawn; piece <= king; ++piece) {
			for (int square = 0; square < 64; ++square) {
				append(zobristKey(color == 0 ? piece : -piece, square));
			}
		}
	}
	for (int file = 0; file < 8; ++file) {
		append(zobristEpKey(file));
	}
	for (int color = 0; color < 2; ++color) {
		append(zobristKingSideCastleKey(color));
		append(zobristQueenSideCastleKey(color));
	}
	return fingerprint;
}
static_assert(legacyKeyFingerprint() == 0x434b599defd764d0ULL);
static_assert(zobristData.pieceKey(pawn, 0) == 0x729a5f973ae6b929ULL);
static_assert(zobristKey(empty, 0) == 0);
static_assert(zobristRepeatKey(0) == 0);
static_assert(zobristRepeatKey(2) == 2);

struct HashFixture {
	const char *fen;
	std::uint64_t positionKey;
	std::uint64_t pawnKey;
};

// Captured by loading each FEN with the engine before replacing its generator.
constexpr std::array<HashFixture, 7> legacyHashes{{
	{initialPosition, 0x443797b64e31ad88ULL, 0x640e398bc9223df2ULL},
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1",
	 0xa5eaaf06fb869e93ULL, 0x640e398bc9223df2ULL},
	{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 0xf2249a1a5fb3a477ULL,
	 0x5e15261fc979b0afULL},
	{"r3k2r/8/8/8/8/8/8/R3K2R w Qk - 0 1", 0xd4efd24f9e0d88bbULL,
	 0x5e15261fc979b0afULL},
	{"7k/8/8/3pP3/8/8/8/K7 w - d6 0 1", 0x57a3a47b1350374fULL,
	 0xb67e9ccba6e70454ULL},
	{"7k/P7/8/8/8/8/8/7K w - - 0 1", 0x864a5d0f0015095bULL,
	 0x679765bfb5a23a40ULL},
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	 0x39c8db59b5f8ce5bULL, 0xf31d5f96e3e8c2b8ULL},
}};
} // namespace

TEST(Zobrist, PreservesEveryLegacyKey) {
	EXPECT_EQ(legacyKeyFingerprint(), 0x434b599defd764d0ULL);
}

TEST(Zobrist, PreservesPositionAndPawnHashesFromRuntimeTables) {
	Board board;
	for (const auto &fixture : legacyHashes) {
		SCOPED_TRACE(fixture.fen);
		ASSERT_TRUE(board.loadFen(fixture.fen));
		EXPECT_EQ(board.positionKey(), fixture.positionKey);
		EXPECT_EQ(board.pawnKey(), fixture.pawnKey);
		EXPECT_EQ(board.computePositionKey(), fixture.positionKey);
		EXPECT_EQ(board.computePawnKey(), fixture.pawnKey);
	}
}
} // namespace morphy_chess
