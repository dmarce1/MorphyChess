#pragma once

#include <array>
#include <source_location>
#include <stdio.h>
#include <string>

#include <morphychess/bitboard.hpp>
#include <morphychess/bits.hpp>
#include <morphychess/integers.hpp>
#include <morphychess/params.hpp>

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

inline constexpr auto initialPosition =
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
// Piece type zero means empty; signed square values use + for white and -
// for black. Color indices deliberately use white=1 and black=0.
inline constexpr auto empty = 0;
inline constexpr auto pawn = 1;
inline constexpr auto knight = 2;
inline constexpr auto bishop = 3;
inline constexpr auto rook = 4;
inline constexpr auto queen = 5;
inline constexpr auto king = 6;
inline constexpr auto white = 1;
inline constexpr auto black = 0;
inline constexpr auto kA1 = 0;
inline constexpr auto kB1 = 1;
inline constexpr auto kC1 = 2;
inline constexpr auto kD1 = 3;
inline constexpr auto kE1 = 4;
inline constexpr auto kF1 = 5;
inline constexpr auto kG1 = 6;
inline constexpr auto kH1 = 7;
inline constexpr auto kA8 = 56;
inline constexpr auto kB8 = 57;
inline constexpr auto kC8 = 58;
inline constexpr auto kD8 = 59;
inline constexpr auto kE8 = 60;
inline constexpr auto kF8 = 61;
inline constexpr auto kG8 = 62;
inline constexpr auto kH8 = 63;
inline constexpr auto maxMoves = 512;
inline constexpr auto maxHalfMoves = 1024;
inline constexpr Bitboard fileA{0x0101010101010101ULL};
inline constexpr Bitboard fileB{0x0202020202020202ULL};
inline constexpr Bitboard fileC{0x0404040404040404ULL};
inline constexpr Bitboard fileD{0x0808080808080808ULL};
inline constexpr Bitboard fileE{0x1010101010101010ULL};
inline constexpr Bitboard fileF{0x2020202020202020ULL};
inline constexpr Bitboard fileG{0x4040404040404040ULL};
inline constexpr Bitboard fileH{0x8080808080808080ULL};
inline constexpr Bitboard rank1{0x00000000000000FFULL};
inline constexpr Bitboard rank2{0x000000000000FF00ULL};
inline constexpr Bitboard rank3{0x0000000000FF0000ULL};
inline constexpr Bitboard rank4{0x00000000FF000000ULL};
inline constexpr Bitboard rank5{0x000000FF00000000ULL};
inline constexpr Bitboard rank6{0x0000FF0000000000ULL};
inline constexpr Bitboard rank7{0x00FF000000000000ULL};
inline constexpr Bitboard rank8{0xFF00000000000000ULL};
inline constexpr Bitboard whiteSquares{0xAA55AA55AA55AA55ULL};
inline constexpr auto blackSquares = (~(whiteSquares));

constexpr int colorFlip(int color) noexcept {
	return color == white ? black : white;
}
enum Result {
	whiteWin = 1,
	blackWin = 2,
	draw = 3,
	inProgress = 4,
	invalid = 5
};

// Compact coordinate move. Promotion=1 is the empty/list-end/null-search
// sentinel, since a real promotion can only be knight through queen.
class Move {
public:
	constexpr Move() noexcept = default;
	static Move fromString(char const *text);
	std::string coordinateString() const;
	constexpr Move(int from, int to, int promo = 0) noexcept :
		fromValue(from), toValue(to), promoValue(promo) {}
	constexpr int from() const noexcept { return fromValue; }
	constexpr int to() const noexcept { return toValue; }
	constexpr int promo() const noexcept { return promoValue; }
	constexpr void setFrom(int value) noexcept { fromValue = value; }
	constexpr void setTo(int value) noexcept { toValue = value; }
	constexpr void setPromo(int value) noexcept { promoValue = value; }
	constexpr bool isEmpty() const noexcept { return promoValue == 1; }
	constexpr void setEmpty() noexcept { *this = Move(0, 0, 1); }
	constexpr bool operator==(Move const &) const noexcept = default;

private:
	U16 fromValue : 6 = 0;
	U16 toValue : 6 = 0;
	U16 promoValue : 4 = 1;
};

