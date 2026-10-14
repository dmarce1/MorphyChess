# Bibliography

Source comments use bracketed keys such as `[zobrist]` to refer to entries
here. [algorithms.md](algorithms.md) maps the algorithms to their code.
References identify an algorithm family or a format specification; they do
not imply that MorphyChess exactly reproduces the cited implementation or
that its original author used that source. Implementation differences are
listed below. Links were checked on 2026-10-14.

## Search

<a id="alpha_beta"></a>
**[alpha_beta]** Donald E. Knuth and Ronald W. Moore.
“An analysis of alpha-beta pruning.” *Artificial Intelligence* 6(4),
293–326 (1975). [Publisher DOI](https://doi.org/10.1016/0004-3702(75)90019-3).
The background for the negamax threshold tests in
[search.cpp](src/search.cpp). MorphyChess returns a Boolean bound test and
adds selective pruning; it is not an exhaustive minimax implementation.

<a id="mtdf"></a>
**[mtdf]** Aske Plaat, Jonathan Schaeffer, Wim Pijls, and Arie de Bruin.
“A New Paradigm for Minimax Search.” University of Alberta Technical Report
94–18 (December 1994; archived in 2014).
[Author paper](https://arxiv.org/abs/1404.1515).
Related to `Search::serialTestDriver`: repeated memory-backed null-window
tests close a score interval. This engine steps by its score grain using
Boolean results, rather than using canonical fail-soft MTD(f) return values.
Its parallel bound scheduler is a local variation, not an implementation
attributed to this paper.

<a id="null_move"></a>
**[null_move]** Omid David-Tabibi and Nathan S. Netanyahu.
“Verified Null-Move Pruning.” *ICGA Journal* 25(3), 153–161 (2002).
[Publisher DOI](https://doi.org/10.3233/ICG-2002-25305),
[author paper](https://arxiv.org/abs/0808.1125).
Background for `Search::nullTest`, which searches a reduced-depth pass and
then verifies a successful cutoff. Its material threshold, reduction,
and verification flags are MorphyChess-specific; null-move pruning remains
a heuristic in zugzwang positions.

<a id="history_heuristic"></a>
**[history_heuristic]** Jonathan Schaeffer. “The History Heuristic.”
*ICCA Journal* 6(3), 16–19 (1983).
[Publisher DOI](https://doi.org/10.3233/ICG-1983-6305).
Background for [history.cpp](src/history.cpp): quiet moves producing cutoffs
receive depth-weighted bonuses. The exponential increments, rescaling,
and two killer slots are local choices, not claims about the paper's exact
data structures.

<a id="quiescence"></a>
**[quiescence]** Claude E. Shannon. “Programming a Computer for Playing
Chess.” *The London, Edinburgh, and Dublin Philosophical Magazine and
Journal of Science*, Series 7, 41(314), 256–275 (1950).
[Computer History Museum paper archive](https://www.computerhistory.org/chess/doc-431614f453dde/).
Historical background for evaluating relatively quiet positions instead
of stopping immediately during exchanges. The modern stand-pat, capture,
promotion, and check-evasion implementation in
[search_quiescent.cpp](src/search_quiescent.cpp) is more specific than that
general proposal.

<a id="late_move_reductions"></a>
**[late_move_reductions]** Stockfish developers. *Stockfish 17*,
[`src/search.cpp`](https://github.com/official-stockfish/Stockfish/blob/sf_17/src/search.cpp)
(2024 release).
A primary implementation reference for reducing late moves and re-searching
promising results. This is a comparison, not an origin claim or a claim of
code lineage. MorphyChess uses a simpler one-ply reduction selected by
`Node::okToReduce`, with a full-depth retry on a successful threshold test.

## Board representation and exchanges

<a id="zobrist"></a>
**[zobrist]** Albert L. Zobrist. *A New Hashing Method With Application
for Game Playing*. University of Wisconsin Technical Report 88 (April 1970).
[University report](https://research.cs.wisc.edu/techreports/1970/TR88.pdf).
The XOR feature signatures in [zobrist.hpp](morphychess/zobrist.hpp) and
[board.cpp](src/board.cpp) permit incremental make/unmake updates. MorphyChess
retains its own historical key table and stores side-to-move separately in
the relevant consumers; these are local representation choices.

<a id="rotated_bitboards"></a>
**[rotated_bitboards]** Robert M. Hyatt. “Rotated Bitmaps, a New Twist on
an Old Idea.” *ICCA Journal* 22(4), 213–222 (1999).
[Publisher DOI](https://doi.org/10.3233/ICG-1999-22403).
Background for [bits.hpp](morphychess/bits.hpp) and
[move_data.hpp](morphychess/move_data.hpp): maintain permuted occupancies
and index precomputed sliding attacks. Compile-time construction and the
cyclic diagonal layout are local choices; this is not magic-bitboard hashing.

<a id="static_exchange"></a>
**[static_exchange]** Jonatan Pettersson. “[Guide] Static exchange evaluation
(SEE).” *Mediocre Chess* (March 26, 2007).
[Engine author's tutorial](https://mediocrechess.blogspot.com/2007/03/guide-static-exchange-evaluation-see.html).
A primary tutorial for least-valued-attacker exchanges and the option to
stop exchanging. [see.cpp](src/see.cpp) instead uses recursive make/unmake
and legality filtering. It is a standalone helper, currently exercised by
tests; active search capture ordering uses a cheaper defended-square estimate.
This citation establishes the concept, not code provenance or priority.

## Learning and ratings

<a id="td_learning"></a>
**[td_learning]** Richard S. Sutton. “Learning to Predict by the Methods
of Temporal Differences.” *Machine Learning* 3, 9–44 (1988).
[Original paper scan](https://jmvidal.cse.sc.edu/library/sutton88a.pdf).
`temporalDifferenceWeights` in [learning.cpp](src/learning.cpp) computes
episodic TD(lambda) residual sums with a backward recurrence; the evaluator
update combines these weights with parameter gradients after the game.

<a id="tdleaf"></a>
**[tdleaf]** Jonathan Baxter, Andrew Tridgell, and Lex Weaver.
“TDLeaf(lambda): Combining Temporal Difference Learning with Game-Tree
Search.” *Australian Journal of Intelligent Information Processing Systems*
5(1), 39–43 (1998; arXiv posting 1999).
[Author paper](https://arxiv.org/abs/cs/9901001).
Related to `ParallelSearch::triggerLearning`: train from saved
principal-variation leaves. MorphyChess re-evaluates those leaves with
quiescence and uses centered finite differences through its quantized
evaluator; this is an adaptation, not an exact reproduction of KnightCap.

<a id="elo"></a>
**[elo]** Arpad E. Elo. *The Rating of Chessplayers, Past and Present*,
second edition (1986; first edition 1978), sections 8.3–8.4.
[Book scan](https://gwern.net/doc/statistics/order/comparison/1978-elo-theratingofchessplayerspastandpresent.pdf).
Background for the logistic rating expectation in [rating.cpp](src/rating.cpp).
The code uses base-10 scaling by 400 and fixed K=32. Its first-20-games
performance average and integer rounding are local policy, not a claim of
compliance with any current federation's rating regulations.

## Game formats and protocol

<a id="pgn"></a>
**[pgn]** Steven J. Edwards, coordinator, with contributors from
`rec.games.chess`. *Portable Game Notation Specification and Implementation
Guide* (1994).
[Specification transcription](https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm).
Relevant sections: 7 (tokens), 8 (tags, SAN, annotations, variations, results),
and 16.1 (FEN). [pgn.cpp](src/pgn.cpp) imports the main line and skips
variations; [board.cpp](src/board.cpp) reads FEN and resolves algebraic move
text. Its move formatter emits a long algebraic form with explicit origins.
The importer deliberately accepts some noncanonical input and is not a
complete validator of every PGN constraint.

<a id="xboard_protocol"></a>
**[xboard_protocol]** Tim Mann and H. G. Muller. *Chess Engine Communication
Protocol*, version 2 (document dated September 3, 2009).
[Maintainer specification](https://home.hccnet.nl/h.g.muller/engine-intf.html).
Defines command/reply framing, feature negotiation, thinking output, clock
units, and analysis mode implemented by [xboard.cpp](src/xboard.cpp).
The supported subset is announced by `protover`; the local time-budget
formula and HPX scheduling policy are not prescribed by the protocol.
