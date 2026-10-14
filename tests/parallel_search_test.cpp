#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <hpx/modules/threading.hpp>

#include <morphychess/parallel_search.hpp>

namespace morphy_chess {
namespace {
class ParallelSearchTest : public ::testing::Test {
protected:
	Board board;
	StaticEvaluator evaluator;
	MemoryTable table{32768, 32, 2};
};

TEST_F(ParallelSearchTest, OneAndTwoWorkersReturnLegalCompletedSearches) {
	for (int const workers : {1, 2}) {
		ParallelSearch engine(board, evaluator, table, workers);
		engine.setUseBook(false);
		auto const key = board.positionKey();
		int const ply = board.halfMoveCount();
		SearchResult const result = engine.run(3);
		EXPECT_EQ(result.depth(), 3);
		EXPECT_FALSE(result.move().isEmpty());
		EXPECT_TRUE(board.isLegalMove(result.move()));
		EXPECT_GT(result.nodes(), 0);
		EXPECT_EQ(board.positionKey(), key);
		EXPECT_EQ(board.halfMoveCount(), ply);
		EXPECT_FALSE(engine.running());
		table.reset();
	}
}

TEST_F(ParallelSearchTest, DepthOneCompletesAndReportsProgress) {
	ParallelSearch engine(board, evaluator, table, 2);
	engine.setUseBook(false);
	std::vector<SearchResult> progress;
	engine.setProgressHandler(
		[&](SearchResult const &result) { progress.push_back(result); });
	SearchResult const result = engine.run(1);
	ASSERT_EQ(progress.size(), 1u);
	EXPECT_EQ(progress.front().depth(), 1);
	EXPECT_EQ(result.depth(), 1);
	EXPECT_TRUE(board.isLegalMove(result.move()));
}

TEST_F(ParallelSearchTest, StopBeforeAnyStartIsHarmless) {
	ParallelSearch engine(board, evaluator, table, 1);
	EXPECT_TRUE(engine.stop().move().isEmpty());
	EXPECT_TRUE(engine.finish().move().isEmpty());
	EXPECT_FALSE(engine.running());
	EXPECT_FALSE(engine.ready());
}

TEST_F(ParallelSearchTest, ImmediateStopJoinsEvenBeforeBackgroundTaskStarts) {
	ParallelSearch engine(board, evaluator, table, 2);
	engine.setUseBook(false);
	auto const key = board.positionKey();
	engine.start(16);
	SearchResult const result = engine.stop();
	EXPECT_FALSE(engine.running());
	EXPECT_FALSE(engine.ready());
	EXPECT_TRUE(board.isLegalMove(result.move()));
	EXPECT_EQ(board.positionKey(), key);
}

TEST_F(ParallelSearchTest, StopDuringSearchJoinsAllWorkers) {
	ParallelSearch engine(board, evaluator, table, 2);
	engine.setUseBook(false);
	auto const key = board.positionKey();
	int const ply = board.halfMoveCount();
	engine.start(16);
	hpx::this_thread::sleep_for(std::chrono::milliseconds(5));
	EXPECT_TRUE(engine.running());
	SearchResult const result = engine.stop();
	EXPECT_FALSE(engine.running());
	EXPECT_TRUE(board.isLegalMove(result.move()));
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_EQ(board.halfMoveCount(), ply);
	EXPECT_EQ(engine.run(1).depth(), 1);
}

TEST_F(ParallelSearchTest,
	   RepeatedStartStopsPreviousWorkBeforeReplacingItsFuture) {
	ParallelSearch engine(board, evaluator, table, 2);
	engine.setUseBook(false);
	auto const key = board.positionKey();
	for (int repetition = 0; repetition < 5; ++repetition) {
		engine.start(16);
		engine.start(1);
		SearchResult const result = engine.finish();
		EXPECT_EQ(result.depth(), 1);
		EXPECT_TRUE(board.isLegalMove(result.move()));
		EXPECT_FALSE(engine.running());
		EXPECT_EQ(board.positionKey(), key);
	}
}

TEST_F(ParallelSearchTest, ReadyBecomesTrueBeforeFinishConsumesTheFuture) {
	ParallelSearch engine(board, evaluator, table, 1);
	engine.setUseBook(false);
	engine.start(1);
	for (int attempt = 0; attempt < 1000 && !engine.ready(); ++attempt) {
		hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(engine.ready());
	EXPECT_EQ(engine.finish().depth(), 1);
	EXPECT_FALSE(engine.ready());
	EXPECT_FALSE(engine.running());
}

TEST_F(ParallelSearchTest, SubsecondLimitInterruptsAnActiveSearch) {
	ParallelSearch engine(board, evaluator, table, 2);
	engine.setUseBook(false);
	auto const start = std::chrono::steady_clock::now();
	SearchResult const result = engine.run(16, 0.02);
	auto const elapsed = std::chrono::steady_clock::now() - start;
	EXPECT_LT(elapsed, std::chrono::seconds(1));
	EXPECT_LT(result.depth(), 16);
	EXPECT_TRUE(board.isLegalMove(result.move()));
	EXPECT_FALSE(engine.running());
}

TEST_F(ParallelSearchTest, FastSearchDoesNotWaitForItsLongTimer) {
	ParallelSearch engine(board, evaluator, table, 1);
	engine.setUseBook(false);
	auto const start = std::chrono::steady_clock::now();
	EXPECT_EQ(engine.run(1, 60.0).depth(), 1);
	EXPECT_LT(std::chrono::steady_clock::now() - start,
			  std::chrono::seconds(1));
}

TEST_F(ParallelSearchTest, DestructorJoinsOutstandingWork) {
	auto const key = board.positionKey();
	{
		ParallelSearch engine(board, evaluator, table, 2);
		engine.setUseBook(false);
		engine.start(16);
		hpx::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	EXPECT_EQ(board.positionKey(), key);
	table.reset();
}

TEST_F(ParallelSearchTest, TerminalRootReturnsNoMove) {
	ParallelSearch engine(board, evaluator, table, 2);
	ASSERT_TRUE(board.loadFen("7k/6Q1/5K2/8/8/8/8/8 b - - 0 1"));
	EXPECT_TRUE(engine.run(2).move().isEmpty());
	EXPECT_FALSE(engine.running());
	ASSERT_TRUE(board.loadFen("7k/5Q2/5K2/8/8/8/8/8 b - - 0 1"));
	engine.start(2);
	EXPECT_TRUE(engine.finish().move().isEmpty());
	EXPECT_FALSE(engine.running());
}

TEST_F(ParallelSearchTest, ProgressExceptionCancelsJoinsAndAllowsReuse) {
	ParallelSearch engine(board, evaluator, table, 2);
	engine.setUseBook(false);
	engine.setProgressHandler([](SearchResult const &) {
		throw std::runtime_error("test progress failure");
	});
	EXPECT_THROW(engine.run(16), std::runtime_error);
	EXPECT_FALSE(engine.running());
	engine.start(16);
	EXPECT_THROW(engine.finish(), std::runtime_error);
	EXPECT_FALSE(engine.running());
	engine.setProgressHandler({});
	EXPECT_EQ(engine.run(1).depth(), 1);
}

TEST_F(ParallelSearchTest, WorkerExceptionCancelsAllOtherTasks) {
	MemoryTable undersizedTable(4096, 16, 1);
	ParallelSearch engine(board, evaluator, undersizedTable, 2);
	engine.setUseBook(false);
	EXPECT_THROW(engine.run(16), std::out_of_range);
	EXPECT_FALSE(engine.running());
}

TEST_F(ParallelSearchTest, ChangedDrawPreferencesInvalidateCachedBounds) {
	ASSERT_TRUE(board.loadFen("4k3/8/8/8/8/8/8/R3K3 w - - 99 1"));
	ParallelSearch engine(board, evaluator, table, 1);
	engine.setUseBook(false);
	// Every legal move reaches the fifty-move draw threshold.
	EXPECT_EQ(engine.run(1).score(), 0);
	engine.setContemptFactor(400);
	EXPECT_EQ(engine.run(1).score(), 400);
	engine.setComputerColor(black);
	EXPECT_EQ(engine.run(1).score(), -400);
	engine.setContemptFactor(0);
	EXPECT_EQ(engine.run(1).score(), 0);
}

TEST_F(ParallelSearchTest, RejectsInvalidSearchArguments) {
	EXPECT_THROW(ParallelSearch(board, evaluator, table, 0),
				 std::invalid_argument);
	ParallelSearch engine(board, evaluator, table, 1);
	EXPECT_THROW(engine.run(0), std::invalid_argument);
	EXPECT_THROW(engine.run(kMaxSearchDepth + 1), std::invalid_argument);
	EXPECT_THROW(engine.run(1, -1.0), std::invalid_argument);
	EXPECT_THROW(engine.run(1, std::numeric_limits<double>::infinity()),
				 std::invalid_argument);
	EXPECT_THROW(engine.run(1, std::numeric_limits<double>::max()),
				 std::invalid_argument);
	EXPECT_THROW(engine.run(1, 0.0, std::numeric_limits<int>::max()),
				 std::invalid_argument);
	EXPECT_FALSE(engine.running());
}

TEST_F(ParallelSearchTest,
	   OpeningBookRouteReturnsItsMoveWithoutLaunchingSearch) {
	auto const path =
		std::filesystem::temp_directory_path() /
		("morphychess_parallel_book_" +
		 std::to_string(
			 std::chrono::steady_clock::now().time_since_epoch().count()) +
		 ".pgn");
	{
		std::ofstream output(path);
		output << "[Event \"Single opening\"]\n[Result \"*\"]\n\n1. e4 *\n";
	}
	ParallelSearch engine(board, evaluator, table, 2);
	int const count = engine.openingBook().digest(path.string());
	std::filesystem::remove(path);
	ASSERT_EQ(count, 1);
	auto const key = board.positionKey();
	SearchResult const result = engine.run(16);
	EXPECT_EQ(result.move(), Move(12, 28));
	EXPECT_EQ(result.depth(), 0);
	EXPECT_EQ(result.nodes(), 0);
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_FALSE(engine.running());
}
} // namespace
} // namespace morphy_chess
