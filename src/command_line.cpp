#include <cmath>
#include <stdexcept>

#include <morphychess/command_line.hpp>

namespace morphy_chess {
hpx::program_options::options_description CommandLineOptions::description() {
	namespace po = hpx::program_options;
	po::options_description options("MorphyChess options");
	options.add_options()("bench,b", po::bool_switch()->default_value(false),
						  "Run the ten-position benchmark")(
		"threads,t", po::value<int>()->default_value(defaultThreadCount),
		"Number of search tasks (HPX workers: --hpx:threads)")(
		"depth,d", po::value<int>()->default_value(kMaxSearchDepth),
		"Maximum benchmark depth")(
		"bench-seconds",
		po::value<double>()->default_value(double(kBenchTime) / kNumBenchmarks),
		"Seconds per benchmark position; zero means depth only")(
		"book", po::value<std::string>()->default_value(kBookName),
		"Opening-book filename")("no-book",
								 po::bool_switch()->default_value(false),
								 "Disable the opening book")(
		"version", po::bool_switch()->default_value(false),
		"Show engine version");
	return options;
}
CommandLineOptions::CommandLineOptions(
	const hpx::program_options::variables_map &values) :
	searchWorkers(values["threads"].as<int>()),
	searchDepth(values["depth"].as<int>()),
	secondsPerPosition(values["bench-seconds"].as<double>()),
	runBenchmark(values["bench"].as<bool>()),
	showVersion(values["version"].as<bool>()),
	bookEnabled(!values["no-book"].as<bool>()),
	openingBookPath(values["book"].as<std::string>()) {
	// Reject invalid dimensions and nonfinite time limits before allocating
	// caches or constructing asynchronous search tasks.
	if (searchWorkers < 1 || searchWorkers > kTableSize / 8) {
		throw std::invalid_argument(
			"--threads must be positive and fit the transposition table");
	}
	if (searchDepth < 1 || searchDepth > kMaxSearchDepth) {
		throw std::invalid_argument("--depth must be between 1 and " +
									std::to_string(kMaxSearchDepth));
	}
	if (!std::isfinite(secondsPerPosition) || secondsPerPosition < 0) {
		throw std::invalid_argument(
			"--bench-seconds must be finite and nonnegative");
	}
	if (openingBookPath.empty() && bookEnabled) {
		throw std::invalid_argument(
			"--book must name a file when the opening book is enabled");
	}
}
} // namespace morphy_chess
