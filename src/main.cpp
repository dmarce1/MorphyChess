#include <exception>
#include <hpx/init_runtime_local/init_runtime_local.hpp>
#include <hpx/runtime_local/config_entry.hpp>
#include <iostream>
#include <morphychess/bench.hpp>
#include <morphychess/command_line.hpp>
#include <morphychess/xboard.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
int runApplication(hpx::program_options::variables_map &values) {
	int status = 0;
	try {
		const auto unknown = hpx::get_config_entry("hpx.unknown_cmd_line", "");
		if (!unknown.empty() || values.count("hpx:positional")) {
			throw std::invalid_argument(
				"Unrecognized command-line arguments: " + unknown);
		}
		const morphy_chess::CommandLineOptions options(values);
		if (options.version()) {
			std::cout << morphy_chess::kName << " (C++23, HPX local runtime)\n";
		} else if (options.benchmark()) {
			status = morphy_chess::runBenchmark(options, std::cout);
		} else {
			morphy_chess::Xboard interface(options.workers(), options.useBook(),
										   options.bookPath());
			status = interface.run();
		}
	} catch (const std::exception &error) {
		std::cerr << "MorphyChess: " << error.what() << '\n';
		status = 1;
	}
	hpx::local::finalize();
	return status;
}
} // namespace
int main(int argc, char **argv) {
	// Retain the historical spelling while all option parsing belongs to HPX.
	std::vector<std::string> arguments;
	std::vector<char *> pointers;
	for (int index = 0; index < argc; ++index) {
		std::string argument(argv[index]);
		if (argument == "-bench") {
			argument = "--bench";
		} else if (argument == "--help" || argument == "-h") {
			argument = "--hpx:help";
		}
		arguments.push_back(std::move(argument));
	}
	for (auto &argument : arguments) {
		pointers.push_back(argument.data());
	}
	pointers.push_back(nullptr);
	const auto description = morphy_chess::CommandLineOptions::description();
	hpx::local::init_params parameters;
	parameters.desc_cmdline = description;
	parameters.cfg = {
		"hpx.os_threads=" + std::to_string(morphy_chess::defaultThreadCount),
		"hpx.stacks.small_size=0x400000", "hpx.commandline.allow_unknown=0"};
	try {
		return hpx::local::init(&runApplication, argc, pointers.data(),
								parameters);
	} catch (const std::exception &error) {
		std::cerr << "MorphyChess: " << error.what() << '\n';
		return 1;
	}
}
