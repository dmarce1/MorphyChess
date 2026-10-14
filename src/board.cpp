#include <array>
#include <cstddef>
#include <ctype.h>
#include <limits>
#include <memory>
#include <morphychess/bits.hpp>
#include <morphychess/board.hpp>
#include <morphychess/debug.hpp>
#include <morphychess/move_data.hpp>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <morphychess/zobrist.hpp>

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

constexpr auto initUndoSize = 128;
constexpr auto wPawn = pawn;
constexpr auto wKnight = knight;
constexpr auto wBishop = bishop;
constexpr auto wRook = rook;
constexpr auto wQueen = queen;
constexpr auto wKing = king;
constexpr auto bPawn = -pawn;
constexpr auto bKnight = -knight;
constexpr auto bBishop = -bishop;
constexpr auto bRook = -rook;
constexpr auto bQueen = -queen;
constexpr auto bKing = -king;
constexpr auto noEpCapture = 0;

auto storeMove = [](Move *&list, int from, int to, int promo) {
	*list++ = Move(from, to, promo);
};

static constexpr std::array<Bitboard, 2> mayAffectCastle = {
	Bitboard{0x9100000000000000ULL}, Bitboard{0x0000000000000091ULL}};

static int charToPiece(int);

bool Board::checkForDraw() {
	adjustRepeatCount();
	return fiftyMoveRuleData >= 100 || drawByMaterial() ||
		   flagsData.repeatCountValue >= 2;
}

bool Board::isPseudoLegalMoveDebug(Move m) {
	auto &b = *this;
	int rc;
	std::array<Move, maxMoves> list;
	auto listPtr = list.begin();
	b.genPseudolegalMoves(list.data());
	rc = false;
	while (!listPtr->isEmpty()) {
		if (m.from() == listPtr->from() && m.to() == listPtr->to() &&
			m.promo() == listPtr->promo()) {
			rc = true;
		}
		listPtr++;
	}
	return rc;
}

bool Board::isPseudoLegalMove(Move m) {
	auto &b = *this;
	bool legal;
	int fromMan, movingColor, to, from;
	Bitboard ownMen, toBoard;
	legal = false;
	fromMan = b.pieceValuesData[m.from()];
	movingColor = b.flagsData.sideToMoveValue;
	ownMen = b.pieceBoardsData[movingColor][0];
	toBoard = Bitboard{};
	to = m.to();
	from = m.from();
	if (m.isEmpty()) {
		return false;
	}
	if ((((movingColor == white) && (fromMan > 0)) ||
		 ((movingColor == black) && (fromMan < 0))) &&
		(m.isEmpty() == false)) {
		switch (abs(fromMan)) {
		case pawn:
			if (movingColor == white) {
				if (b.isEnPassantMove(m)) {
					if (to == b.flagsData.epCaptureSquareValue + 8) {
						legal = true;
					}
				} else if (b.pieceValuesData[to] == 0) {
					if (to - from == 8) {
						legal = true;
					} else if ((to - from == 16) &&
							   (b.pieceValuesData[to - 8] == 0) &&
							   (from / 8 == 1)) {
						legal = true;
					}
				} else {
					toBoard =
						whitePawnAttacks(from) & b.pieceBoardsData[black][0];
				}
			} else /* movingColor == black */
			{
				if (b.isEnPassantMove(m)) {
					if (to == b.flagsData.epCaptureSquareValue - 8) {
						legal = true;
					}
				} else if (b.pieceValuesData[to] == 0) {
					if (to - from == -8) {
						legal = true;
					} else if ((to - from == -16) &&
							   (b.pieceValuesData[to + 8] == 0) &&
							   (from / 8 == 6)) {
						legal = true;
					}
				} else {
					toBoard =
						blackPawnAttacks(from) & b.pieceBoardsData[white][0];
				}
			}
			break;
		case knight:
			toBoard = knightAttacks(from) & ~ownMen;
			break;
		case bishop:
			toBoard = b.occupiedSquaresData.bishopAttacks(from) & ~ownMen;
			break;
		case rook:
			toBoard = b.occupiedSquaresData.rookAttacks(from) & ~ownMen;
			break;
		case queen:
			toBoard = b.occupiedSquaresData.queenAttacks(from) & ~ownMen;
			break;
		case king:
			if ((from == 4) && (to == 6) && (movingColor == white) &&
				(b.flagsData.whiteKingSideValue == 1)) {
				toBoard = (b.occupiedSquaresData[0] &
						   Bitboard{0x0000000000000060ULL});
				if (!toBoard) {
					if (!b.squareUnderAttackBy(kF1, black) &&
						!b.squareUnderAttackBy(kE1, black)) {
						legal = true;
					}
				}
				toBoard = Bitboard{};
			} else if ((from == 60) && (to == 62) && (movingColor == black) &&
					   (b.flagsData.blackKingSideValue == 1)) {
				toBoard = (b.occupiedSquaresData[0] &
						   Bitboard{0x6000000000000000ULL});
				if (!toBoard) {
					if (!b.squareUnderAttackBy(kF8, white) &&
						!b.squareUnderAttackBy(kE8, white)) {
						legal = true;
					}
				}
				toBoard = Bitboard{};
			} else if ((from == 4) && (to == 2) && (movingColor == white) &&
					   (b.flagsData.whiteQueenSideValue == 1)) {
				toBoard = (b.occupiedSquaresData[0] &
						   Bitboard{0x000000000000000EULL});
				if (!toBoard) {
					if (!b.squareUnderAttackBy(kD1, black) &&
						!b.squareUnderAttackBy(kE1, black)) {
						legal = true;
					}
				}
				toBoard = Bitboard{};
			} else if ((from == 60) && (to == 58) && (movingColor == black) &&
					   (b.flagsData.blackQueenSideValue == 1)) {
				toBoard = (b.occupiedSquaresData[0] &
						   Bitboard{0x0E00000000000000ULL});
				if (!toBoard) {
					if (!b.squareUnderAttackBy(kD8, white) &&
						!b.squareUnderAttackBy(kE8, white)) {
						legal = true;
					}
				}
				toBoard = Bitboard{};
			} else {
				toBoard = kingAttacks(from) & ~ownMen;
			}
			break;
		default:
			assert(false);
		}
		if (legal == false) {
			if (toBoard.test(to) == true) {
				legal = true;
			}
		}
		if (legal == true) {
			if ((abs(fromMan) == pawn) &&
				(((to / 8) == 0) || ((to / 8) == 7))) {
				if ((m.promo() < knight) || (m.promo() > queen)) {
					legal = false;
				}
			} else {
				if (m.promo() != 0) {
					legal = false;
				}
			}
		}
	}
	assert(legal == b.isPseudoLegalMoveDebug(m));
	return legal;
}

bool Board::isLegalMove(Move m) {
	auto &b = *this;
	int rc;
	std::array<Move, maxMoves> list;
	auto listPtr = list.begin();
	b.genPseudolegalMoves(list.data());
	rc = false;
	while (!listPtr->isEmpty()) {
		if (m.from() == listPtr->from() && m.to() == listPtr->to() &&
			m.promo() == listPtr->promo()) {
			if (b.makeMove(m)) {
				b.undoMove();
				rc = true;
			}
			break;
		}
		listPtr++;
	}
	return rc;
}

bool Board::kingInCheckmate(int color) {
	if (color != sideToMove() || !(kingInCheck(color))) {
		return false;
	}
	std::array<Move, maxMoves> moves;
	const int count = genPseudolegalMoves(moves.data());
	for (int index = 0; index < count; ++index) {
		if (makeMove(moves[index])) {
			undoMove();
			return false;
		}
	}
	return true;
}

bool Board::kingInStalemate(int color) {
	if (color != sideToMove() || !(!kingInCheck(color))) {
		return false;
	}
	std::array<Move, maxMoves> moves;
	const int count = genPseudolegalMoves(moves.data());
	for (int index = 0; index < count; ++index) {
		if (makeMove(moves[index])) {
			undoMove();
			return false;
		}
	}
	return true;
}

bool Board::kingInCheck(int color) const {
	auto &b = *this;
	bool rc;
	int square;
	Bitboard bb;
	bb = b.pieceBoardsData[color][king];
	rc = false;
	assert(bb);
	square = bb.firstOne();
	rc = b.squareUnderAttackBy(square, colorFlip(color));
	return rc;
}

bool Board::kingInCheck() const {
	auto &b = *this;
	bool rc;
	rc = (b.flagsData.checkValue == 1) ? true : false;
	return rc;
}

bool Board::isDraw() const {
	auto &b = *this;
	bool rc;
	rc = (b.flagsData.drawValue == 1) ? true : false;
	return rc;
}

