#include <algorithm>
#include <random>
#include <stdexcept>

#include <hpx/modules/async_local.hpp>
#include <hpx/modules/threading.hpp>

#include <morphychess/seek_bot.hpp>

namespace morphy_chess {
SeekBot::~SeekBot() {
	try {
		stop();
	} catch (...) {
	}
}
void SeekBot::start(std::function<void(std::string const &)> output,
					std::chrono::milliseconds interval) {
	if (!output || interval.count() <= 0) {
		throw std::invalid_argument(
			"Seek bot requires output and a positive interval");
	}
	stop();
	stopRequested.store(false);
	active.store(true);
	try {
		pending = hpx::async([this, output = std::move(output), interval] {
			try {
				std::mt19937 random(std::random_device{}());
				std::uniform_int_distribution<int> timeControl(1, 18);
				while (!stopRequested.load()) {
					output("tellics seek " +
						   std::to_string(timeControl(random)) + " " +
						   std::to_string(timeControl(random)) + "\n");
					auto const next =
						std::chrono::steady_clock::now() + interval;
					while (!stopRequested.load() &&
						   std::chrono::steady_clock::now() < next) {
						// Bound cancellation latency independently of the seek
						// interval, yielding the HPX worker between checks.
						hpx::this_thread::sleep_for(
							std::chrono::milliseconds(10));
					}
				}
			} catch (...) {
				active.store(false);
				throw;
			}
			active.store(false);
		});
	} catch (...) {
		active.store(false);
		throw;
	}
}
void SeekBot::stop() {
	stopRequested.store(true);
	if (pending.valid()) {
		pending.get();
	}
}
} // namespace morphy_chess
