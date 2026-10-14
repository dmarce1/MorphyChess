#include <array>
#include <chrono>
#include <morphychess/bench.hpp>
#include <morphychess/parallel_search.hpp>
#include <ostream>
#include <stdexcept>
namespace morphy_chess {
int runBenchmark(const CommandLineOptions &options, std::ostream &output) {
	constexpr std::array<const char *, kNumBenchmarks> positions{
		initialPosition,
		"8/1b3Rk1/p2rB1pp/8/p2p4/8/1PP3PP/6K1 b - - 0 33",
		"4r2k/1q4p1/pp4p1/2b1B1Nb/PPQ5/8/2P3PP/5R1K b - b3 0 32",
		"r1bq1rk1/pp2bppp/2nppn2/2p5/4P3/2NP1NP1/PPP2PBP/R1BQ1RK1 w - - 0 8",
		"rnbqkb1r/p1p2ppp/1p2p3/4P3/3Pp3/4B3/PPP1NPPP/R2QKB1R b KQkq - 0 7",
		"r1b1kb1r/pppnqppp/3p1n2/6B1/8/3P1N2/PPP1QPPP/RN2KB1R w KQkq - 0 8",
		"r3k2r/4qppp/p3p3/1p2n3/3QN3/8/PPP3PP/2KR3R b kq - 0 17",
		"8/5k2/p1r1pp2/6pp/6r1/2N3P1/RPP1K2P/2N5 w - h6 0 32",
		"r3rbk1/3n1pp1/p3pn1p/3b4/1PBN4/P4N2/1B3PPP/3R1RK1 w - - 0 25",
		"3k4/8/3q4/8/8/4KB2/8/8 b - - 0 32"};
	Board board;
	StaticEvaluator evaluator;
	MemoryTable table(kTableSize, kTableLocksPerThread * options.workers(),
					  options.workers());
	ParallelSearch search(board, evaluator, table, options.workers());
	search.setUseBook(false);
	long long totalNodes = 0;
	const auto start = std::chrono::steady_clock::now();
	for (size_t index = 0; index < positions.size(); ++index) {
		if (!board.loadFen(positions[index])) {
			throw std::runtime_error("Invalid benchmark position " +
									 std::to_string(index + 1));
		}
		search.resetMemory();
		const auto result =
			search.run(options.depth(), options.benchmarkSeconds());
		if (result.move().isEmpty() || !board.isLegalMove(result.move())) {
			throw std::runtime_error("Benchmark returned no legal move");
		}
		totalNodes += result.nodes();
		char move[64]{};
		board.moveToStr(move, result.move());
		output << "Position " << index + 1 << '/' << positions.size() << ": "
			   << move << " depth=" << result.depth()
			   << " score=" << result.score() << " nodes=" << result.nodes()
			   << '\n';
	}
	const double elapsed =
		std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
			.count();
	output << "Total nodes: " << totalNodes << " in " << elapsed
		   << " seconds\n";
	return 0;
}
} // namespace morphy_chess
