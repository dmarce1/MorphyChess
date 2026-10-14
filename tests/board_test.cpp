#include <array>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include <morphychess/board.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/zobrist.hpp>

namespace morphy_chess {
namespace {
std::uint64_t perft(Board &board, int depth) {
	if (depth == 0) {
		return 1;
	}
	std::array<Move, maxMoves> moves;
	const int count = board.genPseudolegalMoves(moves.data());
	std::uint64_t result = 0;
	for (int index = 0; index < count; ++index) {
		if (board.makeMove(moves[index])) {
			result += perft(board, depth - 1);
			EXPECT_TRUE(board.undoMove());
		}
	}
	return result;
}
void expectSamePosition(const Board &lhs, const Board &rhs) {
	for (int square = 0; square < 64; ++square) {
		EXPECT_EQ(lhs.pieceValue(square), rhs.pieceValue(square));
	}
	EXPECT_EQ(lhs.positionKey(), rhs.positionKey());
	EXPECT_EQ(lhs.pawnKey(), rhs.pawnKey());
	EXPECT_EQ(lhs.sideToMove(), rhs.sideToMove());
	EXPECT_EQ(lhs.halfMoveCount(), rhs.halfMoveCount());
	EXPECT_EQ(lhs.fiftyMoveRule(), rhs.fiftyMoveRule());
	for (int color = black; color <= white; ++color) {
		EXPECT_EQ(lhs.hasKingsideCastled(color), rhs.hasKingsideCastled(color));
		EXPECT_EQ(lhs.hasQueensideCastled(color),
				  rhs.hasQueensideCastled(color));
	}
}
} // namespace
TEST(Move, EmptyValueCoordinatesAndPromotionParsing) {
	EXPECT_TRUE(Move{}.isEmpty());
	EXPECT_EQ(Move::fromString("e2e4"), Move(12, 28));
	EXPECT_EQ(Move::fromString("a7a8q"), Move(48, 56, queen));
	EXPECT_EQ(Move::fromString("a7a8=N"), Move(48, 56, knight));
	for (const char *text :
		 {"", "a", "a1", "a1b", "i1a1", "a0a1", "a7a8k", "a7a8="}) {
		EXPECT_TRUE(Move::fromString(text).isEmpty()) << text;
	}
	EXPECT_TRUE(Move::fromString(nullptr).isEmpty());
}
TEST(Board, InitialPositionAndPerftThroughFourPlies) {
	Board board;
	EXPECT_EQ(board.sideToMove(), white);
	EXPECT_EQ(board.pieceCount(white, pawn), 8);
	EXPECT_EQ(board.pieceCount(black, 0), 16);
	EXPECT_EQ(board.result(), inProgress);
	EXPECT_EQ(perft(board, 1), 20);
	EXPECT_EQ(perft(board, 2), 400);
	EXPECT_EQ(perft(board, 3), 8902);
	EXPECT_EQ(perft(board, 4), 197281);
}
TEST(Board, CastlingPositionPerftAndEnPassantEndgame) {
	Board board;
	ASSERT_TRUE(board.loadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/"
							  "PPPBBPPP/R3K2R w KQkq - 0 1"));
	EXPECT_EQ(perft(board, 1), 48);
	EXPECT_EQ(perft(board, 2), 2039);
	EXPECT_EQ(perft(board, 3), 97862);
	ASSERT_TRUE(board.loadFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"));
	EXPECT_EQ(perft(board, 1), 14);
	EXPECT_EQ(perft(board, 2), 191);
	EXPECT_EQ(perft(board, 3), 2812);
}
TEST(Board, RejectsMalformedFenWithoutChangingPosition) {
	Board board;
	const Board before = board;
	for (const char *fen :
		 {"", "8/8", "8/8/8/8/8/8/8/8 w - - 0 1", "8/8/8/8/8/8/8/K6k x - - 0 1",
		  "8/8/8/8/8/8/8/K6k w K - 0 1", "8/8/8/8/8/8/8/K6k w - z9 0 1",
		  "8/8/8/8/8/8/8/K6k w - - -1 1"}) {
		EXPECT_FALSE(board.loadFen(fen));
		expectSamePosition(board, before);
	}
	EXPECT_FALSE(board.loadFen(nullptr));
	EXPECT_FALSE(board.loadFen(std::string(4096, '8').c_str()));
}
TEST(Board, MaterialDrawIsIndependentOfHalfmoveClock) {
	Board board;
	for (const char *fen :
		 {"7k/8/8/8/8/8/8/K7 w - - 0 1", "7k/8/8/8/8/8/8/K7 w - - 1 1",
		  "7k/8/8/8/8/8/8/KN6 w - - 99 1"}) {
		ASSERT_TRUE(board.loadFen(fen));
		EXPECT_TRUE(board.isDraw());
		EXPECT_EQ(board.result(), draw);
	}
}
TEST(Board, MoveUndoCastlingEnPassantPromotionAndHashConsistency) {
	Board board;
	const std::array<std::pair<const char *, Move>, 3> examples{
		{{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", Move(kE1, kG1)},
		 {"7k/8/8/3pP3/8/8/8/K7 w - d6 0 1", Move(36, 43)},
		 {"7k/P7/8/8/8/8/8/7K w - - 0 1", Move(48, 56, queen)}}};
	for (const auto &[fen, move] : examples) {
		ASSERT_TRUE(board.loadFen(fen));
		const Board original = board;
		ASSERT_TRUE(board.isPseudoLegalMove(move));
		ASSERT_TRUE(board.makeMove(move));
		EXPECT_EQ(board.positionKey(), board.computePositionKey());
		EXPECT_EQ(board.pawnKey(), board.computePawnKey());
		board.assertBoardConsistency();
		ASSERT_TRUE(board.undoMove());
		expectSamePosition(board, original);
	}
	EXPECT_FALSE(board.undoMove());
}
TEST(Board, EnPassantIsACaptureAndSeeRestoresBothColors) {
	const std::array<std::pair<const char *, Move>, 2> examples{
		{{"4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", Move(36, 43)},
		 {"4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1", Move(28, 19)}}};
	for (const auto &[fen, move] : examples) {
		SCOPED_TRACE(fen);
		Board board;
		ASSERT_TRUE(board.loadFen(fen));
		const Board original = board;
		EXPECT_TRUE(board.moveIsCapture(move));
		EXPECT_EQ(board.see(move, 0), 100);
		expectSamePosition(board, original);
		ASSERT_TRUE(board.makeMove(move));
		EXPECT_TRUE(board.lastMoveWasCapture());
		// The ordinary destination-capture field stays empty for EP undo.
		EXPECT_EQ(board.mostRecentCapture(), empty);
		EXPECT_EQ(board.pieceCount(colorFlip(original.sideToMove()), pawn), 0);
		EXPECT_EQ(board.positionKey(), board.computePositionKey());
		EXPECT_EQ(board.pawnKey(), board.computePawnKey());
		ASSERT_TRUE(board.undoMove());
		expectSamePosition(board, original);
		EXPECT_EQ(board.pieceCount(white, pawn), 1);
		EXPECT_EQ(board.pieceCount(black, pawn), 1);
		EXPECT_FALSE(board.lastMoveWasCapture());
	}
}
TEST(Board, NullMoveDoesNotInheritThePreviousCapture) {
	Board board;
	EXPECT_FALSE(board.moveIsCapture(Move{}));
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/p7/R6K w - - 0 1"));
	ASSERT_TRUE(board.makeMove(Move(kA1, 8)));
	EXPECT_TRUE(board.lastMoveWasCapture());
	ASSERT_TRUE(board.makeMove(Move{}));
	EXPECT_FALSE(board.lastMoveWasCapture());
	ASSERT_TRUE(board.undoMove());
	EXPECT_TRUE(board.lastMoveWasCapture());
}
TEST(Board, SeeTriesLegalRecapturesAfterPinnedAttackers) {
	const std::array<std::pair<const char *, Move>, 2> examples{
		{{"3rk3/4p3/3n4/2B5/8/8/8/K3R3 w - - 0 1", Move(kD8, 43)},
		 {"2k5/2p1p3/3n4/2B5/8/8/8/K1R5 w - - 0 1", Move(52, 43)}}};
	for (const auto &[fen, recapture] : examples) {
		SCOPED_TRACE(fen);
		Board board;
		ASSERT_TRUE(board.loadFen(fen));
		const Board original = board;
		const Move capture(34, 43);
		// The bishop wins a knight and is recaptured: material is equal.
		EXPECT_EQ(board.see(capture, 0), 0);
		expectSamePosition(board, original);
		ASSERT_TRUE(board.makeMove(capture));
		const Board afterCapture = board;
		Move selected;
		ASSERT_TRUE(board.genLvaAttackToSquare(selected, 43));
		EXPECT_EQ(selected, recapture);
		expectSamePosition(board, afterCapture);
		EXPECT_EQ(board.positionKey(), board.computePositionKey());
		EXPECT_EQ(board.pawnKey(), board.computePawnKey());
		ASSERT_TRUE(board.undoMove());
		expectSamePosition(board, original);
	}
}
TEST(Board, SeeIncludesPromotionGainAndSubsequentRecapture) {
	struct Example {
		const char *fen;
		Move move;
		int score;
	};
	const std::array<Example, 6> examples{
		{{"7k/P7/8/8/8/8/8/7K w - - 0 1", Move(48, 56, queen), 800},
		 {"7k/P7/8/8/8/8/8/7K w - - 0 1", Move(48, 56, knight), 200},
		 {"1r5k/P7/8/8/8/8/8/7K w - - 0 1", Move(48, 57, queen), 1300},
		 {"1r5k/P7/8/8/8/8/8/7K w - - 0 1", Move(48, 56, queen), -100},
		 {"7K/8/8/8/8/8/p7/7k b - - 0 1", Move(8, 0, queen), 800},
		 {"7k/8/8/8/8/R6K/1p6/b7 w - - 0 1", Move(16, 0), -1000}}};
	for (const auto &[fen, move, score] : examples) {
		SCOPED_TRACE(fen);
		Board board;
		ASSERT_TRUE(board.loadFen(fen));
		const Board original = board;
		ASSERT_TRUE(board.isLegalMove(move));
		EXPECT_EQ(board.see(move, 0), score);
		expectSamePosition(board, original);
		EXPECT_EQ(board.positionKey(), board.computePositionKey());
		EXPECT_EQ(board.pawnKey(), board.computePawnKey());
	}
}
TEST(Board, PromotedArmyUsesMoreThanLegacyMoveBuffer) {
	Board board;
	ASSERT_TRUE(board.loadFen("7k/4QQ2/Q5Q1/2Q5/5Q1Q/3Q4/2Q5/K7 w - - 0 1"));
	std::array<Move, maxMoves> moves;
	const int count = board.genPseudolegalMoves(moves.data());
	EXPECT_GT(count, 128);
	EXPECT_LT(count, maxMoves);
	EXPECT_TRUE(moves[count].isEmpty());
}
TEST(Board, RepetitionAndValueCopyPreserveHistory) {
	Board board;
	const std::array<Move, 4> cycle{Move(6, 21), Move(62, 45), Move(21, 6),
									Move(45, 62)};
	for (int repetition = 0; repetition < 2; ++repetition) {
		for (Move move : cycle) {
			ASSERT_TRUE(board.makeMove(move));
		}
	}
	EXPECT_TRUE(board.isDraw());
	EXPECT_EQ(board.repetitions(), 2);
	Board copied = board;
	EXPECT_TRUE(copied.undoMove());
	EXPECT_EQ(board.halfMoveCount(), 8);
	EXPECT_EQ(copied.halfMoveCount(), 7);
	Board castled;
	ASSERT_TRUE(castled.loadFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
	ASSERT_TRUE(castled.makeMove(Move(kE1, kG1)));
	copied.copy(castled);
	EXPECT_TRUE(copied.hasKingsideCastled(white));
	expectSamePosition(copied, castled);
}
TEST(Board, SanDisambiguationIgnoresPinnedCandidates) {
	Board board;
	for (const char *move :
		 {"e4", "e5", "Nf3", "Nc6", "Bc4", "Nf6", "Ng5", "d5", "exd5", "Nxd5",
		  "Nxf7", "Kxf7", "Qf3+", "Ke6", "Nc3"}) {
		ASSERT_FALSE(board.makeTextMove(move).isEmpty()) << move;
	}
	EXPECT_EQ(board.makeTextMove("Nb4"), Move(42, 25));
}
TEST(Board, TextMovesRejectWrappedCoordinatesWithoutChangingPosition) {
	Board board;
	const Board initial = board;
	for (const char *text :
		 {"e2u2", "e0e4", "e2e9", "e", "", "Xe4", "e2ee4", "\xFF" "e4"}) {
		SCOPED_TRACE(text);
		EXPECT_TRUE(board.makeTextMove(text).isEmpty());
		expectSamePosition(board, initial);
	}
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/8/R6K w - - 0 1"));
	const Board rookPosition = board;
	for (const char *text : {"a9a2", "a1a0"}) {
		SCOPED_TRACE(text);
		EXPECT_TRUE(board.makeTextMove(text).isEmpty());
		expectSamePosition(board, rookPosition);
	}
	// Fully specified coordinate moves retain support for nonpawn pieces.
	EXPECT_EQ(board.makeTextMove("a1a2"), Move(kA1, 8));
}
TEST(Board, TextMovesRequireUniqueLegalDisambiguation) {
	Board board;
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/8/KN3N2 w - - 0 1"));
	const Board original = board;
	for (const char *text : {"Nd2", "N1d2"}) {
		SCOPED_TRACE(text);
		EXPECT_TRUE(board.makeTextMove(text).isEmpty());
		expectSamePosition(board, original);
	}
	EXPECT_EQ(board.makeTextMove("Nbd2"), Move(1, 11));
	ASSERT_TRUE(board.undoMove());
	EXPECT_EQ(board.makeTextMove("Nfd2"), Move(5, 11));
	ASSERT_TRUE(board.loadFen("4r2k/8/8/8/8/8/2N1N3/4K3 w - - 0 1"));
	// e2 is pinned by the rook, leaving c2 as the only legal source for Nd4.
	EXPECT_EQ(board.makeTextMove("Nd4"), Move(10, 27));
}
TEST(Board, TextMovesAndBlackStalemate) {
	Board board;
	EXPECT_EQ(board.makeTextMove("e4"), Move(12, 28));
	EXPECT_EQ(board.makeTextMove("e5"), Move(52, 36));
	EXPECT_EQ(board.makeTextMove("Nf3"), Move(6, 21));
	EXPECT_TRUE(board.makeTextMove("garbage").isEmpty());
	ASSERT_TRUE(board.loadFen("7k/5K2/6Q1/8/8/8/8/8 b - - 0 1"));
	EXPECT_EQ(board.result(), draw);
	ASSERT_TRUE(board.loadFen("7k/6Q1/5K2/8/8/8/8/8 b - - 100 1"));
	EXPECT_EQ(board.result(), whiteWin);
}
} // namespace morphy_chess
