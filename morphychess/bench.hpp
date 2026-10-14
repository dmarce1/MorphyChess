#pragma once
#include <iosfwd>

#include <morphychess/command_line.hpp>

namespace morphy_chess {
// Fixed ten-position search smoke/performance run; this is not a strength test.
int runBenchmark(const CommandLineOptions &options, std::ostream &output);
} // namespace morphy_chess