class Board;
// halfMoveCount indexes local undo history, independent of the FEN move
// number. epCaptureSquare records the pawn to remove, not its landing target;
// zero means no en-passant right because a capturable pawn cannot be on a1.
class BoardFlags {
public:
	constexpr int halfMoveCount() const noexcept { return halfMoveCountValue; }
	constexpr int epCaptureSquare() const noexcept {
		return epCaptureSquareValue;
	}
	constexpr int mostRecentCapture() const noexcept {
		return mostRecentCaptureValue;
	}
	constexpr int repeatCount() const noexcept { return repeatCountValue; }
	constexpr int check() const noexcept { return checkValue; }
	constexpr int draw() const noexcept { return drawValue; }
	constexpr int sideToMove() const noexcept { return sideToMoveValue; }
	constexpr int blackKingSide() const noexcept { return blackKingSideValue; }
	constexpr int blackQueenSide() const noexcept {
		return blackQueenSideValue;
	}
	constexpr int whiteKingSide() const noexcept { return whiteKingSideValue; }
	constexpr int whiteQueenSide() const noexcept {
		return whiteQueenSideValue;
	}

private:
	friend class Board;
	U32 halfMoveCountValue : 11 = 0;
	U32 epCaptureSquareValue : 6 = 0;
	U32 mostRecentCaptureValue : 4 = 0;
	U32 repeatCountValue : 2 = 0;
	U32 checkValue : 1 = 0;
	U32 drawValue : 1 = 0;
	U32 sideToMoveValue : 1 = 0;
	U32 blackKingSideValue : 1 = 0;
	U32 blackQueenSideValue : 1 = 0;
	U32 whiteKingSideValue : 1 = 0;
	U32 whiteQueenSideValue : 1 = 0;
};

// Snapshot the irreversible metadata before each move; piece placement is
// reversed from the move and captured piece type instead of copying a board.
class UndoInfo {
public:
	std::uint64_t positionKey() const noexcept { return positionKeyData; }
	std::uint64_t pawnKey() const noexcept { return pawnKeyData; }
	int fiftyMoveRule() const noexcept { return fiftyMoveRuleData; }
	Move move() const noexcept { return moveData; }
	BoardFlags const &flags() const noexcept { return flagsData; }

private:
	friend class Board;
	std::uint64_t positionKeyData = 0;
	std::uint64_t pawnKeyData = 0;
	BoardFlags flagsData;
	Move moveData;
	int fiftyMoveRuleData = 0;
};

