#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <hpx/modules/threading.hpp>
#include <morphychess/seek_bot.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace morphy_chess {
namespace {
bool waitFor(const std::function<bool()> &condition) {
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (!condition() && std::chrono::steady_clock::now() < deadline) {
		hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return condition();
}
} // namespace
TEST(SeekBot, RejectsMissingOutputAndInvalidIntervals) {
	SeekBot bot;
	EXPECT_THROW(bot.start({}), std::invalid_argument);
	EXPECT_THROW(
		bot.start([](const std::string &) {}, std::chrono::milliseconds(0)),
		std::invalid_argument);
	EXPECT_FALSE(bot.running());
	EXPECT_NO_THROW(bot.stop());
}
TEST(SeekBot, ImmediateStopWorksWithOneRuntimeWorker) {
	SeekBot bot;
	std::atomic<int> calls{0};
	const auto before = std::chrono::steady_clock::now();
	bot.start([&](const std::string &) { ++calls; }, std::chrono::hours(1));
	bot.stop();
	EXPECT_FALSE(bot.running());
	EXPECT_LT(std::chrono::steady_clock::now() - before,
			  std::chrono::seconds(1));
	const int completedCalls = calls.load();
	bot.stop();
	EXPECT_EQ(calls.load(), completedCalls);
}
TEST(SeekBot, ProducesProtocolCommandsAndCanRestart) {
	SeekBot bot;
	std::atomic<int> calls{0};
	std::string command;
	auto output = [&](const std::string &text) {
		command = text;
		++calls;
	};
	bot.start(output, std::chrono::hours(1));
	ASSERT_TRUE(waitFor([&] { return calls.load() > 0; }));
	bot.stop();
	std::istringstream stream(command);
	std::string protocol, verb;
	int minutes = 0, increment = 0;
	stream >> protocol >> verb >> minutes >> increment;
	EXPECT_EQ(protocol, "tellics");
	EXPECT_EQ(verb, "seek");
	EXPECT_GE(minutes, 1);
	EXPECT_LE(minutes, 18);
	EXPECT_GE(increment, 1);
	EXPECT_LE(increment, 18);
	EXPECT_EQ(command.back(), '\n');
	bot.start(output, std::chrono::hours(1));
	ASSERT_TRUE(waitFor([&] { return calls.load() > 1; }));
	bot.stop();
	EXPECT_FALSE(bot.running());
}
TEST(SeekBot, CallbackExceptionsAreJoinedAndPropagated) {
	SeekBot bot;
	bot.start(
		[](const std::string &) { throw std::runtime_error("output failed"); });
	ASSERT_TRUE(waitFor([&] { return !bot.running(); }));
	EXPECT_THROW(bot.stop(), std::runtime_error);
	EXPECT_NO_THROW(bot.stop());
}
TEST(SeekBot, DestructorJoinsPendingTask) {
	std::atomic<int> calls{0};
	{
		SeekBot bot;
		bot.start([&](const std::string &) { ++calls; }, std::chrono::hours(1));
		ASSERT_TRUE(waitFor([&] { return calls.load() > 0; }));
	}
	const int completedCalls = calls.load();
	hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
	EXPECT_EQ(calls.load(), completedCalls);
}
} // namespace morphy_chess
