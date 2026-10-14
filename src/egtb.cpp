#include <array>
#include <string>

#include <morphychess/bits.hpp>
#include <morphychess/egtb.hpp>

namespace morphy_chess {
// Decode a mixed-radix placement index: 64 choices for the stronger king,
// then 63 and 62 remaining squares. Illegal arrangements remain holes in
// the index space. This is indexing support, not a retrograde tablebase solver.
bool Board::loadEgtbIndex(int thirdMan, int index, int sideToMove) {
	if (thirdMan < pawn || thirdMan > queen || index < 0 ||
		index >= threeManPositionCount ||
		(sideToMove != white && sideToMove != black)) {
		return false;
	}
	const int thirdIndex = index % 62;
	index /= 62;
	const int blackIndex = index % 63;
	const int whiteKing = index / 63;
	const int blackKing = blackIndex + (blackIndex >= whiteKing);
	int thirdSquare = -1;
	int remaining = thirdIndex;
	for (int square = 0; square < 64; ++square) {
		if (square == whiteKing || square == blackKing) {
			continue;
		}
		if (remaining-- == 0) {
			thirdSquare = square;
			break;
		}
	}
	std::array<char, 64> squares{};
	squares[whiteKing] = 'K';
	squares[blackKing] = 'k';
	squares[thirdSquare] = " PNBRQ"[thirdMan];
	std::string fen;
	for (int rank = 7; rank >= 0; --rank) {
		int emptyCount = 0;
		for (int file = 0; file < 8; ++file) {
			const char piece = squares[8 * rank + file];
			if (piece == 0) {
				++emptyCount;
				continue;
			}
			if (emptyCount) {
				fen += static_cast<char>('0' + emptyCount);
				emptyCount = 0;
			}
			fen += piece;
		}
		if (emptyCount) {
			fen += static_cast<char>('0' + emptyCount);
		}
		if (rank) {
			fen += '/';
		}
	}
	fen += sideToMove == white ? " w - - 0 1" : " b - - 0 1";
	Board candidate;
	if (!candidate.loadFen(fen.c_str()) ||
		candidate.kingInCheck(colorFlip(sideToMove))) {
		return false;
	}
	*this = candidate;
	return true;
}
int Board::threeManEgtbIndex(int thirdMan, int pieceColor) const {
	if (thirdMan < pawn || thirdMan > queen ||
		(pieceColor != white && pieceColor != black) ||
		pieceCount(pieceColor, thirdMan) != 1 ||
		pieceCount(pieceColor, 0) != 2 ||
		pieceCount(colorFlip(pieceColor), 0) != 1) {
		return -1;
	}
	int whiteKing = pieceBoard(pieceColor, king).firstOne();
	int blackKing = pieceBoard(colorFlip(pieceColor), king).firstOne();
	int thirdSquare = pieceBoard(pieceColor, thirdMan).firstOne();
	// Canonicalize the side with the extra piece as white. Reflect ranks
	// when swapping colors so pawn movement keeps the same direction.
	if (pieceColor == black) {
		whiteKing ^= 56;
		blackKing ^= 56;
		thirdSquare ^= 56;
	}
	const int blackIndex = blackKing - (blackKing > whiteKing);
	// Remove the occupied king squares from the third piece's ordinal.
	const int thirdIndex =
		thirdSquare - (thirdSquare > whiteKing) - (thirdSquare > blackKing);
	return (whiteKing * 63 + blackIndex) * 62 + thirdIndex;
}
} // namespace morphy_chess