bool Board::loadFen(const char *text) {
	if (text == nullptr) {
		return false;
	}
	std::istringstream input(text);
	std::string placement, side, castling, enPassant, trailing;
	int halfmove = 0;
	int fullmove = 1;
	if (!(input >> placement >> side >> castling >> enPassant >> halfmove >>
		  fullmove) ||
		(input >> trailing) || halfmove < 0 || fullmove < 1 ||
		halfmove > 1000000) {
		return false;
	}
	std::unique_ptr<Board> candidateStorage(new Board(EmptyState{}));
	Board &candidate = *candidateStorage;
	int row = 7;
	int column = 0;
	const std::string pieces = "PNBRQKpnbrqk";
	for (char token : placement) {
		if (token == '/') {
			if (column != 8 || row == 0) {
				return false;
			}
			--row;
			column = 0;
		} else if (token >= '1' && token <= '8') {
			column += token - '0';
			if (column > 8) {
				return false;
			}
		} else {
			const auto index = pieces.find(token);
			if (index == std::string::npos || column >= 8) {
				return false;
			}
			const int color = index < 6 ? white : black;
			const int piece = static_cast<int>(index % 6) + 1;
			if (piece == pawn && (row == 0 || row == 7)) {
				return false;
			}
			const int square = 8 * row + column++;
			candidate.pieceValuesData[square] = color == white ? piece : -piece;
			++candidate.pieceCountsData[color][piece];
			++candidate.pieceCountsData[color][0];
			candidate.pieceBoardsData[color][piece].set(square);
			candidate.pieceBoardsData[color][0].set(square);
			candidate.occupiedSquaresData.set(square);
		}
	}
	if (row != 0 || column != 8 || (side != "w" && side != "b")) {
		return false;
	}
	for (int color = black; color <= white; ++color) {
		if (candidate.pieceCount(color, king) != 1 ||
			candidate.pieceCount(color, 0) > 16 ||
			candidate.pieceCount(color, pawn) > 8) {
			return false;
		}
	}
	candidate.flagsData.sideToMoveValue = side == "w" ? white : black;
	if (castling != "-") {
		for (char token : castling) {
			switch (token) {
			case 'K':
				if (candidate.flagsData.whiteKingSideValue) {
					return false;
				}
				candidate.flagsData.whiteKingSideValue = 1;
				break;
			case 'Q':
				if (candidate.flagsData.whiteQueenSideValue) {
					return false;
				}
				candidate.flagsData.whiteQueenSideValue = 1;
				break;
			case 'k':
				if (candidate.flagsData.blackKingSideValue) {
					return false;
				}
				candidate.flagsData.blackKingSideValue = 1;
				break;
			case 'q':
				if (candidate.flagsData.blackQueenSideValue) {
					return false;
				}
				candidate.flagsData.blackQueenSideValue = 1;
				break;
			default:
				return false;
			}
		}
	}
	if ((candidate.flags().whiteKingSide() &&
		 (candidate.pieceValue(kE1) != king ||
		  candidate.pieceValue(kH1) != rook)) ||
		(candidate.flags().whiteQueenSide() &&
		 (candidate.pieceValue(kE1) != king ||
		  candidate.pieceValue(kA1) != rook)) ||
		(candidate.flags().blackKingSide() &&
		 (candidate.pieceValue(kE8) != -king ||
		  candidate.pieceValue(kH8) != -rook)) ||
		(candidate.flags().blackQueenSide() &&
		 (candidate.pieceValue(kE8) != -king ||
		  candidate.pieceValue(kA8) != -rook))) {
		return false;
	}
	if (enPassant != "-") {
		if (enPassant.size() != 2 || enPassant[0] < 'a' || enPassant[0] > 'h' ||
			enPassant[1] != (candidate.sideToMove() == white ? '6' : '3')) {
			return false;
		}
		const int target = 8 * (enPassant[1] - '1') + enPassant[0] - 'a';
		const int captured =
			target + (candidate.sideToMove() == white ? -8 : 8);
		if (candidate.pieceValue(target) != empty ||
			candidate.pieceValue(captured) !=
				(candidate.sideToMove() == white ? -pawn : pawn)) {
			return false;
		}
		candidate.flagsData.epCaptureSquareValue = captured;
	}
	candidate.fiftyMoveRuleData = halfmove;
	candidate.positionKeyData = candidate.computePositionKey();
	candidate.pawnKeyData = candidate.computePawnKey();
	candidate.flagsData.checkValue =
		candidate.kingInCheck(candidate.sideToMove());
	candidate.flagsData.drawValue = candidate.checkForDraw();
	*this = candidate;
	assertBoardConsistency();
	return true;
}

bool Board::nextMoveIsForced() {
	auto &board = *this;
	bool rc;
	std::array<Move, maxMoves> list;
	int i, cnt, legalCnt;

	cnt = board.genPseudolegalMoves(list.data());
	legalCnt = 0;
	for (i = 0; i < cnt; i++) {
		if (board.makeMove(list[i])) {
			legalCnt++;
			board.undoMove();
		}
	}
	if (legalCnt > 1) {
		rc = false;
	} else {
		rc = true;
	}
	return rc;
}

bool Board::makeMove(Move move) {
	if (halfMoveCount() >= maxHalfMoves) {
		return false;
	}
	if (!move.isEmpty() &&
		(move.from() == move.to() || pieceValue(move.from()) == empty ||
		 (pieceValue(move.from()) > 0) != (sideToMove() == white) ||
		 abs(pieceValue(move.to())) == king ||
		 (move.promo() != 0 &&
		  (move.promo() < knight || move.promo() > queen)))) {
		return false;
	}
	auto &b = *this;
	int movingPiece;
	int capturedPiece;
	int movingColor;
	int opposingColor;
	bool rc;
	bool isEp, isCastle;

	isCastle = b.isCastleMove(move);
	isEp = b.isEnPassantMove(move);
	UndoInfo undo;
	undo.positionKeyData = b.positionKeyData;
	undo.pawnKeyData = b.pawnKeyData;
	undo.moveData = move;
	undo.flagsData = b.flagsData;
	undo.fiftyMoveRuleData = b.fiftyMoveRuleData;
	b.storeUndoInfo(undo);
	capturedPiece = abs(b.pieceValuesData[move.to()]);
	movingPiece = abs(b.pieceValuesData[move.from()]);
	movingColor = b.flagsData.sideToMoveValue == white;
	opposingColor = colorFlip(movingColor);
	b.fiftyMoveRuleData++;
	if (!move.isEmpty()) {
		b.pieceBoardsData[movingColor][0].clear(move.from());
		b.pieceBoardsData[movingColor][movingPiece].clear(move.from());
		b.occupiedSquaresData.clear(move.from());
		b.pieceBoardsData[movingColor][0].set(move.to());
		b.pieceBoardsData[movingColor][movingPiece].set(move.to());
		b.occupiedSquaresData.set(move.to());
		b.positionKeyData ^=
			zobristKey(b.pieceValuesData[move.from()], move.to());
		b.positionKeyData ^=
			zobristKey(b.pieceValuesData[move.from()], move.from());
		if ((abs(b.pieceValuesData[move.from()]) == pawn) ||
			(abs(b.pieceValuesData[move.from()]) == king)) {
			b.pawnKeyData ^=
				zobristKey(b.pieceValuesData[move.from()], move.to());
			b.pawnKeyData ^=
				zobristKey(b.pieceValuesData[move.from()], move.from());
		}
		if (capturedPiece != 0) {
			b.fiftyMoveRuleData = 0;
			b.pieceBoardsData[opposingColor][0].clear(move.to());
			b.pieceBoardsData[opposingColor][capturedPiece].clear(move.to());
			b.pieceCountsData[opposingColor][0]--;
			b.pieceCountsData[opposingColor][capturedPiece]--;
			b.flagsData.mostRecentCaptureValue = capturedPiece;
			b.positionKeyData ^=
				zobristKey(b.pieceValuesData[move.to()], move.to());
			if (capturedPiece == pawn) {
				b.pawnKeyData ^=
					zobristKey(b.pieceValuesData[move.to()], move.to());
			}
		} else {
			b.flagsData.mostRecentCaptureValue = empty;
		}
		b.pieceValuesData[move.to()] = b.pieceValuesData[move.from()];
		b.pieceValuesData[move.from()] = 0;

		if (movingPiece == pawn) {
			b.fiftyMoveRuleData = 0;
		}

		if (isCastle) {
			b.handleCastle(move);
		} else if (mayAffectCastle[movingColor].test(move.from()) ||
				   mayAffectCastle[opposingColor].test(move.to())) {
			b.handleCastleRightsChanges(move);
		}

		if (move.promo() != 0) {
			b.handlePawnPromotion(move);
		} else if (isEp) {
			b.handleEnPassant(move);
		}
	}
	if (b.flagsData.epCaptureSquareValue != 0) {
		b.positionKeyData ^= zobristEpKey(b.flagsData.epCaptureSquareValue);
		b.pawnKeyData ^= zobristEpKey(b.flagsData.epCaptureSquareValue);
		b.flagsData.epCaptureSquareValue = noEpCapture;
	}
	if ((movingPiece == pawn) && (abs(move.from() - move.to()) == 16) &&
		!move.isEmpty()) {
		b.flagsData.epCaptureSquareValue = move.to();
		b.positionKeyData ^= zobristEpKey(b.flagsData.epCaptureSquareValue);
		b.pawnKeyData ^= zobristEpKey(b.flagsData.epCaptureSquareValue);
	}
	b.flagsData.sideToMoveValue = opposingColor;
	b.flagsData.halfMoveCountValue++;
	b.flagsData.checkValue = b.kingInCheck(opposingColor) ? 1 : 0;
	b.flagsData.drawValue = b.checkForDraw() ? 1 : 0;
	if (!b.kingInCheck(colorFlip(b.flagsData.sideToMoveValue))) {
		rc = true;
	} else {
		b.undoMove();
		rc = false;
	}
	b.assertBoardConsistency();
	return rc;
}

