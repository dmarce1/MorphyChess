#pragma once
#include <vector>
namespace morphy_chess {
std::vector<double>
temporalDifferenceWeights(const std::vector<double> &predictions, double target,
						  double lambda);
}
