#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <morphychess/learning.hpp>
#include <morphychess/parallel_search.hpp>
#include <stdexcept>

namespace morphy_chess {
std::vector<double>
temporalDifferenceWeights(const std::vector<double> &predictions, double target,
						  double lambda) {
	if (!std::isfinite(lambda) || lambda < 0.0 || lambda > 1.0 ||
		!std::isfinite(target)) {
		throw std::invalid_argument("Invalid temporal-difference parameters");
	}
	std::vector<double> weights(predictions.size());
	double accumulated = 0.0;
	for (size_t reverseIndex = predictions.size(); reverseIndex > 0;
		 --reverseIndex) {
		const size_t index = reverseIndex - 1;
		if (!std::isfinite(predictions[index])) {
			throw std::invalid_argument(
				"Non-finite evaluation in learning history");
		}
		const double next =
			index + 1 < predictions.size() ? predictions[index + 1] : target;
		accumulated = next - predictions[index] + lambda * accumulated;
		if (!std::isfinite(accumulated)) {
			throw std::overflow_error(
				"Temporal-difference accumulation overflowed");
		}
		weights[index] = accumulated;
	}
	return weights;
}
void ParallelSearch::setLearning(bool value) {
	stop();
	if (value && !learningOn) {
		learningPositions.clear();
	}
	learningOn = value;
}
void ParallelSearch::triggerLearning(int computerSide, Result result) {
	stop();
	if (!learningOn || learningPositions.empty() || result == inProgress ||
		result == invalid) {
		return;
	}
	constexpr double lambda = 0.70;
	constexpr int delta = 2;
	if (computerSide != white && computerSide != black) {
		throw std::invalid_argument("Invalid learning perspective");
	}
	const double perspective = computerSide == white ? 1.0 : -1.0;
	const double target = perspective * (result == whiteWin	  ? 1.0
										 : result == blackWin ? -1.0
															  : 0.0);
	std::vector<double> predictions;
	std::vector<EvaluatorParameters> jacobians;
	MemoryTable scratch(4096, 16, 1);
	auto evaluate = [&](const Board &position,
						const EvaluatorParameters &parameters) {
		Board copy = position;
		StaticEvaluator value(parameters);
		value.setGrainSize(delta);
		Search search(copy, value, scratch);
		search.setGrainSize(delta);
		scratch.reset();
		const double score = search.quiescentValue(0);
		return perspective * std::tanh(value.beta() * score / 100.0);
	};
	for (const auto &position : learningPositions) {
		predictions.push_back(evaluate(position, evalParams));
		EvaluatorParameters jacobian;
		for (size_t index = 0; index < evalParams.size(); ++index) {
			EvaluatorParameters perturbed = evalParams;
			perturbed[index] += delta;
			const double above = evaluate(position, perturbed);
			perturbed[index] -= 2 * delta;
			const double below = evaluate(position, perturbed);
			jacobian[index] = (above - below) / (2.0 * delta);
		}
		jacobians.push_back(jacobian);
	}
	const auto weights = temporalDifferenceWeights(predictions, target, lambda);
	EvaluatorParameters updatedParameters = evalParams;
	for (size_t index = 0; index < evalParams.size(); ++index) {
		double update = 0.0;
		for (size_t position = 0; position < weights.size(); ++position) {
			update += weights[position] * jacobians[position][index];
		}
		updatedParameters[index] += kLearningAlpha * update;
	}
	// Validate the complete candidate before changing the live evaluator or
	// saved parameters.
	StaticEvaluator validated(updatedParameters);
	const std::filesystem::path temporaryBrain = "brain.dat.tmp";
	try {
		std::ofstream brain(temporaryBrain, std::ios::binary | std::ios::trunc);
		if (!brain) {
			throw std::runtime_error("Unable to write learning parameters");
		}
		brain.write(reinterpret_cast<const char *>(updatedParameters.data()),
					static_cast<std::streamsize>(sizeof(double) *
												 updatedParameters.size()));
		brain.close();
		if (!brain) {
			throw std::runtime_error(
				"Unable to finish writing learning parameters");
		}
		std::ofstream history("history.txt", std::ios::app);
		for (size_t index = 0; index < updatedParameters.size(); ++index) {
			history << updatedParameters[index] << ' ';
		}
		history << '\n';
		history.close();
		if (!history) {
			throw std::runtime_error("Unable to write learning history");
		}
		std::filesystem::rename(temporaryBrain, "brain.dat");
	} catch (...) {
		std::error_code ignored;
		std::filesystem::remove(temporaryBrain, ignored);
		throw;
	}
	evaluator.configure(updatedParameters);
	evalParams = updatedParameters;
	table.reset();
	learningPositions.clear();
}
} // namespace morphy_chess
