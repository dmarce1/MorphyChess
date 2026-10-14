#pragma once
#include <iosfwd>
#include <morphychess/command_line.hpp>
namespace morphy_chess {
int runBenchmark(const CommandLineOptions &options, std::ostream &output);
}
