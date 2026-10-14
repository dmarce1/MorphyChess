#include <gtest/gtest.h>
#include <limits>
#include <morphychess/rating.hpp>
#include <stdexcept>

namespace morphy_chess {
namespace {

TEST(Rating, EqualEstablishedPlayersHaveSymmetricChanges) {
	EXPECT_EQ(ratingChangeWin(2000, 2000), 16);
	EXPECT_EQ(ratingChangeLoss(2000, 2000), -16);
	EXPECT_EQ(ratingChangeDraw(2000, 2000), 0);
	EXPECT_EQ(ratingComputeNewRating(2000, 2000, 1, 21), 2016);
	EXPECT_EQ(ratingComputeNewRating(2000, 2000, -1, 21), 1984);
	EXPECT_EQ(ratingComputeNewRating(2000, 2000, 0, 21), 2000);
}

TEST(Rating, OpponentStrengthAffectsGain) {
	EXPECT_LT(ratingChangeWin(2400, 2000), ratingChangeWin(2000, 2000));
	EXPECT_GT(ratingChangeWin(1600, 2000), ratingChangeWin(2000, 2000));
	EXPECT_GT(ratingChangeDraw(1600, 2000), 0);
	EXPECT_LT(ratingChangeDraw(2400, 2000), 0);
}

TEST(Rating, ComputesProvisionalAverage) {
	EXPECT_EQ(ratingComputeNewRating(2000, 1800, 1, 1), 2200);
	EXPECT_EQ(ratingComputeNewRating(2000, 1800, -1, 1), 1400);
	EXPECT_EQ(ratingComputeNewRating(2000, 1800, 0, 1), 1800);
	EXPECT_EQ(ratingComputeNewRating(2000, 1800, 1, 2), 2100);
	EXPECT_EQ(ratingComputeNewRating(2000, 1800, 1, 20), 2010);
}

TEST(Rating, ValidatesInputsAndAvoidsIntegerOverflow) {
	EXPECT_THROW(ratingComputeNewRating(2000, 2000, 1, 0),
				 std::invalid_argument);
	EXPECT_THROW(ratingComputeNewRating(2000, 2000, 2, 10),
				 std::invalid_argument);
	constexpr auto high = std::numeric_limits<int>::max();
	constexpr auto low = std::numeric_limits<int>::min();
	EXPECT_EQ(ratingComputeNewRating(high, high, 1, 1), high);
	EXPECT_EQ(ratingComputeNewRating(low, low, -1, 1), low);
	EXPECT_EQ(ratingChangeWin(low, high), 32);
	EXPECT_EQ(ratingChangeWin(high, low), 0);
}

} // namespace
} // namespace morphy_chess
