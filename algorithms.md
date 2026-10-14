# Algorithms used by MorphyChess

This inventory describes the active source, including optional and legacy
entry points where indicated. Reference keys link to
[bibliography.md](bibliography.md). Algorithm names describe the actual
implementation; local heuristics are identified explicitly.

## Board representation and move generation

| Algorithm or technique | Implementation | Details |
| --- | --- | --- |
| Bitboards and set-bit iteration | [`Bitboard`](morphychess/bitboard.hpp) | One bit per square, `a1 = 0`; population counts and bit scans use C++ bit operations. Clearing `x & (x - 1)` visits occupied squares only. |
| Parallel bit permutation | [`Bitboard::horizontalFlip`, `verticalFlip`](morphychess/bitboard.hpp) | Horizontal reflection swaps nibbles, bit pairs, then adjacent bits; vertical reflection reverses rank bytes. |
| Rotated bitboards and occupancy lookup tables | [`RotatedBitboard`](morphychess/bits.hpp), [`MoveData`](morphychess/move_data.hpp) | Four occupancy orientations turn sliding attacks into indexed rank/diagonal table lookups. Knight, king, and pawn attack masks are precomputed. See [rotated_bitboards](bibliography.md#rotated_bitboards). |
| Pseudolegal generation followed by legality filtering | [`Board::genPseudolegalMoves`, `makeMove`](src/board.cpp) | Generate geometric moves, apply one, and reject/undo it if the moving side's king is exposed. Separate generators cover captures, quiet moves, and promotions. |
| Incremental make/unmake | [`Board::makeMove`, `undoMove`](src/board.cpp) | Update square arrays, piece counts, occupancy, hashes, rights, and clocks together; undo records recover prior state, including special moves. |
| Zobrist hashing | [`ZobristData`](morphychess/zobrist.hpp), [`Board::computePositionKey`](src/board.cpp) | XOR feature keys support incremental updates. Historical keys are retained for book compatibility. The main key omits side to move; consumers distinguish it separately. See [zobrist](bibliography.md#zobrist). |
| Repetition and draw detection | [`Board::adjustRepeatCount`, `drawByMaterial`, `result`](src/board.cpp) | Compare historical keys within reversible history; handle the fifty-move counter, supported insufficient-material cases, mate, and stalemate. Search also uses a repetition flag to avoid caching path-dependent bounds. |
| Static exchange evaluation (SEE), standalone helper | [`Board::see`](src/see.cpp), [`genLvaAttackToSquare`](src/board.cpp) | Recursively consider the least-valued legal recapture on one square and allow declining an unfavorable continuation. Includes promotion gains and en passant; this remains a local exchange estimate, not full tactical search. Currently called by tests, not search move ordering. See [static_exchange](bibliography.md#static_exchange). |
| Mixed-radix three-piece indexing | [`Board::loadEgtbIndex`, `threeManEgtbIndex`](src/egtb.cpp) | Rank distinct placements in a `64 × 63 × 62` index space and canonicalize the stronger side by color/rank reflection. This supplies indexing helpers, not a retrograde solver or a loaded tablebase. |

## Search and concurrency

| Algorithm or technique | Implementation | Details |
| --- | --- | --- |
| Negamax null-window alpha-beta tests | [`Search::test`, `testChild`](src/search.cpp) | Determine whether the side-to-move score exceeds one threshold. Negate the threshold and Boolean result between plies; stop after a successful cutoff. See [alpha_beta](bibliography.md#alpha_beta). |
| MTD-style score convergence | [`Search::serialTestDriver`](src/search.cpp) | Repeat cached threshold tests from a guess, moving by the evaluator's score grain until lower and upper bounds meet. This is not canonical fail-soft MTD(f). See [mtdf](bibliography.md#mtdf). |
| Iterative deepening and parallel score-bound scheduling | [`ParallelSearch::nextSearch`, `worker`, `removeOutliers`](src/parallel_search.cpp) | Seed depths from earlier scores and assign different thresholds to tasks. Merge bounds under a mutex and cancel tests made obsolete by new bounds. This is a local scheduler, not root-move splitting or Lazy SMP. |
| Verified null-move pruning | [`Search::nullTest`](src/search.cpp) | Reduced-depth search after a pass, guarded by check/repetition/material conditions, followed by verification. See [null_move](bibliography.md#null_move). |
| Selective extensions | [`Search::extensionNeeded`](src/search.cpp) | Extend checks, promotions, and pawn advances to the seventh rank; these are engine-specific triggers. |
| Late-move reductions and re-search | [`Search::testChild`](src/search.cpp), [`Node::okToReduce`](src/node.cpp) | Reduce selected later quiet moves by one ply, then retry at full depth if the reduced test succeeds. See [late_move_reductions](bibliography.md#late_move_reductions). |
| Quiescence search | [`Search::quiescentTest`, `quiescentValue`](src/search_quiescent.cpp), [`Node::doNextNonquiescentMove`](src/node.cpp) | Use stand-pat outside check, continue captures/promotions, and search evasions when checked. Recognize terminal draws before accepting a static score. `quiescentValue` returns a White-relative score. See [quiescence](bibliography.md#quiescence). |
| Staged move ordering | [`Node::doNextMove`, `captureMoveValue`](src/node.cpp) | Hash move, promotions, favorable captures, an available killer, remaining captures, then history-ranked quiet moves. Select the next best move without fully sorting unsearched moves. Capture scoring combines victim/attacker values with a defended-square penalty; it does not call SEE. |
| History and killer heuristics | [`History::update`, `moveHistory`](src/history.cpp) | Reward cutoff moves by depth, rescale large counters, and retain two candidate killer moves per ply. See [history_heuristic](bibliography.md#history_heuristic). |
| Bound transposition tables | [`MemoryTable::read`, `write`](src/memory.cpp) | Separate upper/lower and local/global stores, with key, color, depth, fifty-move clock, and null-verification checks. Repetition-sensitive and mate-range bounds are excluded. Striped locks protect shared access. |
| Principal-variation reconstruction | [`Search::reconstructPrincipalVariation`](src/search.cpp) | Follow cached best moves, re-search missing moves where possible, and undo all temporary moves; optionally retain the leaf for learning. |
| Cooperative task cancellation | [`Search::checkKill`](src/search.cpp), [`ParallelSearch::runImpl`](src/parallel_search.cpp) | Private worker boards, shared synchronized caches, stop flags, cooperative yields, and joined futures. Retain a legal fallback if no depth finishes. |

## Evaluation, learning, and opening book

| Algorithm or technique | Implementation | Details |
| --- | --- | --- |
| Handcrafted material and positional evaluation | [`StaticEvaluator::materialScore`, `nonMaterialScore`](src/static_evaluator.cpp) | Material imbalance, exchange incentives, piece-square tables, pawn-dependent piece bonuses, bishop pairs, rook files/ranks, and king phase/shelter. Weights are validated and converted to integers; evaluation scores are bounded and rounded to a score grain. |
| Fifty-move score damping | [`StaticEvaluator::nonMaterialScore`](src/static_evaluator.cpp) | Reduce nonzero positional advantages toward a draw as the reversible-move clock grows. This is an evaluation heuristic in addition to rule-based draw detection. |
| Lazy threshold evaluation | [`StaticEvaluator::test`](src/static_evaluator.cpp) | First compare material plus/minus a bound on positional terms against the search threshold; calculate positional features only when necessary. |
| Pawn/king structure evaluation and hash cache | [`StaticEvaluator::evaluatePawnsAndKings`, `readPawnTable`, `writePawnTable`](src/static_evaluator.cpp) | File/rank summaries classify pawn weaknesses and passers. Cache keys include pawn/king state and king-evaluation phase flags; striped locks synchronize slots. |
| Legacy material evaluator | [`Board::evaluate`, `evaluateMaterial`](src/evaluate.cpp) | An independent material-only helper with exchange bonuses and symmetric score clamps. The main search uses `StaticEvaluator`. |
| Forward-view TD(lambda), computed backward | [`temporalDifferenceWeights`](src/learning.cpp) | Compute lambda-weighted future prediction differences (no reward discount factor) in linear time using a backward recurrence. See [td_learning](bibliography.md#td_learning). |
| Optional TDLeaf-style parameter update | [`ParallelSearch::triggerLearning`](src/learning.cpp) | Re-evaluate saved PV leaves through quiescence, squash pawn scores with `tanh`, estimate parameter gradients by centered finite differences, and batch the update after a result. See [tdleaf](bibliography.md#tdleaf). Learning is opt-in. |
| Elo-style ratings | [`ratingChangeWin`, `ratingComputeNewRating`](src/rating.cpp) | Logistic expectation with scale 400 and fixed K=32 after the provisional period; early games use a local performance-average rule. See [elo](bibliography.md#elo). |
| Sorted opening-book lookup | [`Book::search`, `digest`](src/book.cpp) | Import main lines, collect positions by undoing moves, sort by color/key, and look up entries with binary search. |
| Weighted random book selection | [`Book::findMove`](src/book.cpp) | Sample legal book continuations by stored win counts, with uniform fallback if all weights are zero. Book displays instead report the share of all recorded games. |
| Opening-book result learning | [`Book::learning`](src/book.cpp) | Traverse game history, update W/L/D counters and ratings for stored positions after the engine's moves, and save modified entries. Independent of evaluator learning. |
| Failure-safe book publication | [`Book::save`](src/book.cpp) | Write and close a new sibling temporary file before renaming it over the destination. Refuse existing temporary files and symlink destinations; failure preserves the prior book. This is not a guarantee against power-loss data loss. |

## Parsing, protocol, and validation

| Algorithm or technique | Implementation | Details |
| --- | --- | --- |
| Transactional FEN/PGN parsing | [`Board::loadFen`, `makeTextMove`](src/board.cpp), [`Pgn::next`](src/pgn.cpp) | Validate/replay into a candidate board before replacing the caller's board. PGN scanning skips comments and nested variations and recognizes adjacent annotation/result tokens. See [pgn](bibliography.md#pgn). |
| XBoard protocol state machine | [`Xboard::processCommand`, `pollSearch`, `run`](src/xboard.cpp) | Nonblocking line input, explicit play/ponder/analysis modes, joined search completion, synchronized output, and a local time-budget heuristic. See [xboard_protocol](bibliography.md#xboard_protocol). |
| Periodic cooperative seek task | [`SeekBot`](src/seek_bot.cpp) | Generate random ICS time controls, sleep cooperatively, and join on stop/destruction. |
| Perft and state reconstruction checks | [`board_test.cpp`](tests/board_test.cpp), [`Board::assertBoardConsistency`](src/debug.cpp) | Count legal move-tree leaves and compare against reference totals; independently rebuild cached board/hash state to check make/unmake. These are correctness checks, not playing-strength measurements. |

## Boundaries and further review

The engine contains inactive historical tuning constants; their presence does
not establish an active algorithm. In particular, the current search does not
implement futility pruning, and three-piece indexing does not provide endgame
tablebase probing. Scores and move choices are heuristic rather than proofs
of optimal play.

Further review identified these issues outside the contained fixes in this pass:

- Repetition identity currently includes the en-passant file even when no
  legal en-passant capture exists. For example, after `e2e4`, two cycles of
  `g8f6 g1f3 f6g8 f3g1` miss the first occurrence because its irrelevant
  en-passant key differs. Correcting repetition identity should preserve
  historical opening-book keys and account for pinned en-passant captures.
- `StaticEvaluator::configure` accepts very small positive piece values
  that make its derived material caps negative. Four material parameters
  equal to 1 produce `maxMaterialWoBonus = -793`. Derived-bound validation
  and finite-difference behavior at parameter limits need a coordinated
  policy for custom/learned weights; default parameters are unaffected.
- The middlegame White and Black king tables are not exact rank mirrors.
  The White second rank is -40 where the mirrored Black rank is -20;
  changing this requires deciding whether it is a misplaced tuning row.
- Book display percentages use all recorded games whereas selection uses
  wins. The display should make that distinction clearer if retained.

`Board::makeMove` is a low-level operation with a pseudolegal-input
precondition. External callers should use `isLegalMove` or the text parsers
before applying arbitrary moves.
