#pragma once
#include <cassert>
#include <morphychess/board.hpp>
#include <source_location>
namespace morphy_chess {
#ifdef NDEBUG
inline constexpr bool debugMode = false;
#else
inline constexpr bool debugMode = true;
#endif
void assertCondition(bool condition, const std::source_location &location =
										 std::source_location::current());
void assertBoardConsistency(
	const Board &board,
	const std::source_location &location = std::source_location::current());
void assertTranspositionKeyConsistency(
	const Board &board,
	const std::source_location &location = std::source_location::current());
void debugRegisterBoard(const Board *board);
template <class L, class V, class H>
inline void assertRange([[maybe_unused]] L low, [[maybe_unused]] V value,
						[[maybe_unused]] H high) {
	assert(value >= low && value <= high);
}
} // namespace morphy_chess