bool Board::squareUnderAttackBy(int square, int color) const {
	auto &b = *this;
	Bitboard bb1, bb2;
	if (color == black) {

		bb2 = b.pieceBoardsData[black][pawn] & whitePawnAttacks(square);
		bb2 |= b.pieceBoardsData[black][knight] & knightAttacks(square);
		bb2 |= b.pieceBoardsData[black][king] & kingAttacks(square);

		bb1 =
			b.pieceBoardsData[black][queen] | b.pieceBoardsData[black][bishop];
		bb2 |= bb1 & b.occupiedSquaresData.bishopAttacks(square);

		bb1 = b.pieceBoardsData[black][queen] | b.pieceBoardsData[black][rook];
		bb2 |= bb1 & b.occupiedSquaresData.rookAttacks(square);
	} else {
		bb2 = b.pieceBoardsData[white][pawn] & blackPawnAttacks(square);
		bb2 |= b.pieceBoardsData[white][knight] & knightAttacks(square);
		bb2 |= b.pieceBoardsData[white][king] & kingAttacks(square);

		bb1 =
			b.pieceBoardsData[white][queen] | b.pieceBoardsData[white][bishop];
		bb2 |= bb1 & b.occupiedSquaresData.bishopAttacks(square);

		bb1 = b.pieceBoardsData[white][queen] | b.pieceBoardsData[white][rook];
		bb2 |= bb1 & b.occupiedSquaresData.rookAttacks(square);
	}
	return static_cast<bool>(bb2);
}

bool Board::undoMove() {
	auto &b = *this;
	int movingPiece, capturedPiece, movingColor, opposingColor, rc;
	Move move;
	UndoInfo undo;

	if (b.flagsData.halfMoveCountValue != 0) {
		rc = true;
		capturedPiece = b.flagsData.mostRecentCaptureValue;
		undo = b.getUndoInfo();
		b.flagsData = undo.flagsData;
		b.fiftyMoveRuleData = undo.fiftyMoveRuleData;
		b.positionKeyData = undo.positionKeyData;
		b.pawnKeyData = undo.pawnKeyData;
		move = undo.moveData;
		if (!move.isEmpty()) {
			opposingColor = colorFlip(b.flagsData.sideToMoveValue);
			;
			movingColor = b.flagsData.sideToMoveValue;
			movingPiece = abs(b.pieceValuesData[move.to()]);
			b.pieceBoardsData[movingColor][0].set(move.from());
			b.pieceBoardsData[movingColor][movingPiece].set(move.from());
			b.occupiedSquaresData.set(move.from());
			b.pieceBoardsData[movingColor][0].clear(move.to());
			b.pieceBoardsData[movingColor][movingPiece].clear(move.to());
			b.occupiedSquaresData.clear(move.to());
			b.pieceValuesData[move.from()] = b.pieceValuesData[move.to()];
			b.pieceValuesData[move.to()] = 0;
			if (capturedPiece != 0) {
				b.pieceBoardsData[opposingColor][0].set(move.to());
				b.pieceBoardsData[opposingColor][capturedPiece].set(move.to());
				b.occupiedSquaresData.set(move.to());
				b.pieceCountsData[opposingColor][0]++;
				b.pieceCountsData[opposingColor][capturedPiece]++;
				b.pieceValuesData[move.to()] =
					capturedPiece * ((opposingColor == white) ? 1 : -1);
			}
			if (b.isCastleMove(move)) {
				b.handleCastleUndo(move);
			} else if (move.promo() != 0) {
				b.handlePromotionUndo(move);
			} else if (b.isEnPassantMove(move)) {
				b.handleEnPassantUndo(move);
			}
		}
	} else {
		rc = false;
	}
	b.assertBoardConsistency();
	return rc;
}

char *Board::moveToStr(char *buffer, Move move) {
	auto &b = *this;
	const std::array<char, 7> pieceChars = {'\0', '\0', 'N', 'B',
											'R',  'Q',	'K'};
	const int fromRank = move.from() / 8;
	const int fromFile = move.from() % 8;
	const int toRank = move.to() / 8;
	const int toFile = move.to() % 8;
	const int piece = abs(b.pieceValuesData[move.from()]);
	bool isEp;
	char *start = buffer;

	isEp = b.isEnPassantMove(move);
	if (b.isCastleMove(move)) {
		if (move.from() > move.to()) {
			strcpy(buffer, "O-O-O");
		} else {
			strcpy(buffer, "O-O");
		}
		buffer += strlen(buffer);
	} else {
		if (piece != pawn) {
			*buffer = pieceChars[piece];
			buffer++;
		}
		*buffer = 'a' + fromFile;
		buffer++;
		*buffer = '1' + fromRank;
		buffer++;
		if ((b.pieceValuesData[move.to()] != 0) || b.isEnPassantMove(move)) {
			*buffer = 'x';
		} else {
			*buffer = '-';
		}
		buffer++;
		*buffer = 'a' + toFile;
		buffer++;
		*buffer = '1' + toRank;
		buffer++;
		if (move.promo() != 0) {
			*buffer = '=';
			buffer++;
			*buffer = pieceChars[move.promo()];
			buffer++;
		}
	}

	if (isEp) {
		strcpy(buffer, "e.p.");
		buffer += 4;
	}
	if (b.makeMove(move)) {
		if (b.flagsData.checkValue == 1) {
			if (b.kingInCheckmate(white) || b.kingInCheckmate(black)) {
				*buffer = '#';
			} else {
				*buffer = '+';
			}
			buffer++;
		}
		b.undoMove();
	}
	*buffer = '\0';
	return start;
}

int Board::genPseudolegalMoves(Move *mList) const {
	auto &b = *this;
	int moveCnt;

	moveCnt = 0;
	moveCnt += b.genPseudolegalPromotions(mList + moveCnt);
	moveCnt += b.genPseudolegalCaptures(mList + moveCnt);
	moveCnt += b.genPseudolegalNoncaptures(mList + moveCnt);

	return moveCnt;
}

