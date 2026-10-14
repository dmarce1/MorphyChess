#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace morphy_chess {
using U32 = std::uint32_t;
using U16 = std::uint16_t;
using U8 = std::uint8_t;
using I64 = long long;
using I32 = std::int32_t;
using I16 = std::int16_t;
using I8 = std::int8_t;
template <class T, class U> constexpr auto maximum(T a, U b) {
	return a > b ? a : b;
}
template <class T, class U> constexpr auto minimum(T a, U b) {
	return a < b ? a : b;
}
template <class T, class U, class V>
constexpr auto restrictRange(T a, U value, V b) {
	// Inclusive clamp; callers supply ordered bounds and compatible types.
	return minimum(maximum(a, value), b);
}
} // namespace morphy_chess
