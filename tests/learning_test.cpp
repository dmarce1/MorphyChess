#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <limits>
#include <morphychess/learning.hpp>
#include <morphychess/parallel_search.hpp>
#include <stdexcept>

namespace morphy_chess {
namespace {
class LearningDirectory {
public:
	LearningDirectory() :
		previous(std::filesystem::current_path()) {
		static std::atomic<unsigned> sequence{0};
		path =
			std::filesystem::temp_directory_path() /
			("morphychess_learning_" +
			 std::to_string(
				 std::chrono::steady_clock::now().time_since_epoch().count()) +
			 "_" + std::to_string(sequence++));
		std::filesystem::create_directory(path);
		std::filesystem::current_path(path);
	}
	~LearningDirectory() {
		std::filesystem::current_path(previous);
		std::error_code ignored;
		std::filesystem::remove_all(path, ignored);
	}

private:
	std::filesystem::path previous;
	std::filesystem::path path;
};
} // namespace
TEST(Learning, TemporalDifferenceLimitsAndDiscounting) {
	const std::vector<double> predictions{0.2, 0.4, 0.3};
	const auto oneStep = temporalDifferenceWeights(predictions, 1.0, 0.0);
	EXPECT_NEAR(oneStep[0], 0.2, 1e-14);
	EXPECT_NEAR(oneStep[1], -0.1, 1e-14);
	EXPECT_NEAR(oneStep[2], 0.7, 1e-14);
	const auto monteCarlo = temporalDifferenceWeights(predictions, 1.0, 1.0);
	EXPECT_NEAR(monteCarlo[0], 0.8, 1e-14);
	EXPECT_NEAR(monteCarlo[1], 0.6, 1e-14);
	EXPECT_NEAR(monteCarlo[2], 0.7, 1e-14);
	const auto discounted = temporalDifferenceWeights(predictions, 1.0, 0.5);
	EXPECT_NEAR(discounted[0], 0.325, 1e-14);
	EXPECT_NEAR(discounted[1], 0.25, 1e-14);
	EXPECT_NEAR(discounted[2], 0.7, 1e-14);
	EXPECT_TRUE(temporalDifferenceWeights({}, 1.0, 0.7).empty());
}
TEST(Learning, RejectsInvalidInputsAndOverflow) {
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();
	EXPECT_THROW(temporalDifferenceWeights({0.0}, 1.0, -0.1),
				 std::invalid_argument);
	EXPECT_THROW(temporalDifferenceWeights({0.0}, 1.0, 1.1),
				 std::invalid_argument);
	EXPECT_THROW(temporalDifferenceWeights({0.0}, nan, 0.5),
				 std::invalid_argument);
	EXPECT_THROW(temporalDifferenceWeights({inf}, 1.0, 0.5),
				 std::invalid_argument);
	EXPECT_THROW(temporalDifferenceWeights({0.0}, 1.0, nan),
				 std::invalid_argument);
	const double largest = std::numeric_limits<double>::max();
	EXPECT_THROW(temporalDifferenceWeights({-largest}, largest, 0.5),
				 std::overflow_error);
}
TEST(Learning, InvalidEvaluatorParametersLeaveConfigurationIntact) {
	Board board;
	ASSERT_TRUE(board.loadFen("7k/8/8/8/8/8/Q7/K7 w - - 0 1"));
	StaticEvaluator evaluator;
	const int before = evaluator.materialBalance(board);
	for (double invalid :
		 {0.0, -1.0, 10001.0, std::numeric_limits<double>::infinity(),
		  std::numeric_limits<double>::quiet_NaN()}) {
		auto parameters = defaultEvaluatorParameters;
		parameters[0] = invalid;
		EXPECT_THROW(evaluator.configure(parameters), std::invalid_argument);
		EXPECT_EQ(evaluator.materialBalance(board), before);
	}
}
TEST(Learning, CommitsFiniteParametersAndKeepsStateWhenSavingFails) {
	LearningDirectory directory;
	Board board;
	StaticEvaluator evaluator;
	MemoryTable table(4096, 16, 1);
	ParallelSearch search(board, evaluator, table, 1);
	search.setUseBook(false);
	search.setLearning(true);
	ASSERT_FALSE(search.run(1).move().isEmpty());
	const auto original = search.parameters();
	std::filesystem::create_directory("brain.dat");
	EXPECT_THROW(search.triggerLearning(white, whiteWin),
				 std::filesystem::filesystem_error);
	EXPECT_FALSE(std::filesystem::exists("brain.dat.tmp"));
	for (size_t index = 0; index < original.size(); ++index) {
		EXPECT_EQ(search.parameters()[index], original[index]);
	}
	std::filesystem::remove("brain.dat");
	EXPECT_NO_THROW(search.triggerLearning(white, whiteWin));
	EXPECT_EQ(std::filesystem::file_size("brain.dat"),
			  sizeof(double) * original.size());
	std::ifstream file("brain.dat", std::ios::binary);
	for (size_t index = 0; index < original.size(); ++index) {
		double value = 0;
		file.read(reinterpret_cast<char *>(&value), sizeof(value));
		EXPECT_TRUE(std::isfinite(value));
		EXPECT_DOUBLE_EQ(value, search.parameters()[index]);
	}
	EXPECT_TRUE(file.good());
}
TEST(Learning, DisabledAndEmptyHistoryDoNotWriteFiles) {
	LearningDirectory directory;
	Board board;
	StaticEvaluator evaluator;
	MemoryTable table(1024, 4, 1);
	ParallelSearch search(board, evaluator, table, 1);
	search.triggerLearning(white, whiteWin);
	search.setLearning(true);
	search.triggerLearning(white, whiteWin);
	EXPECT_FALSE(std::filesystem::exists("brain.dat"));
	EXPECT_FALSE(std::filesystem::exists("history.txt"));
}
} // namespace morphy_chess