int Board::genPseudolegalCaptures(Move *mList) const {
	auto &b = *this;
	const Move *startPtr = mList;
	int attackColor, defendColor, from, to;
	Bitboard toBoard, fromBoard;

	assert(mList != NULL);

	attackColor = b.flagsData.sideToMoveValue;
	defendColor = colorFlip(attackColor);

	if (attackColor == white) {
		/* White pawn captures */
		toBoard = b.pieceBoardsData[white][pawn] << 7;
		toBoard &= b.pieceBoardsData[black][0];
		toBoard &= ~rank8;
		toBoard &= ~fileH;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 7, to, 0);
		}
		toBoard = b.pieceBoardsData[white][pawn] << 9;
		toBoard &= b.pieceBoardsData[black][0];
		toBoard &= ~rank8;
		toBoard &= ~fileA;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 9, to, 0);
		}
		/* ep captures */
		if (b.flagsData.epCaptureSquareValue != noEpCapture) {
			to = b.flagsData.epCaptureSquareValue + 8;
			from = to - 7;
			if ((from % 8 != 0) && b.pieceBoardsData[white][pawn].test(from)) {
				storeMove(mList, from, to, 0);
			}
			from = to - 9;
			if ((from % 8 != 7) && b.pieceBoardsData[white][pawn].test(from)) {
				storeMove(mList, from, to, 0);
			}
		}
	} else {
		/********* BLACK PAWN Captures ********************/
		toBoard = b.pieceBoardsData[black][pawn] >> 7;
		toBoard &= b.pieceBoardsData[white][0];
		toBoard &= ~rank1;
		toBoard &= ~fileA;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 7, to, 0);
		}
		toBoard = b.pieceBoardsData[black][pawn] >> 9;
		toBoard &= b.pieceBoardsData[white][0];
		toBoard &= ~rank1;
		toBoard &= ~fileH;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 9, to, 0);
		}
		/* ep move */
		if (b.flagsData.epCaptureSquareValue != noEpCapture) {
			to = b.flagsData.epCaptureSquareValue - 8;
			from = to + 7;
			if ((from % 8 != 7) && b.pieceBoardsData[black][pawn].test(from)) {
				storeMove(mList, from, to, 0);
			}
			from = to + 9;
			if ((from % 8 != 0) && b.pieceBoardsData[black][pawn].test(from)) {
				storeMove(mList, from, to, 0);
			}
		}
	}
	/********* KNIGHT captures********************/
	fromBoard = b.pieceBoardsData[attackColor][knight];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = knightAttacks(from) & b.pieceBoardsData[defendColor][0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	/********* BISHOP captures********************/
	fromBoard = b.pieceBoardsData[attackColor][bishop];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.bishopAttacks(from) &
				  b.pieceBoardsData[defendColor][0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	/********* ROOK captures********************/
	fromBoard = b.pieceBoardsData[attackColor][rook];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.rookAttacks(from) &
				  b.pieceBoardsData[defendColor][0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	fromBoard = b.pieceBoardsData[attackColor][queen];
	/********* QUEEN  captures ********************/
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.queenAttacks(from) &
				  b.pieceBoardsData[defendColor][0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	/************ KING captures ************************/
	fromBoard = b.pieceBoardsData[attackColor][king];
	from = fromBoard.firstOne();
	toBoard = kingAttacks(from) & b.pieceBoardsData[defendColor][0];
	while (toBoard) {
		to = toBoard.removeFirstOne();
		storeMove(mList, from, to, 0);
	}
	storeMove(mList, 0, 0, 1);
	return (int)(mList - startPtr) - 1;
}

int Board::pseudolegalMoveCount(Bitboard &control, int color) const {
	auto &b = *this;
	int attackColor, from, count;
	Bitboard toBoard, fromBoard, tempBoard;

	count = 0;
	attackColor = color;
	control = Bitboard{};
	if (attackColor == white) {
		toBoard = b.pieceBoardsData[white][pawn] << 8;
		toBoard &= ~b.occupiedSquaresData[0];
		tempBoard = toBoard;
		count += toBoard.countOnes();
		toBoard = tempBoard << 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank4;
		count += toBoard.countOnes();
		toBoard = b.pieceBoardsData[white][pawn] << 7;
		toBoard &= ~fileH;
		toBoard &= b.pieceBoardsData[black][0];
		control |= toBoard;
		count += toBoard.countOnes();
		toBoard = b.pieceBoardsData[white][pawn] << 9;
		toBoard &= ~fileA;
		toBoard &= b.pieceBoardsData[black][0];
		control |= toBoard;
		count += toBoard.countOnes();
	} else {
		toBoard = b.pieceBoardsData[black][pawn] >> 8;
		toBoard &= ~b.occupiedSquaresData[0];
		tempBoard = toBoard;
		count += toBoard.countOnes();
		toBoard = tempBoard >> 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank5;
		count += toBoard.countOnes();
		toBoard = b.pieceBoardsData[black][pawn] >> 7;
		toBoard &= ~fileA;
		toBoard &= b.pieceBoardsData[white][0];
		control |= toBoard;
		count += toBoard.countOnes();
		toBoard = b.pieceBoardsData[black][pawn] >> 9;
		toBoard &= ~fileH;
		toBoard &= b.pieceBoardsData[white][0];
		control |= toBoard;
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][knight];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = knightAttacks(from);
		control |= toBoard;
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][bishop];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.bishopAttacks(from);
		control |= toBoard;
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][rook];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.rookAttacks(from);
		control |= toBoard;
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][queen];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.queenAttacks(from);
		control |= toBoard;
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][king];
	from = fromBoard.firstOne();
	toBoard = kingAttacks(from);
	control |= toBoard;
	toBoard &= ~b.pieceBoardsData[attackColor][0];
	count += toBoard.countOnes();
	return count;
}

int Board::pseudolegalPieceMoveCount(int color) const {
	auto &b = *this;
	int attackColor, from, count;
	Bitboard toBoard, fromBoard;

	count = 0;
	attackColor = color;
	fromBoard = b.pieceBoardsData[attackColor][knight];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = knightAttacks(from);
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][bishop];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.bishopAttacks(from);
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][rook];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.rookAttacks(from);
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][queen];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.queenAttacks(from);
		toBoard &= ~b.pieceBoardsData[attackColor][0];
		count += toBoard.countOnes();
	}
	fromBoard = b.pieceBoardsData[attackColor][king];
	from = fromBoard.firstOne();
	toBoard = kingAttacks(from);
	toBoard &= ~b.pieceBoardsData[attackColor][0];
	count += toBoard.countOnes();
	return count;
}

int Board::genPseudolegalNoncaptures(Move *mList) const {
	auto &b = *this;
	const Move *startPtr = mList;
	int attackColor, from, to, cnt;
	Bitboard toBoard, fromBoard, tempBoard;

	assert(mList != NULL);

	attackColor = b.flagsData.sideToMoveValue;

	if (attackColor == white) {
		/********* WHITE PAWN MOVES ********************/
		toBoard = b.pieceBoardsData[white][pawn] << 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= ~rank8;
		tempBoard = toBoard;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 8, to, 0);
		}
		toBoard = tempBoard << 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank4;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 16, to, 0);
		}
		/*********** WHITE CASTLE MOVES ******************************/
		if (b.flagsData.whiteKingSideValue == 1) {
			tempBoard =
				(b.occupiedSquaresData[0] & Bitboard{0x0000000000000060ULL});
			if (!tempBoard) {
				if (!b.squareUnderAttackBy(kF1, black) &&
					!b.squareUnderAttackBy(kE1, black)) {
					storeMove(mList, kE1, kG1, 0);
				}
			}
		}
		if (b.flagsData.whiteQueenSideValue == 1) {
			tempBoard =
				(b.occupiedSquaresData[0] & Bitboard{0x000000000000000EULL});
			if (!tempBoard) {
				if (!b.squareUnderAttackBy(kD1, black) &&
					!b.squareUnderAttackBy(kE1, black)) {
					storeMove(mList, kE1, kC1, 0);
				}
			}
		}
	} else {
		/********* BLACK PAWN MOVES ********************/
		assert(attackColor == black);
		toBoard = b.pieceBoardsData[black][pawn] >> 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= ~rank1;
		tempBoard = toBoard;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 8, to, 0);
		}
		toBoard = tempBoard >> 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank5;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 16, to, 0);
		}
		/*********** BLACK CASTLE MOVES ******************************/
		if (b.flagsData.blackKingSideValue == 1) {
			tempBoard =
				(b.occupiedSquaresData[0] & Bitboard{0x6000000000000000ULL});
			if (!tempBoard) {
				if (!b.squareUnderAttackBy(kF8, white) &&
					!b.squareUnderAttackBy(kE8, white)) {
					storeMove(mList, kE8, kG8, 0);
				}
			}
		}
		if (b.flagsData.blackQueenSideValue == 1) {
			tempBoard =
				(b.occupiedSquaresData[0] & Bitboard{0x0E00000000000000ULL});
			if (!tempBoard) {
				if (!b.squareUnderAttackBy(kD8, white) &&
					!b.squareUnderAttackBy(kE8, white)) {
					storeMove(mList, kE8, kC8, 0);
				}
			}
		}
	}
	/********* KNIGHT MOVES ********************/
	fromBoard = b.pieceBoardsData[attackColor][knight];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = knightAttacks(from) & ~b.occupiedSquaresData[0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	/********* BISHOP MOVES ********************/
	fromBoard = b.pieceBoardsData[attackColor][bishop];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.bishopAttacks(from) &
				  ~b.occupiedSquaresData[0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	/********* ROOK MOVES ********************/
	fromBoard = b.pieceBoardsData[attackColor][rook];
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard =
			b.occupiedSquaresData.rookAttacks(from) & ~b.occupiedSquaresData[0];
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	fromBoard = b.pieceBoardsData[attackColor][queen];
	/********* QUEEN MOVES ********************/
	while (fromBoard) {
		from = fromBoard.removeFirstOne();
		toBoard = b.occupiedSquaresData.queenAttacks(from) &
				  ~b.occupiedSquaresData[0];
		;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, from, to, 0);
		}
	}
	/************ KING MOVES ************************/
	fromBoard = b.pieceBoardsData[attackColor][king];
	from = fromBoard.firstOne();
	toBoard = kingAttacks(from) & ~b.occupiedSquaresData[0];
	;
	while (toBoard) {
		to = toBoard.removeFirstOne();
		storeMove(mList, from, to, 0);
	}
	cnt = (int)(mList - startPtr);
	/* under promotions */
	cnt += b.genPseudolegalUnderPromotions(mList);

	return cnt;
}

