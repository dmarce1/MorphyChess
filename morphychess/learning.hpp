#pragma once
#include <vector>

namespace morphy_chess {
// Undiscounted episodic TD(lambda) residual sums for an ordered history.
// lambda=0 uses the next prediction; lambda=1 uses target-prediction at
// every position. Inputs must be finite and lambda must lie in [0, 1].
std::vector<double>
temporalDifferenceWeights(const std::vector<double> &predictions, double target,
						  double lambda);
} // namespace morphy_chess
