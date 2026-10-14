#pragma once
#include <hpx/modules/program_options.hpp>
#include <morphychess/params.hpp>
#include <string>
namespace morphy_chess {
class CommandLineOptions {
public:
	explicit CommandLineOptions(
		const hpx::program_options::variables_map &values);
	static hpx::program_options::options_description description();
	int workers() const { return searchWorkers; }
	int depth() const { return searchDepth; }
	double benchmarkSeconds() const { return secondsPerPosition; }
	bool benchmark() const { return runBenchmark; }
	bool version() const { return showVersion; }
	bool useBook() const { return bookEnabled; }
	const std::string &bookPath() const { return openingBookPath; }

private:
	int searchWorkers;
	int searchDepth;
	double secondsPerPosition;
	bool runBenchmark;
	bool showVersion;
	bool bookEnabled;
	std::string openingBookPath;
};
} // namespace morphy_chess
