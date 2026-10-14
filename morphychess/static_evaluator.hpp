#pragma once

#include <array>
#include <cstddef>

#include <hpx/synchronization/mutex.hpp>

#include <morphychess/board.hpp>
#include <morphychess/params.hpp>

namespace morphy_chess {
inline constexpr int kEvalParamCount = 16;
inline constexpr int kPawnTableLocks =
	kPawnTableLocksPerThread * defaultThreadCount;
inline constexpr int kEndgameDefinition = 1200;

// Stable 16-double learning/file order: N/B/R/Q material; positional cap;
// pawn table weight; doubled/isolated/backward pawn penalties; passed-pawn
// bonus; king/knight/bishop table weights; semi-open/open/seventh-rank rook
// bonuses. Material is in centipawns and table weights are percentages.
class EvaluatorParameters {
public:
	constexpr EvaluatorParameters() = default;
	constexpr explicit EvaluatorParameters(
		std::array<double, kEvalParamCount> values) :
		values(values) {}
	constexpr double &operator[](size_t index) { return values[index]; }
	constexpr double operator[](size_t index) const { return values[index]; }
	constexpr double *data() { return values.data(); }
	constexpr const double *data() const { return values.data(); }
	constexpr size_t size() const { return values.size(); }

private:
	std::array<double, kEvalParamCount> values{};
};
inline constexpr EvaluatorParameters
	defaultEvaluatorParameters(std::array<double, kEvalParamCount>{
		325.0, 325.0, 500.0, 900.0, 199.9, 100.0, 10.0, 20.0, 8.0, 20.0, 100.0,
		100.0, 100.0, 10.0, 15.0, 20.0});

// A truncated pawn/king hash plus the White-relative pawn/king score.
// A zero key denotes an invalid entry; material phase is mixed into the key.
class PawnTableEntry {
	friend class StaticEvaluator;

private:
	U32 key{};
	I32 score{};
};

// Integer, centipawn-based evaluator used by the search. Public material and
// positional scores are from the side-to-move perspective; internal feature
// sums and pawn-cache values use White's perspective. Configuration must be
// changed only after search workers have stopped.
class StaticEvaluator {
public:
	explicit StaticEvaluator(
		const EvaluatorParameters &params = defaultEvaluatorParameters);
	StaticEvaluator(const StaticEvaluator &) = delete;
	StaticEvaluator &operator=(const StaticEvaluator &) = delete;
	void configure(const EvaluatorParameters &params);
	int materialBalance(const Board &board);
	int materialScore(const Board &board, int &whiteMat, int &blackMat);
	int nonMaterialScore(const Board &board, int whiteMat, int blackMat);
	// Return whether the grain-rounded full score exceeds a threshold placed
	// strictly between score-grain multiples (the search's null-window test).
	bool test(const Board &board, int testValue);
	int checkmateValue(int ply) const;
	int drawValue() const { return 0; }
	void setGrainSize(int value);
	// Scale for tanh(beta * score_in_pawns) in temporal-difference learning.
	constexpr double beta() const { return 0.25; }
	int grainSize() const { return scoreGrain; }
	void resetPawnTable();

private:
	void writePawnTable(U32 key, int score);
	bool readPawnTable(U32 key, int &score);
	int evaluatePawnsAndKings(const Board &board, int whiteMat, int blackMat);
	std::array<int, 7> materialValues{};
	int maximumNonMaterialScore{};
	int scoreGrain{kDefaultGrainSize};
	int pawnSquareValueWeight{};
	int kingSquareValueWeight{};
	int knightSquareValueWeight{};
	int bishopSquareValueWeight{};
	int doubledPawnPenalty{};
	int isolatedPawnPenalty{};
	int backwardsPawnPenalty{};
	int passedPawnBonus{};
	int rookSemiOpenFileBonus{};
	int rookOpenFileBonus{};
	int rookRank7Bonus{};
	int maxMaterial{};
	int maxMaterialWoBonus{};
	int startingMaterial{};
	std::array<PawnTableEntry, kPawnTableSize> pawnTable{};
	std::array<hpx::mutex, kPawnTableLocks> locks;
};
} // namespace morphy_chess
