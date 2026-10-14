#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <hpx/modules/futures.hpp>
#include <string>
namespace morphy_chess {
class SeekBot {
public:
	SeekBot() = default;
	~SeekBot();
	SeekBot(const SeekBot &) = delete;
	SeekBot &operator=(const SeekBot &) = delete;
	void start(std::function<void(const std::string &)> output,
			   std::chrono::milliseconds interval = std::chrono::seconds(5));
	void stop();
	bool running() const { return active.load(); }

private:
	std::atomic<bool> stopRequested{false};
	std::atomic<bool> active{false};
	hpx::future<void> pending;
};
} // namespace morphy_chess
