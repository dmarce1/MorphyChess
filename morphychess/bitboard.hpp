#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace morphy_chess {

// One bit per square: a1 is bit 0, h1 bit 7, and h8 bit 63. Each rank
// occupies one byte, so a shift by eight moves one rank without file wrap.
class Bitboard {
public:
	constexpr Bitboard() noexcept = default;
	explicit constexpr Bitboard(std::uint64_t value) noexcept :
		bits(value) {}

	[[nodiscard]] constexpr std::uint64_t value() const noexcept {
		return bits;
	}
	explicit constexpr operator bool() const noexcept { return bits != 0; }

	[[nodiscard]] constexpr int countOnes() const noexcept {
		return std::popcount(bits);
	}
	[[nodiscard]] constexpr int firstOne() const noexcept {
		// An empty board returns 64; callers must test it before indexing.
		return std::countr_zero(bits);
	}
	[[nodiscard]] constexpr int lastOne() const noexcept {
		// The corresponding empty-board sentinel is -1.
		return 63 - std::countl_zero(bits);
	}

	constexpr int removeFirstOne() noexcept {
		int const square = firstOne();
		// Clearing the least significant set bit visits only occupied squares.
		bits &= bits - 1;
		return square;
	}
	constexpr int removeLastOne() noexcept {
		int const square = lastOne();
		if (square >= 0) {
			clear(square);
		}
		return square;
	}

	constexpr void verticalFlip() noexcept { bits = std::byteswap(bits); }
	constexpr void horizontalFlip() noexcept {
		// Reverse bits within every rank byte in parallel: nibbles, pairs,
		// then adjacent bits. Byte order (the ranks) remains unchanged.
		bits = ((bits & 0xf0f0f0f0f0f0f0f0ULL) >> 4) |
			   ((bits & 0x0f0f0f0f0f0f0f0fULL) << 4);
		bits = ((bits & 0xccccccccccccccccULL) >> 2) |
			   ((bits & 0x3333333333333333ULL) << 2);
		bits = ((bits & 0xaaaaaaaaaaaaaaaaULL) >> 1) |
			   ((bits & 0x5555555555555555ULL) << 1);
	}

	constexpr void clear(int square) noexcept { bits &= ~mask(square).bits; }
	constexpr void set(int square) noexcept { bits |= mask(square).bits; }
	[[nodiscard]] constexpr bool test(int square) const noexcept {
		return (bits & mask(square).bits) != 0;
	}
	[[nodiscard]] static constexpr Bitboard mask(int square) noexcept {
		assert(square >= 0 && square < 64);
		return Bitboard{std::uint64_t{1} << square};
	}

	[[nodiscard]] constexpr bool
	operator==(Bitboard const &) const noexcept = default;
	[[nodiscard]] constexpr Bitboard operator~() const noexcept {
		return Bitboard{~bits};
	}
	[[nodiscard]] constexpr Bitboard operator&(Bitboard other) const noexcept {
		return Bitboard{bits & other.bits};
	}
	[[nodiscard]] constexpr Bitboard operator|(Bitboard other) const noexcept {
		return Bitboard{bits | other.bits};
	}
	[[nodiscard]] constexpr Bitboard operator^(Bitboard other) const noexcept {
		return Bitboard{bits ^ other.bits};
	}
	[[nodiscard]] constexpr Bitboard operator<<(int shift) const noexcept {
		assert(shift >= 0 && shift < 64);
		return Bitboard{bits << shift};
	}
	[[nodiscard]] constexpr Bitboard operator>>(int shift) const noexcept {
		assert(shift >= 0 && shift < 64);
		return Bitboard{bits >> shift};
	}

	constexpr Bitboard &operator&=(Bitboard other) noexcept {
		bits &= other.bits;
		return *this;
	}
	constexpr Bitboard &operator|=(Bitboard other) noexcept {
		bits |= other.bits;
		return *this;
	}
	constexpr Bitboard &operator^=(Bitboard other) noexcept {
		bits ^= other.bits;
		return *this;
	}
	constexpr Bitboard &operator<<=(int shift) noexcept {
		assert(shift >= 0 && shift < 64);
		bits <<= shift;
		return *this;
	}
	constexpr Bitboard &operator>>=(int shift) noexcept {
		assert(shift >= 0 && shift < 64);
		bits >>= shift;
		return *this;
	}

private:
	std::uint64_t bits = 0;
};

static_assert(sizeof(Bitboard) == sizeof(std::uint64_t));
static_assert(std::is_trivially_copyable_v<Bitboard>);
static_assert(std::is_standard_layout_v<Bitboard>);

} // namespace morphy_chess