int Board::genPseudolegalPromotions(Move *mList) const {
	auto &b = *this;
	const Move *startPtr = mList;
	int attackColor, to;
	Bitboard toBoard;

	assert(mList != NULL);

	attackColor = b.flagsData.sideToMoveValue;

	if (attackColor == white) {
		/* White pawn promotions */
		toBoard = b.pieceBoardsData[white][pawn] << 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank8;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 8, to, queen);
		}
		toBoard = b.pieceBoardsData[white][pawn] << 7;
		toBoard &= b.pieceBoardsData[black][0];
		toBoard &= rank8;
		toBoard &= ~fileH;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 7, to, queen);
		}
		toBoard = b.pieceBoardsData[white][pawn] << 9;
		toBoard &= b.pieceBoardsData[black][0];
		toBoard &= rank8;
		toBoard &= ~fileA;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 9, to, queen);
		}
	} else {
		/* Black pawn promotions */
		toBoard = b.pieceBoardsData[black][pawn] >> 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank1;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 8, to, queen);
		}
		toBoard = b.pieceBoardsData[black][pawn] >> 7;
		toBoard &= b.pieceBoardsData[white][0];
		toBoard &= rank1;
		toBoard &= ~fileA;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 7, to, queen);
		}
		toBoard = b.pieceBoardsData[black][pawn] >> 9;
		toBoard &= b.pieceBoardsData[white][0];
		toBoard &= rank1;
		toBoard &= ~fileH;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 9, to, queen);
		}
	}
	storeMove(mList, 0, 0, 1);
	return (int)(mList - startPtr) - 1;
}

int Board::genPseudolegalUnderPromotions(Move *mList) const {
	auto &b = *this;
	const Move *startPtr = mList;
	int attackColor, to;
	Bitboard toBoard;

	assert(mList != NULL);

	attackColor = b.flagsData.sideToMoveValue;

	if (attackColor == white) {
		/* White pawn promotions */
		toBoard = b.pieceBoardsData[white][pawn] << 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank8;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 8, to, rook);
			storeMove(mList, to - 8, to, bishop);
			storeMove(mList, to - 8, to, knight);
		}
		toBoard = b.pieceBoardsData[white][pawn] << 7;
		toBoard &= b.pieceBoardsData[black][0];
		toBoard &= rank8;
		toBoard &= ~fileH;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 7, to, rook);
			storeMove(mList, to - 7, to, bishop);
			storeMove(mList, to - 7, to, knight);
		}
		toBoard = b.pieceBoardsData[white][pawn] << 9;
		toBoard &= b.pieceBoardsData[black][0];
		toBoard &= rank8;
		toBoard &= ~fileA;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to - 9, to, rook);
			storeMove(mList, to - 9, to, bishop);
			storeMove(mList, to - 9, to, knight);
		}
	} else {
		/* Black pawn promotions */
		toBoard = b.pieceBoardsData[black][pawn] >> 8;
		toBoard &= ~b.occupiedSquaresData[0];
		toBoard &= rank1;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 8, to, rook);
			storeMove(mList, to + 8, to, bishop);
			storeMove(mList, to + 8, to, knight);
		}
		toBoard = b.pieceBoardsData[black][pawn] >> 7;
		toBoard &= b.pieceBoardsData[white][0];
		toBoard &= rank1;
		toBoard &= ~fileA;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 7, to, rook);
			storeMove(mList, to + 7, to, bishop);
			storeMove(mList, to + 7, to, knight);
		}
		toBoard = b.pieceBoardsData[black][pawn] >> 9;
		toBoard &= b.pieceBoardsData[white][0];
		toBoard &= rank1;
		toBoard &= ~fileH;
		while (toBoard) {
			to = toBoard.removeFirstOne();
			storeMove(mList, to + 9, to, rook);
			storeMove(mList, to + 9, to, bishop);
			storeMove(mList, to + 9, to, knight);
		}
	}
	storeMove(mList, 0, 0, 1);
	return (int)(mList - startPtr) - 1;
}

Move Board::makeTextMove(const char *text) {
	std::array<char, 32> mt;
	if (text == nullptr || strlen(text) >= mt.size()) {
		return {};
	}
	strcpy(mt.data(), text);
	auto &b = *this;
	Move m = {0, 0, 0};
	auto ptr = mt.begin();
	m.setPromo(empty);
	std::array<Move, maxMoves> moves;
	int i, piece;
	int fr, tr, ff, tf;
	bool matchFound;

	for (size_t index = 0; index < strlen(mt.data()); ++index) {
		if (mt[index] == '\n') {
			mt[index] = '\0';
			break;
		}
	}
	if (strcmp(mt.data(), "O-O") == 0) {
		if (b.flagsData.sideToMoveValue == white) {
			m.setFrom(kE1);
			m.setTo(kG1);
		} else {
			m.setFrom(kE8);
			m.setTo(kG8);
		}
		matchFound = true;
	} else if (strcmp(mt.data(), "O-O-O") == 0) {
		if (b.flagsData.sideToMoveValue == white) {
			m.setFrom(kE1);
			m.setTo(kC1);
		} else {
			m.setFrom(kE8);
			m.setTo(kC8);
		}
		matchFound = true;
	}

	else {
		b.genPseudolegalMoves(moves.data());
		if (isupper(*ptr)) {
			piece = charToPiece(*ptr);
			ptr++;
		} else {
			piece = pawn;
		}
		ff = fr = tf = tr = -1;
		for (i = 0; (i < 5) && (*ptr != '\0') && (*ptr != '='); i++) {
			if (isalpha(*ptr) && *ptr != 'x') {
				if (tf == -1) {
					tf = *ptr - 'a';
				} else {
					ff = tf;
					tf = *ptr - 'a';
				}
			} else if (isdigit(*ptr)) {
				if (tr == -1) {
					tr = *ptr - '1';
				} else {
					fr = tr;
					tr = *ptr - '1';
				}
			}
			ptr++;
		}
		if (*ptr == '=') {
			ptr++;
			m.setPromo(charToPiece(*ptr));
		} else {
			m.setPromo(0);
		}
		m.setTo(8 * tr + tf);
		if ((fr == -1) || (ff == -1)) {
			matchFound = false;
			for (i = 0; !moves[i].isEmpty(); i++) {

				if ((moves[i].to() == m.to()) &&
					(moves[i].promo() == m.promo()) &&
					(abs(b.pieceValuesData[moves[i].from()]) == piece) &&
					isLegalMove(moves[i])) {
					if ((fr == -1) && (ff == -1)) {
						m.setFrom(moves[i].from());
						matchFound = true;
						break;
					}
					if ((ff == -1) && (fr != -1)) {
						if (moves[i].from() / 8 == fr) {
							m.setFrom(moves[i].from());
							matchFound = true;
							break;
						}
					}
					if ((ff != -1) && (fr == -1)) {
						if (moves[i].from() % 8 == ff) {
							m.setFrom(moves[i].from());
							matchFound = true;
							break;
						}
					}
				}
			}
		} else {
			m.setFrom(8 * fr + ff);
			matchFound = true;
		}
	}
	if (matchFound && b.isLegalMove(m)) {
		b.makeMove(m);
	} else {
		m.setEmpty();
	}
	return m;
}

std::string Move::coordinateString() const {
	if (isEmpty()) {
		return {};
	}
	std::string text;
	text += static_cast<char>('a' + from() % 8);
	text += static_cast<char>('1' + from() / 8);
	text += static_cast<char>('a' + to() % 8);
	text += static_cast<char>('1' + to() / 8);
	switch (promo()) {
	case queen:
		text += 'q';
		break;
	case rook:
		text += 'r';
		break;
	case bishop:
		text += 'b';
		break;
	case knight:
		text += 'n';
		break;
	default:
		break;
	}
	return text;
}

