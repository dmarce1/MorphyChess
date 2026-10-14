#pragma once
#include <cassert>
#include <source_location>

#include <morphychess/board.hpp>

namespace morphy_chess {
#ifdef NDEBUG
inline constexpr bool debugMode = false;
#else
inline constexpr bool debugMode = true;
#endif
void assertCondition(bool condition, std::source_location const &location =
										 std::source_location::current());
// Optional borrowed diagnostic board for this OS thread; clear the pointer
// before its board dies. It is not HPX task-local across cooperative yields.
void debugRegisterBoard(Board const *board);
template <class L, class V, class H>
inline void assertRange([[maybe_unused]] L low, [[maybe_unused]] V value,
						[[maybe_unused]] H high) {
	assert(value >= low && value <= high);
}
} // namespace morphy_chess
