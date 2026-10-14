#include <string>

#include <gtest/gtest.h>

#include <morphychess/pgn.hpp>

namespace morphy_chess {
namespace {

TEST(Pgn, ParsesHeadersMovesAndTrailingWhitespace) {
	Board board;
	Pgn pgn;
	ASSERT_TRUE(pgn.loadText(
		"[Event \"Example\"]\n[Site \"Here\"]\n[White \"Morphy\"]\n[Black "
		"\"Opponent\"]\n"
		"[Date \"1858.06.15\"]\n[Round \"2\"]\n[WhiteElo \"2400\"]\n[BlackElo "
		"\"2300\"]\n"
		"[ECO \"C20\"]\n[Result \"1-0\"]\n\n1.e4 e5 2.Nf3 Nc6 1-0\n\n  ",
		board));
	EXPECT_EQ(pgn.event(), "Example");
	EXPECT_EQ(pgn.site(), "Here");
	EXPECT_EQ(pgn.whiteName(), "Morphy");
	EXPECT_EQ(pgn.blackName(), "Opponent");
	EXPECT_EQ(pgn.year(), 1858);
	EXPECT_EQ(pgn.month(), 6);
	EXPECT_EQ(pgn.day(), 15);
	EXPECT_EQ(pgn.round(), 2);
	EXPECT_EQ(pgn.whiteElo(), 2400);
	EXPECT_EQ(pgn.blackElo(), 2300);
	EXPECT_EQ(pgn.eco(), "C20");
	EXPECT_EQ(pgn.result(), whiteWin);
	EXPECT_EQ(pgn.moveCount(), 4);
	EXPECT_EQ(board.halfMoveCount(), 4);
	EXPECT_EQ(pgn.moves().front(), Move(12, 28));
	EXPECT_FALSE(pgn.next(board));
	EXPECT_FALSE(pgn.hasError());
}

TEST(Pgn, SkipsCommentsNestedVariationsAndAnnotations) {
	Board board;
	Pgn pgn;
	ASSERT_TRUE(pgn.loadText(
		"{initial comment}\n1. e4! {comment} (1. d4 d5 (1...Nf6)) e5 $1\n"
		"2. Nf3?! ;rest of line\nNc6 *\n",
		board));
	EXPECT_EQ(pgn.moveCount(), 4);
	EXPECT_EQ(pgn.result(), inProgress);
	EXPECT_EQ(board.halfMoveCount(), 4);
	EXPECT_FALSE(pgn.next(board));
}

TEST(Pgn, AdvancesGamesAndResetsMetadata) {
	Board board;
	Pgn pgn;
	ASSERT_TRUE(pgn.loadText(
		"[ECO \"B00\"]\n1.e4 *\n\n[White \"Next\"]\n1.d4 d5 1/2-1/2\n", board));
	EXPECT_EQ(pgn.moveCount(), 1);
	ASSERT_TRUE(pgn.next(board));
	EXPECT_EQ(pgn.moveCount(), 2);
	EXPECT_EQ(pgn.whiteName(), "Next");
	EXPECT_EQ(pgn.eco(), "");
	EXPECT_EQ(pgn.result(), draw);
	EXPECT_EQ(board.halfMoveCount(), 2);
	EXPECT_FALSE(pgn.next(board));
	EXPECT_FALSE(pgn.next(board));
}

TEST(Pgn, ParsesAdjacentAnnotationAndResultTokens) {
	Board board;
	Pgn pgn;
	ASSERT_TRUE(pgn.loadText("1.e4$1e5$2 2.Nf3$0$255Nc6*1.d4*", board));
	EXPECT_EQ(pgn.moveCount(), 4);
	EXPECT_EQ(board.halfMoveCount(), 4);
	EXPECT_EQ(pgn.result(), inProgress);
	ASSERT_TRUE(pgn.next(board));
	EXPECT_EQ(pgn.moveCount(), 1);
	EXPECT_EQ(board.pieceValue(27), pawn);
	EXPECT_FALSE(pgn.next(board));
	EXPECT_FALSE(pgn.hasError());
}

TEST(Pgn, RejectsMalformedNumericAnnotationsWithoutReplacingBoard) {
	Board board;
	ASSERT_TRUE(board.makeMove(Move(11, 27)));
	const auto key = board.positionKey();
	Pgn pgn;
	for (const auto *text : {"1.e4$ *", "1.e4$-1 *", "1.e4$256 *",
							 "1.e4$999999999999999999999999 *"}) {
		EXPECT_FALSE(pgn.loadText(text, board)) << text;
		EXPECT_TRUE(pgn.hasError());
		EXPECT_EQ(board.positionKey(), key);
	}
}

TEST(Pgn, AcceptsFinalGameWithoutResultOrFinalNewline) {
	Board board;
	Pgn pgn;
	ASSERT_TRUE(pgn.loadText("1. e4 e5\n   ", board));
	EXPECT_EQ(pgn.moveCount(), 2);
	EXPECT_FALSE(pgn.next(board));
	EXPECT_FALSE(pgn.hasError());
	ASSERT_TRUE(pgn.loadText("1. d4", board));
	EXPECT_EQ(pgn.moveCount(), 1);
	EXPECT_FALSE(pgn.next(board));
}

TEST(Pgn, RejectsMalformedInputWithoutReplacingBoard) {
	Board board;
	ASSERT_TRUE(board.makeMove(Move(11, 27)));
	const auto key = board.positionKey();
	Pgn pgn;
	EXPECT_FALSE(pgn.loadText("1.e4 Qa9 *", board));
	EXPECT_TRUE(pgn.hasError());
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_FALSE(pgn.loadText("[Event \"unfinished", board));
	EXPECT_FALSE(pgn.loadText("1.e4 {unfinished", board));
	EXPECT_FALSE(pgn.loadText("1.e4 (1.d4", board));
	EXPECT_FALSE(pgn.loadText("1.e4 ) *", board));
	EXPECT_FALSE(pgn.loadText("\n \t", board));
	EXPECT_FALSE(pgn.hasError());
}

TEST(Pgn, AcceptsOwnedLongHeadersAndEscapedQuotes) {
	Board board;
	Pgn pgn;
	const std::string name(600, 'a');
	ASSERT_TRUE(pgn.loadText("[White \"" + name +
								 "\"]\n[Event \"A \\\"quote\\\"\"]\n1.e4 *",
							 board));
	EXPECT_EQ(pgn.whiteName(), name);
	EXPECT_EQ(pgn.event(), "A \"quote\"");
}

TEST(Pgn, LoadsFenStartingPositionAndUnknownDateFields) {
	Board board;
	Pgn pgn;
	ASSERT_TRUE(
		pgn.loadText("[SetUp \"1\"]\n[FEN \"8/8/8/8/8/8/4K3/7k w - - 0 1\"]\n"
					 "[Date \"2026.??.??\"]\n1.Ke3 *",
					 board));
	EXPECT_EQ(pgn.year(), 2026);
	EXPECT_EQ(pgn.month(), -1);
	EXPECT_EQ(pgn.day(), -1);
	EXPECT_EQ(board.pieceValue(20), king);
	EXPECT_EQ(pgn.moveCount(), 1);
}

} // namespace
} // namespace morphy_chess
