#pragma once

namespace morphy_chess {

/*********************************************************************************
MorphyChess 1.0.5 Copyright (C) 2008 Dominic C. Marcello
(dmarcello@phys.lsu.edu)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see [http://www.gnu.org/licenses/].
**********************************************************************************/
/****************************************
0.0.0 - Original release
0.0.1 - Fixed bug in book move display on Xboard
0.0.2 - Fixed bug in verified null move heuristic
	  - Fixed bug in material evaluation
0.0.3 - Changed REDUCTION_LIMIT to 2
	  - Fixed bug with opening book display
1.0.0a - Fixed bug with draw by repetition
	   - Enabled engine to decide when to resign, or offer/accept draws
	   - added bonuses for pieces based on total pawns on board
	   - engine now moves immediately when there is only one legal move
1.0.1a - fixed bug with pondering
1.0.2a - added a "contempt factor" component, which discourages drawing with
lower rated opponents and encourages drawing against higher rated opponents
1.0.3X - corrected bug with memory tables and made memory table reads more
efficient
	  - made repeated position draws detectable after the 1st repetition
	  - penalty added for side in the lead which increases as the 50 move rule
is approached
	  - adjusted history branch factor to 3.5 - 1, 3, 12, 42, 150, 525, 1838,
6433, 22518, 78815, 275854, 965491, 3379220, 11827271, 41395491, 144884079,
507094277, 1774829971
	  - added more sophisticated book learning
1.0.4 - Added extensions for pawn promotions and moves to the 7th rank
	  - Added futility pruning
1.0.5 - Corrected severe null move bug which allowed all verifications to return
true.
	  - removed futility pruning

******************************************************/

inline constexpr int defaultThreadCount = 8;
inline constexpr auto kMinDepth = 4;
inline constexpr auto kFullDepthMoves = 4;
inline constexpr auto kReductionLimit = 2;
inline constexpr auto kFutilityMargin = 300;
inline constexpr auto kMaxDepth = 64;
inline constexpr auto kMaxSearchDepth = 32;
inline constexpr auto kInfty = 100000;
inline constexpr auto kMinCheckmateValue = (10000 - (5 * kMaxDepth));
inline constexpr auto kDefaultHistoryBranchFactor = 4.0;
inline constexpr auto kNullReduction = 3;
inline constexpr auto kDefaultGrainSize = 8;
inline constexpr auto kDefaultPollFrequency = 1024;
inline constexpr auto kName = "Morphy Chess 1.0.5";
inline constexpr auto kLearningAlpha = 100.0;
inline constexpr auto kTableSize = (8 * 1024 * 1024);
inline constexpr auto kTableLocksPerThread = 16;
inline constexpr auto kPawnTableSize = 1024;
inline constexpr auto kPawnTableLocksPerThread = 16;
inline constexpr auto kNumBenchmarks = 10;
inline constexpr auto kBenchTime = 60;
inline constexpr auto kBookSize = 4096;
inline constexpr auto kTimeCut1 = 0;
inline constexpr auto kTimeCut2 = (-50);
inline constexpr auto kTimeCut3 = (-100);
inline constexpr auto kTimeCut4 = (-200);
inline constexpr auto kResignLimit = 900;
inline constexpr auto kMinHalfmoves4draw = 100;
inline constexpr auto kDrawOfferFreq = 10;
inline constexpr auto kContemptLevel = 25;
inline constexpr auto kDrawTimeLimit = 60;
inline constexpr auto kBookName = "book.bin";
inline constexpr auto kInitialBookRating = 2000;
inline constexpr auto kStartupWait = 10;
inline constexpr auto kSpeedLimit = 200;

} // namespace morphy_chess
