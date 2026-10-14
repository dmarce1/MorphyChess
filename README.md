# MorphyChess

MorphyChess 1.0.5 is a C++23 chess engine with an XBoard protocol interface.
It uses HPX's local runtime in one process. Search tasks, timers, and the seek
bot use HPX asynchronous tasks and futures; no distributed runtime is started.

## Build

Requires CMake 3.25+, a C++23 compiler, and HPX 1.11+. The XBoard input loop
currently uses POSIX `poll`/`read` and is tested on Linux. Prefer an HPX build
with `HPX_WITH_DISTRIBUTED_RUNTIME=OFF` and `HPX_WITH_CUDA=OFF`; an existing
full HPX installation also works with the local entry point. Build HPX with a
compatible compiler and standard library.

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/hpx/install
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure
cd build/release
./morphy --threads=4 --hpx:threads=4
```

GoogleTest is required when `BUILD_TESTING=ON` (the default). CMake first
looks for an installed package, then downloads the pinned GoogleTest 1.14.0
release. For an offline build, supply an existing checkout using
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest`. Use
`-DBUILD_TESTING=OFF` to build only the engine.

An HPX package built with CUDA or MPI enabled imports those dependencies even
for a local CPU application. With this workstation's existing installation,
the validated configuration is:

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/home/dmarce1/local/release \
  -DCMAKE_CUDA_STANDARD=20 \
  -DMPI_CXX_HEADER_DIR=/usr/lib/x86_64-linux-gnu/openmpi/include \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/home/dmarce1/workspace/DG/build/release/_deps/googletest-src
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure
```

`gcc_compile.sh` configures `build/release`; additional arguments are forwarded
to CMake. `icc_compile.sh` selects Intel `icpx` and `build/intel`. The executable
is named `morphy`; the historical prebuilt binaries in the repository are not
the CMake build output.

## Run

Run from the build directory to use its `book.bin` and `eco.pgn`. CMake copies
these files on first configuration and preserves existing copies, including
learned book data, on subsequent configurations. `--book` selects another book
and `--no-book` disables it. Enter `quit` to exit; end of input also exits.

`--threads` sets the number of search tasks; `--hpx:threads` sets the number of
OS workers that execute HPX tasks. Both default to eight and can be set
independently. A single HPX worker supports search, cancellation, and protocol
input. Use `--help` for engine and HPX options, or `--version` for the version.

```sh
./morphy --bench --depth=4 --bench-seconds=0 --threads=4 --hpx:threads=4
```

The benchmark contains ten positions. Zero seconds selects a depth-only run;
the default limit is six seconds per position. The old `-bench` spelling is
also accepted. In XBoard mode, `sd`, `st`, and `level` control search limits.
Evaluator learning is opt-in through the `ParallelSearch::setLearning` and
`triggerLearning` APIs; it saves weights in `brain.dat` and appends to
`history.txt` in the current directory. XBoard uses the default evaluator and
updates opening-book statistics when rated game results are supplied.

## Source and tests

See [algorithms.md](algorithms.md) for the algorithm map and
[bibliography.md](bibliography.md) for references cited in source comments.

Public headers live in `morphychess/`, implementations in `src/`, and GoogleTest
tests in `tests/`. Include headers as `<morphychess/board.hpp>`. Code uses the
`morphy_chess` namespace, PascalCase types, camelCase functions and variables,
and snake_case filenames. Constants are `constexpr` values.

`Board`, `Move`, `Search`, `ParallelSearch`, `MemoryTable`, `History`,
`StaticEvaluator`, `Book`, `Pgn`, `SeekBot`, and `Xboard` encapsulate private
state. `Bitboard` stores a private 64-bit square mask and provides constexpr
methods for bit scans, counts, updates, and flips, plus bitwise operators.
`RotatedBitboard` owns the four occupancy orientations and provides updates,
occupancy indices, and sliding attack queries. `Board` provides hash
recomputation, material evaluation, static exchange evaluation, consistency
checks, and three-piece endgame indexing. `Move::coordinateString` formats
protocol coordinates; capture ordering is a private `Node` method.
Conversions to and from raw integers are explicit; position hashes and counters
remain integer types. `MoveData`, `BitboardMasks`, and `ZobristData` provide
constexpr instance lookups backed by shared inline constexpr objects. Their
headers contain the implementations; `RotatedBitboard` methods live in
`bits.hpp`. The former initialization functions remain compatibility no-ops.
Zobrist data preserves all 780 legacy keys for opening-book compatibility.
`HistoryIncrements` generates the default increment table at compile time;
custom branch factors retain runtime generation. `MaterialValues` holds the
fixed values used by exchange and move-ordering algorithms. Ratings and other
independent algorithms remain ordinary functions.
Searches use private board copies, shared synchronized caches, cooperative
cancellation, and joined futures.

CTest runs the full unit suite with one and four HPX workers, plus executable
help, option validation, version, and benchmark smoke tests. Coverage includes move generation and
perft, make/undo and hashing, FEN/SAN/PGN parsing, opening-book persistence,
evaluation and exchange, history and transposition tables, serial and parallel
search, time limits and cancellation, learning, seek tasks, CLI options, and
XBoard commands. The three-piece endgame indexing helpers are tested; the
repository does not supply a complete external endgame-table database.

For project-code undefined-behavior checks, configure a separate Debug build
with `-DMORPHY_SANITIZERS=undefined,float-cast-overflow`, then run CTest with
`UBSAN_OPTIONS=halt_on_error=1`. The linked HPX installation is not rebuilt or
instrumented by this option. Tests exercise regressions and representative
positions; they do not measure playing strength or prove search correctness
for every chess position.