Move Move::fromString(const char *text) {
	if (text == nullptr) {
		return {};
	}
	std::string str(text);
	if (str.size() >= 5 &&
		std::isupper(static_cast<unsigned char>(str.front()))) {
		str.erase(0, 1);
	}
	if (str.size() > 2 && (str[2] == 'x' || str[2] == '-')) {
		str.erase(2, 1);
	}
	if (str.size() < 4 || str.size() > 6 || str[0] < 'a' || str[0] > 'h' ||
		str[2] < 'a' || str[2] > 'h' || str[1] < '1' || str[1] > '8' ||
		str[3] < '1' || str[3] > '8') {
		return {};
	}
	int promotion = 0;
	if (str.size() > 4) {
		const size_t index = str[4] == '=' ? 5 : 4;
		if (str.size() != index + 1) {
			return {};
		}
		switch (std::tolower(static_cast<unsigned char>(str[index]))) {
		case 'q':
			promotion = queen;
			break;
		case 'r':
			promotion = rook;
			break;
		case 'b':
			promotion = bishop;
			break;
		case 'n':
			promotion = knight;
			break;
		default:
			return {};
		}
	}
	return Move(8 * (str[1] - '1') + str[0] - 'a',
				8 * (str[3] - '1') + str[2] - 'a', promotion);
}

Result Board::result() {
	if (kingInCheckmate(sideToMove())) {
		return sideToMove() == white ? blackWin : whiteWin;
	}
	if (isDraw() || kingInStalemate(sideToMove())) {
		return draw;
	}
	return inProgress;
}

void Board::copy(const Board &source) { *this = source; }

Board::Board() {
	initMoveData();
	initZobristKeys();
	reset();
}

void Board::reset() { loadFen(initialPosition); }

void Board::toStream(FILE *fp) const {
	auto &b = *this;
	int rank, file, i;
	const std::array<char, 13> pieceChar = {'k', 'q', 'r', 'b', 'n', 'o', ' ',
											'x', 'N', 'B', 'R', 'Q', 'K'};
	char c;

	if (debugMode) {
		b.debugBoardToStream(fp);
	} else {
		for (rank = 7; rank >= 0; rank--) {
			fprintf(fp, "----------------------------------\n");
			fprintf(fp, "%i", rank + 1);
			for (file = 0; file <= 7; file++) {
				i = 8 * rank + file;
				c = pieceChar[b.pieceValuesData[i] + 6];
				fprintf(fp, "| %c ", c);
			}
			fprintf(fp, "|\n");
		}
		fprintf(fp, "----------------------------------\n");
		fprintf(fp, "%c| a | b | c | d | e | f | g | h |\n",
				(b.flagsData.sideToMoveValue == white) ? 'W' : 'B');
	}
	fprintf(fp, "#%i\n", (b.flagsData.halfMoveCountValue / 2) + 1);
}

bool Board::drawByMaterial() const {
	auto &b = *this;
	bool rc;
	rc = false;
	if ((b.pieceCountsData[white][0] <= 2) &&
		(b.pieceCountsData[black][0] <= 2)) {
		if ((b.pieceCountsData[white][0] == 1) &&
			(b.pieceCountsData[black][0] == 1)) {
			rc = true;
		} else if (b.pieceCountsData[white][0] == 1) {
			if ((b.pieceCountsData[black][knight] == 1) ||
				(b.pieceCountsData[black][bishop] == 1)) {
				rc = true;
			}
		} else if (b.pieceCountsData[black][0] == 1) {
			if ((b.pieceCountsData[white][knight] == 1) ||
				(b.pieceCountsData[white][bishop] == 1)) {
				rc = true;
			}
		} else {
			if ((b.pieceCountsData[white][bishop] == 1) &&
				(b.pieceCountsData[black][bishop] == 1)) {
				if (!(((b.pieceBoardsData[white][bishop] & whiteSquares) &&
					   (b.pieceBoardsData[black][bishop] & blackSquares)) ||
					  ((b.pieceBoardsData[white][bishop] & blackSquares) &&
					   (b.pieceBoardsData[black][bishop] & whiteSquares)))) {
					rc = true;
				}
			}
		}
	}
	return rc;
}

bool Board::isCastleMove(Move move) {
	auto &b = *this;
	int rc = 0;

	if ((abs(b.pieceValuesData[move.from()]) == king) &&
		(abs(move.from() - move.to()) == 2)) {
		rc = true;
	} else {
		rc = false;
	}
	return rc;
}

bool Board::isEnPassantMove(Move move) {
	auto &b = *this;
	int rc;
	rc = false;
	if ((abs(b.pieceValuesData[move.from()]) == pawn) &&
		(b.pieceValuesData[move.to()] == empty)) {
		if (b.flagsData.sideToMoveValue == white) {
			if (((move.to() - move.from()) == 7) ||
				((move.to() - move.from()) == 9)) {
				rc = true;
			}
		} else {
			if (((move.to() - move.from()) == -7) ||
				((move.to() - move.from()) == -9)) {
				rc = true;
			}
		}
	}

	return rc;
}

bool Board::positionIsRepeat() {
	auto &b = *this;
	return (bool)(b.flagsData.repeatCountValue > 0);
}

void Board::adjustRepeatCount() {
	auto &b = *this;
	int i, start, stop;
	std::uint64_t thisKey;

	b.flagsData.repeatCountValue = 0;
	start = b.flagsData.halfMoveCountValue - 4;
	stop = b.flagsData.halfMoveCountValue - b.fiftyMoveRuleData;
	thisKey = b.positionKeyData /* & REMOVE_REPEAT_KEY*/;
	if (start >= 0) {
		stop = (stop >= 0) ? stop : 0;
		for (i = start; i >= stop; i -= 2) {
			if (thisKey ==
				(b.undoInfoData[i].positionKeyData /*& REMOVE_REPEAT_KEY*/)) {
				b.flagsData.repeatCountValue++;
				if (b.flagsData.repeatCountValue >= 2) {
					break;
				}
			}
		}
	}
}

static int charToPiece(int c) {
	int piece;
	switch (c) {
	case 'K':
		piece = king;
		break;
	case 'Q':
		piece = queen;
		break;
	case 'R':
		piece = rook;
		break;
	case 'B':
		piece = bishop;
		break;
	case 'N':
		piece = knight;
		break;
	default:
		piece = pawn;
		break;
	}
	return piece;
}

UndoInfo Board::getUndoInfo() {
	auto &b = *this;
	UndoInfo u;
	u = b.undoInfoData[b.flagsData.halfMoveCountValue - 1];
	return u;
}

void Board::debugBoardToStream(FILE *fp) const {
	auto &b = *this;
	int rank, file, i, j;
	const std::array<char, 13> pieceChar = {'k', 'q', 'r', 'b', 'n', 'p', ' ',
											'P', 'N', 'B', 'R', 'Q', 'K'};
	char c;

	rank = 7;
	fprintf(fp, "----------------------------------\n");
	fprintf(fp, "%i", rank + 1);
	for (file = 0; file <= 7; file++) {
		i = 8 * rank + file;
		c = pieceChar[b.pieceValuesData[i] + 6];
		fprintf(fp, "| %c ", c);
	}
	fprintf(fp, "|            white           black\n");
	j = 6;
	for (rank = 6; rank > 3; rank--) {
		fprintf(fp, "---------------------------------");
		fprintf(fp, "| %c: %016llx  %016llx\n", pieceChar[6 + j],
				static_cast<unsigned long long>(
					b.pieceBoardsData[white][j].value()),
				static_cast<unsigned long long>(
					b.pieceBoardsData[black][j].value()));
		j--;
		fprintf(fp, "%i", rank + 1);
		for (file = 0; file <= 7; file++) {
			i = 8 * rank + file;
			c = pieceChar[b.pieceValuesData[i] + 6];
			fprintf(fp, "| %c ", c);
		}
		fprintf(fp, "| %c: %016llx  %016llx\n", pieceChar[6 + j],
				static_cast<unsigned long long>(
					b.pieceBoardsData[white][j].value()),
				static_cast<unsigned long long>(
					b.pieceBoardsData[black][j].value()));
		j--;
	}
	for (rank = 3; rank >= 0; rank--) {
		fprintf(fp, "---------------------------------");
		if (rank == 3) {
			fprintf(fp, "| %c: %016llx  %016llx\n", pieceChar[6 + j],
					static_cast<unsigned long long>(
						b.pieceBoardsData[white][j].value()),
					static_cast<unsigned long long>(
						b.pieceBoardsData[black][j].value()));
		} else if (rank == 2) {
			fprintf(fp, "|%016llx %016llx %016llx %016llx\n",
					static_cast<unsigned long long>(
						b.occupiedSquaresData[0].value()),
					static_cast<unsigned long long>(
						b.occupiedSquaresData[1].value()),
					static_cast<unsigned long long>(
						b.occupiedSquaresData[2].value()),
					static_cast<unsigned long long>(
						b.occupiedSquaresData[3].value()));
		} else if (rank == 1) {
			fprintf(
				fp, "|   %i, %i, %i, %i, %i, %i <> %i, %i, %i, %i, %i, %i\n",
				b.pieceCountsData[white][pawn],
				b.pieceCountsData[white][knight],
				b.pieceCountsData[white][bishop],
				b.pieceCountsData[white][rook], b.pieceCountsData[white][queen],
				b.pieceCountsData[white][king], b.pieceCountsData[black][pawn],
				b.pieceCountsData[black][knight],
				b.pieceCountsData[black][bishop],
				b.pieceCountsData[black][rook], b.pieceCountsData[black][queen],
				b.pieceCountsData[black][king]);
		} else if (rank == 0) {
			fprintf(fp, "|          %5i         %5i\n",
					b.flagsData.halfMoveCountValue, b.fiftyMoveRuleData);
		}
		fprintf(fp, "%i", rank + 1);
		for (file = 0; file <= 7; file++) {
			i = 8 * rank + file;
			c = pieceChar[b.pieceValuesData[i] + 6];
			fprintf(fp, "| %c ", c);
		}
		fprintf(fp, "|\n");
	}
	fprintf(fp, "---------------------------------");
	fprintf(fp, "|0x%016llx        %i%i%i%i          %i\n",
			static_cast<unsigned long long>(b.positionKeyData),
			b.flagsData.whiteKingSideValue, b.flagsData.whiteQueenSideValue,
			b.flagsData.blackKingSideValue, b.flagsData.blackQueenSideValue,
			b.flagsData.epCaptureSquareValue);
	fprintf(fp, "%c| a | b | c | d | e | f | g | h |    %c\n",
			(b.flagsData.sideToMoveValue == white) ? 'W' : 'B',
			pieceChar[b.flagsData.mostRecentCaptureValue + 6]);
}

