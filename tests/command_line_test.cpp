#include <vector>

#include <gtest/gtest.h>

#include <morphychess/command_line.hpp>

namespace morphy_chess {
namespace {
CommandLineOptions parse(std::vector<std::string> const &arguments) {
	hpx::program_options::variables_map values;
	auto const description = CommandLineOptions::description();
	hpx::program_options::store(
		hpx::program_options::command_line_parser(arguments)
			.options(description)
			.run(),
		values);
	hpx::program_options::notify(values);
	return CommandLineOptions(values);
}
} // namespace
TEST(CommandLineTest, DefaultsMatchEngine) {
	auto const options = parse({});
	EXPECT_EQ(options.workers(), defaultThreadCount);
	EXPECT_EQ(options.depth(), kMaxSearchDepth);
	EXPECT_FALSE(options.benchmark());
	EXPECT_TRUE(options.useBook());
}
TEST(CommandLineTest, ParsesBenchmarkAndBookOptions) {
	auto const options = parse({"--bench", "--threads=2", "--depth=3",
								"--bench-seconds=0.5", "--no-book"});
	EXPECT_TRUE(options.benchmark());
	EXPECT_EQ(options.workers(), 2);
	EXPECT_EQ(options.depth(), 3);
	EXPECT_DOUBLE_EQ(options.benchmarkSeconds(), 0.5);
	EXPECT_FALSE(options.useBook());
}
TEST(CommandLineTest, RejectsInvalidOptions) {
	EXPECT_THROW(parse({"--threads=0"}), std::invalid_argument);
	EXPECT_THROW(parse({"--depth=0"}), std::invalid_argument);
	EXPECT_THROW(parse({"--depth=33"}), std::invalid_argument);
	EXPECT_THROW(parse({"--bench-seconds=-1"}), std::invalid_argument);
	EXPECT_THROW(parse({"--bench-seconds=nan"}), std::invalid_argument);
	EXPECT_THROW(parse({"--unrecognized"}), hpx::program_options::error);
}
} // namespace morphy_chess
