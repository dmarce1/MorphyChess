#include <morphychess/history.hpp>
#include <morphychess/node.hpp>
#include <morphychess/search.hpp>
#include <morphychess/static_evaluator.hpp>

#include <chrono>
#include <gtest/gtest.h>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/threading.hpp>
#include <limits>
#include <set>
#include <utility>

namespace morphy_chess {
namespace {
class SearchTest : public ::testing::Test {
protected:
	Board board;
	StaticEvaluator evaluator;
	MemoryTable table{4096, 16, 2};
	Search search{&board, &evaluator, &table};
};

TEST(HistoryTest, StartsEmptyAndRecordsQuietCutoffs) {
	History history;
	const Move first(12, 28);
	const Move second(11, 27);
	EXPECT_EQ(history.moveHistory(first), 0);
	EXPECT_TRUE(history.killer1(0).isEmpty());
	history.update(first, true, 3, 0, true);
	EXPECT_GT(history.moveHistory(first), 0);
	EXPECT_EQ(history.killer1(0), first);
	history.update(second, true, 2, 0, true);
	EXPECT_EQ(history.killer1(0), second);
	EXPECT_EQ(history.killer2(0), first);
	history.reset();
	EXPECT_EQ(history.moveHistory(first), 0);
	EXPECT_TRUE(history.killer1(0).isEmpty());
}

TEST(HistoryTest, IgnoresFailuresAndCaptures) {
	History history;
	const Move move(12, 28);
	history.update(move, false, 3, 0, true);
	history.update(move, true, 3, 0, false);
	history.update(Move{}, true, 3, 0, true);
	EXPECT_EQ(history.moveHistory(move), 0);
	EXPECT_TRUE(history.killer1(0).isEmpty());
}

TEST(HistoryTest, DeepUpdatesDoNotOverflow) {
	History history;
	const Move move(12, 28);
	for (int depth = 1; depth < kMaxDepth; ++depth) {
		for (int repetition = 0; repetition < 8; ++repetition) {
			history.update(move, true, depth, 0, true);
			EXPECT_GT(history.moveHistory(move), 0);
		}
	}
	EXPECT_THROW(History(0.0), std::invalid_argument);
	EXPECT_THROW(History(std::numeric_limits<double>::infinity()),
				 std::invalid_argument);
}

TEST_F(SearchTest, DefaultEvaluatorBalancesStartingPosition) {
	EXPECT_EQ(evaluator.materialBalance(&board), 0);
	EXPECT_TRUE(evaluator.test(&board, -4));
	EXPECT_FALSE(evaluator.test(&board, 4));
	EXPECT_EQ(evaluator.grainSize(), kDefaultGrainSize);
	EXPECT_GT(evaluator.checkmateValue(0), evaluator.checkmateValue(5));
	EXPECT_EQ(evaluator.drawValue(), 0);
}

TEST_F(SearchTest, MaterialAdvantageChangesWithSideToMove) {
	ASSERT_TRUE(board.loadFen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1"));
	const int whiteScore = evaluator.materialBalance(&board);
	EXPECT_GT(whiteScore, 0);
	ASSERT_TRUE(board.loadFen("4k3/8/8/8/8/8/8/3QK3 b - - 0 1"));
	EXPECT_EQ(evaluator.materialBalance(&board), -whiteScore);
}

TEST_F(SearchTest, BishopSquareTableIsColorSymmetric) {
	ASSERT_TRUE(board.loadFen("4k2b/8/8/8/8/8/8/4K2B w - - 0 1"));
	EXPECT_EQ(evaluator.nonMaterialScore(&board, 325, 325), 0);
	evaluator.resetPawnTable();
	EXPECT_EQ(evaluator.nonMaterialScore(&board, 325, 325), 0);
}

TEST_F(SearchTest, ReconfiguringEvaluationClearsPawnCache) {
	ASSERT_TRUE(board.loadFen("4k3/8/8/8/2P5/8/8/4K3 w - - 0 1"));
	const int before = evaluator.nonMaterialScore(&board, 100, 0);
	auto params = defaultEvaluatorParameters;
	params[9] = 40;
	evaluator.configure(params);
	EXPECT_GT(evaluator.nonMaterialScore(&board, 100, 0), before);
}

TEST_F(SearchTest, NodeEnumeratesEveryStartingMoveAndRestoresBoard) {
	const auto key = board.positionKey();
	const auto ply = board.halfMoveCount();
	Node node(&search, Move{});
	Move move;
	std::set<std::pair<int, int>> moves;
	while (node.doNextMove(&move) == kNodeMovesAvailable) {
		moves.emplace(move.from(), move.to());
		node.restore();
		EXPECT_EQ(board.positionKey(), key);
		EXPECT_EQ(board.halfMoveCount(), ply);
	}
	EXPECT_EQ(moves.size(), 20u);
	EXPECT_EQ(node.totalMoves(), 20);
	EXPECT_FALSE(node.noMovesGenerated());
}

TEST_F(SearchTest, QuiescentNodeGeneratesCapturesOnly) {
	ASSERT_TRUE(board.loadFen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1"));
	const auto key = board.positionKey();
	Node node(&search, Move{});
	Move move;
	ASSERT_EQ(node.doNextNonquiescentMove(&move), kNodeMovesAvailable);
	EXPECT_EQ(move, Move(28, 35));
	node.restore();
	EXPECT_EQ(node.doNextNonquiescentMove(&move), kNodeNoMoreMoves);
	EXPECT_EQ(board.positionKey(), key);
}

TEST(MemoryTableTest, RejectsInvalidDimensions) {
	EXPECT_THROW(MemoryTable(0, 1, 1), std::invalid_argument);
	EXPECT_THROW(MemoryTable(64, 0, 1), std::invalid_argument);
	EXPECT_THROW(MemoryTable(64, 1, 0), std::invalid_argument);
	EXPECT_THROW(MemoryTable(8, 1, 2), std::invalid_argument);
}

TEST_F(SearchTest, TranspositionBoundsHonorDepthAndReset) {
	Move move;
	int lower = 0;
	int upper = 0;
	EXPECT_FALSE(table.read(search, &move, &lower, &upper, 1, 0));
	EXPECT_TRUE(move.isEmpty());
	EXPECT_EQ(lower, -kInfty);
	EXPECT_EQ(upper, kInfty);
	const Move stored(12, 28);
	table.write(search, stored, true, 12, 3);
	ASSERT_TRUE(table.read(search, &move, &lower, &upper, 3, 12));
	EXPECT_EQ(move, stored);
	EXPECT_EQ(lower, 16);
	EXPECT_EQ(upper, kInfty);
	EXPECT_FALSE(table.read(search, &move, &lower, &upper, 4, 12));
	table.write(search, stored, false, 20, 3);
	ASSERT_TRUE(table.read(search, &move, &lower, &upper, 3, 16));
	EXPECT_EQ(lower, 16);
	EXPECT_EQ(upper, 16);
	table.reset();
	EXPECT_FALSE(table.read(search, &move, &lower, &upper, 1, 0));
}

TEST_F(SearchTest, TranspositionEntriesAreSharedAcrossWorkers) {
	Search second(&board, &evaluator, &table);
	second.setThreadNumber(1);
	table.write(search, Move(12, 28), true, 12, 3);
	Move move;
	int lower, upper;
	ASSERT_TRUE(table.read(second, &move, &lower, &upper, 3, 12));
	EXPECT_EQ(move, Move(12, 28));
	EXPECT_EQ(lower, 16);
}

TEST_F(SearchTest, TranspositionPrunesOnlyUntouchedEntries) {
	const Move stored(12, 28);
	Move move;
	int lower, upper;
	table.write(search, stored, true, 12, 3);
	table.resetTouchedFlags();
	ASSERT_TRUE(table.read(search, &move, &lower, &upper, 3, 0));
	table.killUntouchedEntries();
	EXPECT_TRUE(table.read(search, &move, &lower, &upper, 3, 0));
	table.resetTouchedFlags();
	table.killUntouchedEntries();
	EXPECT_FALSE(table.read(search, &move, &lower, &upper, 3, 0));
	table.write(search, stored, true, kMinCheckmateValue, 3);
	EXPECT_FALSE(table.read(search, &move, &lower, &upper, 3, 0));
}

TEST_F(SearchTest, SerialSearchReturnsLegalMoveAndRestoresBoard) {
	const auto key = board.positionKey();
	const auto ply = board.halfMoveCount();
	Move move;
	const int score = search.serialTestDriver(&move, 2, 0);
	EXPECT_LT(std::abs(score), kMinCheckmateValue);
	EXPECT_FALSE(move.isEmpty());
	EXPECT_TRUE(board.isLegalMove(move));
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_EQ(board.halfMoveCount(), ply);
	EXPECT_GT(search.nodeCount(), 0);
}

TEST_F(SearchTest, TerminalSearchRootHasNoMove) {
	ASSERT_TRUE(board.loadFen("7k/6Q1/5K2/8/8/8/8/8 b - - 0 1"));
	EXPECT_FALSE(search.test(0, 2));
	EXPECT_FALSE(search.quiescentTest(-kMinCheckmateValue));
	EXPECT_TRUE(search.bestMove().isEmpty());
	EXPECT_GT(search.quiescentValue(0), kMinCheckmateValue);
	ASSERT_TRUE(board.loadFen("8/8/8/8/8/5k2/6q1/7K w - - 0 1"));
	EXPECT_LT(search.quiescentValue(0), -kMinCheckmateValue);
}

TEST_F(SearchTest, QuiescentCountsEveryLeafExactlyOnce) {
	// Stand-pat leaves and quiet nodes without captures both represent one
	// visit.
	EXPECT_TRUE(search.quiescentTest(-4));
	EXPECT_EQ(search.nodeCount(), 1);
	search.resetCounters();
	EXPECT_FALSE(search.quiescentTest(4));
	EXPECT_EQ(search.nodeCount(), 1);

	// A drawn position also visits a leaf even though move generation is
	// skipped.
	ASSERT_TRUE(board.loadFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1"));
	search.resetCounters();
	EXPECT_TRUE(search.quiescentTest(-4));
	EXPECT_EQ(search.nodeCount(), 1);
}

TEST_F(SearchTest, QuiescentEvaluationRestoresCaptureSequence) {
	ASSERT_TRUE(board.loadFen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1"));
	const auto key = board.positionKey();
	const auto ply = board.halfMoveCount();
	EXPECT_GT(search.quiescentValue(0), 0);
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_EQ(board.halfMoveCount(), ply);
}

TEST_F(SearchTest, StopAndExpiredDeadlineLeaveBoardUnchanged) {
	const auto key = board.positionKey();
	search.requestStop();
	EXPECT_FALSE(search.test(-4, 3));
	EXPECT_TRUE(search.stopped());
	EXPECT_TRUE(search.bestMove().isEmpty());
	EXPECT_EQ(board.positionKey(), key);
	search.clearStop();
	search.setKillTime(std::time(nullptr) - 1);
	EXPECT_FALSE(search.test(-4, 3));
	EXPECT_TRUE(search.stopped());
	EXPECT_EQ(board.positionKey(), key);
	search.clearStop();
	search.setKillTime(0);
	EXPECT_TRUE(search.test(-kMinCheckmateValue, 1));
	EXPECT_FALSE(search.stopped());
}

TEST_F(SearchTest, RunningHpxSearchCanBeCancelledAndJoined) {
	const auto key = board.positionKey();
	const auto ply = board.halfMoveCount();
	auto work = hpx::async([&] { return search.test(-4, 16); });
	hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
	EXPECT_FALSE(work.is_ready());
	search.requestStop();
	work.get();
	EXPECT_TRUE(search.stopped());
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_EQ(board.halfMoveCount(), ply);
}
} // namespace
} // namespace morphy_chess