class Board {
public:
	Board();
	bool kingInCheck() const;
	bool kingInCheckmate(int color);
	bool kingInStalemate(int color);
	// Takes a generated/pseudolegal move (or the null sentinel), applies it,
	// and rolls it back if it exposes the moving king. Use isLegalMove for
	// arbitrary input; makeMove does not fully validate movement geometry.
	bool makeMove(Move move);
	bool undoMove();
	bool squareUnderAttackBy(int square, int color) const;
	bool isDraw() const;
	bool isPseudoLegalMove(Move m);
	bool isLegalMove(Move m);
	bool loadFen(char const *fen);
	char *moveToStr(char *buffer, Move move);
	// Lists require maxMoves slots; counts exclude the final empty sentinel.
	// King safety is deferred to makeMove, including the castle destination.
	int genPseudolegalMoves(Move *mList) const;
	int genPseudolegalCaptures(Move *mList) const;
	int genPseudolegalNoncaptures(Move *mList) const;
	int genPseudolegalPromotions(Move *mList) const;
	int pseudolegalPieceMoveCount(int color) const;
	Move makeTextMove(char const *text);
	Result result();
	void reset();
	void copy(Board const &s);
	void toStream(FILE *fp) const;
	bool positionIsRepeat();
	bool lastMoveWasCapture();
	bool moveIsCapture(Move move);
	bool nextMoveIsForced();
	int repetitions();
	// Select a legal recapture without changing the board; used by see().
	bool genLvaAttackToSquare(Move &move, int square);
	bool kingInCheck(int color) const;
	bool checkForDraw();
	int pseudolegalMoveCount(Bitboard &control, int color) const;
	int evaluate() const;
	int see(Move move, int score);
	std::uint64_t computePositionKey() const;
	std::uint64_t computePawnKey() const;
	bool loadEgtbIndex(int thirdMan, int index, int sideToMove);
	int threeManEgtbIndex(int thirdMan, int pieceColor) const;
	void assertBoardConsistency(std::source_location const &location =
									std::source_location::current()) const;
	void assertTranspositionKeyConsistency(
		std::source_location const &location =
			std::source_location::current()) const;
	int pieceValue(int square) const noexcept {
		return pieceValuesData[square];
	}
	int pieceCount(int color, int piece) const noexcept {
		return pieceCountsData[color][piece];
	}
	Bitboard pieceBoard(int color, int piece) const noexcept {
		return pieceBoardsData[color][piece];
	}
	RotatedBitboard const &occupiedSquares() const noexcept {
		return occupiedSquaresData;
	}
	std::uint64_t positionKey() const noexcept { return positionKeyData; }
	std::uint64_t pawnKey() const noexcept { return pawnKeyData; }
	BoardFlags const &flags() const noexcept { return flagsData; }
	int fiftyMoveRule() const noexcept { return fiftyMoveRuleData; }
	int sideToMove() const noexcept { return flagsData.sideToMove(); }
	int halfMoveCount() const noexcept { return flagsData.halfMoveCount(); }
	int mostRecentCapture() const noexcept {
		return flagsData.mostRecentCapture();
	}
	bool hasKingsideCastled(int color) const noexcept {
		return hasKsideCastledData[color];
	}
	bool hasQueensideCastled(int color) const noexcept {
		return hasQsideCastledData[color];
	}
	UndoInfo const &undoInfo(int index) const noexcept {
		return undoInfoData[index];
	}
	Move lastMove() const noexcept {
		return halfMoveCount() > 0 ? undoInfoData[halfMoveCount() - 1].move()
								   : Move{};
	}

private:
	struct EmptyState {};
	explicit Board(EmptyState) noexcept {}
	bool isPseudoLegalMoveDebug(Move m);
	int genPseudolegalUnderPromotions(Move *mList) const;
	bool drawByMaterial() const;
	int evaluateMaterial() const;
	bool isCastleMove(Move move);
	bool isEnPassantMove(Move move);
	void adjustRepeatCount();
	UndoInfo getUndoInfo();
	void debugBoardToStream(FILE *fp) const;
	void handleCastle(Move move);
	void handleCastleRightsChanges(Move move);
	void handleCastleUndo(Move m);
	void handleEnPassant(Move move);
	void handleEnPassantUndo(Move move);
	void handlePawnPromotion(Move move);
	void handlePromotionUndo(Move move);
	void storeUndoInfo(UndoInfo const &undo);
	std::array<UndoInfo, maxHalfMoves> undoInfoData{};
	// Slot zero is the aggregate count/bitboard for that color, not empties.
	std::array<std::array<I8, 7>, 2> pieceCountsData{};
	std::array<std::array<Bitboard, 7>, 2> pieceBoardsData{};
	std::array<I8, 64> pieceValuesData{};
	RotatedBitboard occupiedSquaresData{};
	std::uint64_t positionKeyData = 0;
	std::uint64_t pawnKeyData = 0;
	BoardFlags flagsData;
	std::array<bool, 2> hasKsideCastledData{};
	std::array<bool, 2> hasQsideCastledData{};
	int fiftyMoveRuleData = 0;
};

} // namespace morphy_chess