void Board::handleCastle(Move move) {
	auto &b = *this;
	std::uint64_t keyChange = 0;
	if (b.flagsData.sideToMoveValue == white) {
		keyChange =
			b.flagsData.whiteKingSideValue * zobristKingSideCastleKey(white);
		keyChange ^=
			b.flagsData.whiteQueenSideValue * zobristQueenSideCastleKey(white);
		b.flagsData.whiteKingSideValue = b.flagsData.whiteQueenSideValue = 0;
		/* King Side */
		if (move.from() < move.to()) {
			b.occupiedSquaresData.clear(kH1);
			b.pieceBoardsData[white][0].clear(kH1);
			b.pieceBoardsData[white][rook].clear(kH1);
			b.pieceValuesData[kH1] = 0;
			b.occupiedSquaresData.set(kF1);
			b.pieceBoardsData[white][0].set(kF1);
			b.pieceBoardsData[white][rook].set(kF1);
			b.pieceValuesData[kF1] = rook;
			keyChange ^= zobristKey(rook, kH1);
			keyChange ^= zobristKey(rook, kF1);
			b.hasKsideCastledData[white] = 1;
		}
		/* Qside */
		else {
			b.occupiedSquaresData.clear(kA1);
			b.pieceBoardsData[white][0].clear(kA1);
			b.pieceBoardsData[white][rook].clear(kA1);
			b.pieceValuesData[kA1] = 0;
			b.occupiedSquaresData.set(kD1);
			b.pieceBoardsData[white][0].set(kD1);
			b.pieceBoardsData[white][rook].set(kD1);
			b.pieceValuesData[kD1] = rook;
			keyChange ^= zobristKey(rook, kA1);
			keyChange ^= zobristKey(rook, kD1);
			b.hasQsideCastledData[white] = 1;
		}
	} else {
		keyChange =
			b.flagsData.blackKingSideValue * zobristKingSideCastleKey(black);
		keyChange ^=
			b.flagsData.blackQueenSideValue * zobristQueenSideCastleKey(black);
		b.flagsData.blackKingSideValue = b.flagsData.blackQueenSideValue = 0;
		if (move.from() < move.to()) {
			b.occupiedSquaresData.clear(kH8);
			b.pieceBoardsData[black][0].clear(kH8);
			b.pieceBoardsData[black][rook].clear(kH8);
			b.pieceValuesData[kH8] = 0;
			b.occupiedSquaresData.set(kF8);
			b.pieceBoardsData[black][0].set(kF8);
			b.pieceBoardsData[black][rook].set(kF8);
			b.pieceValuesData[kF8] = -rook;
			keyChange ^= zobristKey(-rook, kH8);
			keyChange ^= zobristKey(-rook, kF8);
			b.hasKsideCastledData[black] = 1;
		} else {
			b.occupiedSquaresData.clear(kA8);
			b.pieceBoardsData[black][0].clear(kA8);
			b.pieceBoardsData[black][rook].clear(kA8);
			b.pieceValuesData[kA8] = 0;
			b.occupiedSquaresData.set(kD8);
			b.pieceBoardsData[black][0].set(kD8);
			b.pieceBoardsData[black][rook].set(kD8);
			b.pieceValuesData[kD8] = -rook;
			keyChange ^= zobristKey(-rook, kA8);
			keyChange ^= zobristKey(-rook, kD8);
			b.hasQsideCastledData[black] = 1;
		}
	}
	b.positionKeyData ^= keyChange;
}

void Board::handleCastleRightsChanges(Move move) {
	auto &b = *this;
	if (b.flagsData.sideToMoveValue == white) {
		if (move.from() == kA1) {
			b.positionKeyData ^= zobristQueenSideCastleKey(white) *
								 b.flagsData.whiteQueenSideValue;
			b.flagsData.whiteQueenSideValue = 0;
		} else if (move.from() == kE1) {
			b.positionKeyData ^= zobristQueenSideCastleKey(white) *
								 b.flagsData.whiteQueenSideValue;
			b.positionKeyData ^= zobristKingSideCastleKey(white) *
								 b.flagsData.whiteKingSideValue;
			b.flagsData.whiteQueenSideValue = 0;
			b.flagsData.whiteKingSideValue = 0;
		} else if (move.from() == kH1) {
			b.positionKeyData ^= zobristKingSideCastleKey(white) *
								 b.flagsData.whiteKingSideValue;
			b.flagsData.whiteKingSideValue = 0;
		}
		if (move.to() == kA8) {
			b.positionKeyData ^= zobristQueenSideCastleKey(black) *
								 b.flagsData.blackQueenSideValue;
			b.flagsData.blackQueenSideValue = 0;
		} else if (move.to() == kE8) {
			b.positionKeyData ^= zobristQueenSideCastleKey(black) *
								 b.flagsData.blackQueenSideValue;
			b.positionKeyData ^= zobristKingSideCastleKey(black) *
								 b.flagsData.blackKingSideValue;
			b.flagsData.blackQueenSideValue = 0;
			b.flagsData.blackKingSideValue = 0;
		} else if (move.to() == kH8) {
			b.positionKeyData ^= zobristKingSideCastleKey(black) *
								 b.flagsData.blackKingSideValue;
			b.flagsData.blackKingSideValue = 0;
		}
	} else {
		if (move.from() == kA8) {
			b.positionKeyData ^= zobristQueenSideCastleKey(black) *
								 b.flagsData.blackQueenSideValue;
			b.flagsData.blackQueenSideValue = 0;
		} else if (move.from() == kE8) {
			b.positionKeyData ^= zobristQueenSideCastleKey(black) *
								 b.flagsData.blackQueenSideValue;
			b.positionKeyData ^= zobristKingSideCastleKey(black) *
								 b.flagsData.blackKingSideValue;
			b.flagsData.blackQueenSideValue = 0;
			b.flagsData.blackKingSideValue = 0;
		} else if (move.from() == kH8) {
			b.positionKeyData ^= zobristKingSideCastleKey(black) *
								 b.flagsData.blackKingSideValue;
			b.flagsData.blackKingSideValue = 0;
		}
		if (move.to() == kA1) {
			b.positionKeyData ^= zobristQueenSideCastleKey(white) *
								 b.flagsData.whiteQueenSideValue;
			b.flagsData.whiteQueenSideValue = 0;
		} else if (move.to() == kE1) {
			b.positionKeyData ^= zobristKingSideCastleKey(white) *
								 b.flagsData.whiteKingSideValue;
			b.positionKeyData ^= zobristQueenSideCastleKey(white) *
								 b.flagsData.whiteQueenSideValue;
			b.flagsData.whiteQueenSideValue = 0;
			b.flagsData.whiteKingSideValue = 0;
		} else if (move.to() == kH1) {
			b.positionKeyData ^= zobristKingSideCastleKey(white) *
								 b.flagsData.whiteKingSideValue;
			b.flagsData.whiteKingSideValue = 0;
		}
	}
}

void Board::handleCastleUndo(Move m) {
	auto &b = *this;
	if (b.flagsData.sideToMoveValue == white) {
		if (m.from() < m.to()) {
			b.occupiedSquaresData.set(kH1);
			b.pieceBoardsData[white][0].set(kH1);
			b.pieceBoardsData[white][rook].set(kH1);
			b.occupiedSquaresData.clear(kF1);
			b.pieceBoardsData[white][0].clear(kF1);
			b.pieceBoardsData[white][rook].clear(kF1);
			b.pieceValuesData[kF1] = empty;
			b.pieceValuesData[kH1] = wRook;
			b.hasKsideCastledData[white] = 0;
		} else {
			b.occupiedSquaresData.set(kA1);
			b.pieceBoardsData[white][0].set(kA1);
			b.pieceBoardsData[white][rook].set(kA1);
			b.occupiedSquaresData.clear(kD1);
			b.pieceBoardsData[white][0].clear(kD1);
			b.pieceBoardsData[white][rook].clear(kD1);
			b.pieceValuesData[kD1] = empty;
			b.pieceValuesData[kA1] = wRook;
			b.hasQsideCastledData[white] = 0;
		}
	} else {
		if (m.from() < m.to()) {
			b.occupiedSquaresData.set(kH8);
			b.pieceBoardsData[black][0].set(kH8);
			b.pieceBoardsData[black][rook].set(kH8);
			b.occupiedSquaresData.clear(kF8);
			b.pieceBoardsData[black][0].clear(kF8);
			b.pieceBoardsData[black][rook].clear(kF8);
			b.pieceValuesData[kF8] = empty;
			b.pieceValuesData[kH8] = bRook;
			b.hasKsideCastledData[black] = 0;
		} else {
			b.occupiedSquaresData.set(kA8);
			b.pieceBoardsData[black][0].set(kA8);
			b.pieceBoardsData[black][rook].set(kA8);
			b.occupiedSquaresData.clear(kD8);
			b.pieceBoardsData[black][0].clear(kD8);
			b.pieceBoardsData[black][rook].clear(kD8);
			b.pieceValuesData[kD8] = empty;
			b.pieceValuesData[kA8] = bRook;
			b.hasQsideCastledData[black] = 0;
		}
	}
}

void Board::handleEnPassant(Move move) {
	auto &b = *this;
	if (b.flagsData.sideToMoveValue == white) {
		b.occupiedSquaresData.clear(move.to() - 8);
		b.pieceBoardsData[black][0].clear(move.to() - 8);
		b.pieceBoardsData[black][pawn].clear(move.to() - 8);
		b.pieceValuesData[move.to() - 8] = empty;
		b.pieceCountsData[black][0]--;
		b.pieceCountsData[black][pawn]--;
		b.positionKeyData ^= zobristKey(bPawn, move.to() - 8);
		b.pawnKeyData ^= zobristKey(bPawn, move.to() - 8);
	} else {
		b.occupiedSquaresData.clear(move.to() + 8);
		b.pieceBoardsData[white][0].clear(move.to() + 8);
		b.pieceBoardsData[white][pawn].clear(move.to() + 8);
		b.pieceValuesData[move.to() + 8] = empty;
		b.pieceCountsData[white][0]--;
		b.pieceCountsData[white][pawn]--;
		b.positionKeyData ^= zobristKey(wPawn, move.to() + 8);
		b.pawnKeyData ^= zobristKey(wPawn, move.to() + 8);
	}
}

void Board::handleEnPassantUndo(Move move) {
	auto &b = *this;
	if (b.flagsData.sideToMoveValue == white) {
		b.occupiedSquaresData.set(move.to() - 8);
		b.pieceBoardsData[black][0].set(move.to() - 8);
		b.pieceBoardsData[black][pawn].set(move.to() - 8);
		b.pieceValuesData[move.to() - 8] = bPawn;
		b.pieceCountsData[black][0]++;
		b.pieceCountsData[black][pawn]++;
	} else {
		b.occupiedSquaresData.set(move.to() + 8);
		b.pieceBoardsData[white][0].set(move.to() + 8);
		b.pieceBoardsData[white][pawn].set(move.to() + 8);
		b.pieceValuesData[move.to() + 8] = wPawn;
		b.pieceCountsData[white][0]++;
		b.pieceCountsData[white][pawn]++;
	}
}

void Board::handlePawnPromotion(Move move) {
	auto &b = *this;
	const int color = b.flagsData.sideToMoveValue;
	if (color == white) {
		b.pieceValuesData[move.to()] = move.promo();
		b.positionKeyData ^= zobristKey(pawn, move.to());
		b.pawnKeyData ^= zobristKey(pawn, move.to());
		b.positionKeyData ^= zobristKey(move.promo(), move.to());
	} else {
		b.pieceValuesData[move.to()] = -move.promo();
		b.positionKeyData ^= zobristKey(-pawn, move.to());
		b.pawnKeyData ^= zobristKey(-pawn, move.to());
		b.positionKeyData ^= zobristKey(-move.promo(), move.to());
	}
	b.pieceBoardsData[color][move.promo()].set(move.to());
	b.pieceBoardsData[color][pawn].clear(move.to());
	b.pieceCountsData[color][pawn]--;
	b.pieceCountsData[color][move.promo()]++;
}

void Board::handlePromotionUndo(Move move) {
	auto &b = *this;
	if (b.flagsData.sideToMoveValue == white) {
		b.pieceBoardsData[white][move.promo()].clear(move.from());
		b.pieceBoardsData[white][pawn].set(move.from());
		b.pieceCountsData[white][move.promo()]--;
		b.pieceCountsData[white][pawn]++;
		b.pieceValuesData[move.from()] = wPawn;
	} else {
		b.pieceBoardsData[black][move.promo()].clear(move.from());
		b.pieceBoardsData[black][pawn].set(move.from());
		b.pieceCountsData[black][move.promo()]--;
		b.pieceCountsData[black][pawn]++;
		b.pieceValuesData[move.from()] = bPawn;
	}
}

void Board::storeUndoInfo(const UndoInfo &undo) {
	auto &b = *this;
	b.undoInfoData[b.flagsData.halfMoveCountValue] = undo;
}

bool Board::lastMoveWasCapture() {
	auto &board = *this;
	if (board.flagsData.mostRecentCaptureValue == 0) {
		return false;
	} else {
		return true;
	}
}

bool Board::moveIsCapture(Move move) {
	auto &board = *this;
	return (board.pieceValuesData[move.to()] != 0);
}

int Board::repetitions() {
	auto &board = *this;
	return board.flagsData.repeatCountValue;
}
bool Board::genLvaAttackToSquare(Move &move, int square) {
	auto &board = *this;
	bool rc;
	int stm;
	Bitboard attackers;
	rc = false;
	move.setPromo(empty);
	move.setTo(square);
	stm = board.flagsData.sideToMoveValue;
	if (rc == false) {
		if (board.flagsData.sideToMoveValue == white) {
			attackers =
				blackPawnAttacks(square) & board.pieceBoardsData[white][pawn];
		} else {
			attackers =
				whitePawnAttacks(square) & board.pieceBoardsData[black][pawn];
		}
		if (attackers) {
			rc = true;
			move.setFrom(attackers.firstOne());
			if ((move.to() / 8 == 0) || (move.to() / 8 == 7)) {
				move.setPromo(queen);
			}
		}
	}
	if (rc == false) {
		attackers = knightAttacks(square) & board.pieceBoardsData[stm][knight];
		if (attackers) {
			rc = true;
			move.setFrom(attackers.firstOne());
		}
	}
	if (rc == false) {
		attackers = board.occupiedSquaresData.bishopAttacks(square) &
					board.pieceBoardsData[stm][bishop];
		if (attackers) {
			rc = true;
			move.setFrom(attackers.firstOne());
		}
	}
	if (rc == false) {
		attackers = board.occupiedSquaresData.rookAttacks(square) &
					board.pieceBoardsData[stm][rook];
		if (attackers) {
			rc = true;
			move.setFrom(attackers.firstOne());
		}
	}
	if (rc == false) {
		attackers = board.occupiedSquaresData.queenAttacks(square) &
					board.pieceBoardsData[stm][queen];
		if (attackers) {
			rc = true;
			move.setFrom(attackers.firstOne());
		}
	}
	if (rc == false) {
		attackers = kingAttacks(square) & board.pieceBoardsData[stm][king];
		if (attackers) {
			rc = true;
			move.setFrom(attackers.firstOne());
		}
	}
	if (rc) {
		assert(board.isPseudoLegalMove(move));
	}
	return rc;
}

} // namespace morphy_chess
