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

#include <morphychess/xboard.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <hpx/modules/threading.hpp>
#include <limits>
#include <mutex>
#include <poll.h>
#include <random>
#include <sstream>
#include <string_view>
#include <unistd.h>

namespace morphy_chess {
namespace {

std::string trim(std::string_view text) {
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string_view::npos) {
		return {};
	}
	const auto last = text.find_last_not_of(" \t\r\n");
	return std::string(text.substr(first, last - first + 1));
}

template <class Number> bool parseNumber(std::string_view text, Number &value) {
	const auto result =
		std::from_chars(text.data(), text.data() + text.size(), value);
	return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

std::string coordinateMove(Move move) {
	if (move.isEmpty()) {
		return {};
	}
	std::string text;
	text += static_cast<char>('a' + move.from() % 8);
	text += static_cast<char>('1' + move.from() / 8);
	text += static_cast<char>('a' + move.to() % 8);
	text += static_cast<char>('1' + move.to() / 8);
	switch (move.promo()) {
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

Result parseGameResult(std::string_view text) {
	if (text == "1-0") {
		return whiteWin;
	}
	if (text == "0-1") {
		return blackWin;
	}
	if (text == "1/2-1/2") {
		return draw;
	}
	if (text == "*") {
		return inProgress;
	}
	return invalid;
}

} // namespace

Xboard::Xboard(int workers, bool useBook, std::string bookPath,
			   std::ostream &output, int memoryEntries) :
	memoryTable(memoryEntries, kTableLocksPerThread * workers, workers),
	parallelSearch(currentBoard, evaluator, memoryTable, workers),
	output(output),
	useBook(useBook) {
	parallelSearch.setUseBook(useBook);
	if (useBook && !bookPath.empty()) {
		parallelSearch.openingBook().open(bookPath);
	}
	parallelSearch.setProgressHandler(
		[this](const SearchResult &result) { showProgress(result); });
}

Xboard::~Xboard() {
	try {
		stopSearch();
	} catch (...) {
	}
}

void Xboard::emit(const std::string &text) {
	const std::lock_guard<hpx::mutex> guard(outputMutex);
	output << text << std::flush;
}

SearchResult Xboard::stopSearch() {
	const SearchResult result = parallelSearch.stop();
	searchMode = SearchMode::none;
	return result;
}

void Xboard::positionChanged(bool keepDrawOffer) {
	ponderCompleted = false;
	analysisCompleted = false;
	if (!keepDrawOffer) {
		drawOffered = false;
	}
	gameResult = inProgress;
}

void Xboard::invalidArgument(const std::string &command) {
	emit("Error (invalid argument): " + command + "\n");
}

void Xboard::reportResult() {
	gameResult = currentBoard.result();
	switch (gameResult) {
	case whiteWin:
		emit("1-0 {White mates}\n");
		break;
	case blackWin:
		emit("0-1 {Black mates}\n");
		break;
	case draw:
		emit("1/2-1/2 {Draw}\n");
		break;
	default:
		break;
	}
}

void Xboard::applySearchResult(const SearchResult &result) {
	const Move move = result.move();
	if (move.isEmpty()) {
		reportResult();
		return;
	}
	if (!currentBoard.isLegalMove(move)) {
		emit("Error (search returned an illegal move)\n");
		forceMode = true;
		return;
	}
	if (result.score() < -kResignLimit) {
		Board position = currentBoard;
		Search confirmation(position, evaluator, memoryTable);
		const int whiteScore = confirmation.quiescentValue(0);
		const int rootScore =
			currentBoard.sideToMove() == white ? whiteScore : -whiteScore;
		if (rootScore < -kResignLimit) {
			emit("resign\n");
			forceMode = true;
			return;
		}
	}
	// The result is joined before applying a move. Only the protocol task
	// mutates the live game position between searches.
	if (!currentBoard.makeMove(move)) {
		return;
	}
	emit("move " + coordinateMove(move) + "\n");
	ponderCompleted = false;
	analysisCompleted = false;
	const int fullMoves = currentBoard.halfMoveCount() / 2;
	if (result.score() <= contemptFactor &&
		currentBoard.halfMoveCount() >= kMinHalfmoves4draw &&
		!(opponentSeconds < kDrawTimeLimit &&
		  opponentSeconds < remainingSeconds)) {
		if (drawOffered) {
			emit("offer draw\n");
		} else if (fullMoves - lastDrawOffer >= kDrawOfferFreq) {
			emit("offer draw\n");
			lastDrawOffer = fullMoves;
		}
	}
	drawOffered = false;
	reportResult();
}

double Xboard::searchSeconds() const {
	if (fixedMoveSeconds > 0.0) {
		return fixedMoveSeconds;
	}
	const int playedMoves = currentBoard.halfMoveCount() / 2;
	const int remainingMoves =
		movesPerControl > 0 ? movesPerControl - playedMoves % movesPerControl
							: 30;
	const double budget =
		remainingSeconds / remainingMoves + incrementSeconds * 0.9;
	return std::max(0.01,
					std::min(budget, std::max(0.01, remainingSeconds * 0.8)));
}

void Xboard::showProgress(const SearchResult &result) {
	if (!posting.load()) {
		return;
	}
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							 std::chrono::steady_clock::now() - searchStarted)
							 .count() /
						 10;
	std::ostringstream line;
	line << result.depth() << ' ' << result.score() << ' ' << elapsed << ' '
		 << result.nodes() << ' ' << coordinateMove(result.move()) << '\n';
	emit(line.str());
}

void Xboard::pollSearch() {
	if (quitRequested) {
		return;
	}
	if (searchMode != SearchMode::none) {
		if (!parallelSearch.ready()) {
			return;
		}
		const auto completedMode = searchMode;
		const SearchResult result = parallelSearch.finish();
		searchMode = SearchMode::none;
		if (completedMode == SearchMode::move) {
			applySearchResult(result);
		} else if (completedMode == SearchMode::ponder) {
			ponderCompleted = true;
		} else if (completedMode == SearchMode::analysis) {
			analysisCompleted = true;
		}
	}
	if (paused || forceMode || illegalPosition || gameResult != inProgress) {
		return;
	}
	SearchMode nextMode = SearchMode::none;
	double seconds = 0.0;
	if (analyzeMode) {
		if (!analysisCompleted) {
			nextMode = SearchMode::analysis;
		}
	} else if (currentBoard.sideToMove() == computerColor) {
		nextMode = SearchMode::move;
		seconds = searchSeconds();
	} else if (ponderEnabled && !ponderCompleted) {
		nextMode = SearchMode::ponder;
	}
	if (nextMode == SearchMode::none) {
		return;
	}
	if (currentBoard.result() != inProgress) {
		reportResult();
		return;
	}
	parallelSearch.setComputerColor(computerColor);
	parallelSearch.setUseBook(useBook && nextMode == SearchMode::move);
	searchStarted = std::chrono::steady_clock::now();
	parallelSearch.start(depthLimit, seconds);
	searchMode = nextMode;
}

void Xboard::showBookMoves() {
	stopSearch();
	struct Choice {
		Move move;
		long long games;
	};
	std::array<Move, maxMoves> moves;
	std::vector<Choice> choices;
	const int count = currentBoard.genPseudolegalMoves(moves.data());
	for (int index = 0; index < count; ++index) {
		if (!currentBoard.makeMove(moves[index])) {
			continue;
		}
		const auto *entry =
			parallelSearch.openingBook().findEntry(currentBoard);
		currentBoard.undoMove();
		if (entry) {
			choices.push_back(
				{moves[index],
				 0LL + entry->wins() + entry->losses() + entry->draws()});
		}
	}
	if (choices.empty()) {
		emit("\tNo book moves found\n\tLast book entry was ECO " +
			 parallelSearch.openingBook().lastEco() + "\n");
		return;
	}
	std::sort(choices.begin(), choices.end(),
			  [](const Choice &left, const Choice &right) {
				  return left.games > right.games;
			  });
	double totalGames = 0.0;
	for (const auto &choice : choices) {
		totalGames += choice.games;
	}
	std::ostringstream text;
	text << "\tBook moves\n";
	for (size_t index = 0; index < choices.size(); ++index) {
		const double chance = totalGames > 0.0
								  ? choices[index].games / totalGames
								  : 1.0 / choices.size();
		text << "\t" << index + 1 << ". " << coordinateMove(choices[index].move)
			 << ' ' << static_cast<int>(100 * chance + 0.5) << "%\n";
	}
	emit(text.str());
}

void Xboard::sendIcsSeeks() {
	std::mt19937 random(std::random_device{}());
	std::uniform_int_distribution<int> control(1, 18);
	for (int index = 0; index < 3; ++index) {
		emit("tellics seek " + std::to_string(control(random)) + " " +
			 std::to_string(control(random)) + " f\n");
	}
}

bool Xboard::processCommand(const std::string &command) {
	if (quitRequested) {
		return false;
	}
	const std::string line = trim(command);
	if (line.empty()) {
		return true;
	}
	const auto split = line.find_first_of(" \t");
	const std::string name = line.substr(0, split);
	const std::string arguments =
		split == std::string::npos
			? ""
			: trim(std::string_view(line).substr(split + 1));
	if (name == "quit") {
		stopSearch();
		quitRequested = true;
		return false;
	}
	if (name == "xboard" || name == "accepted" || name == "rejected" ||
		name == "computer" || name == "random" || name == "hint") {
		return true;
	}
	if (name == "ics") {
		icsActive = !arguments.empty() && arguments != "-";
	} else if (name == "protover") {
		int version;
		if (!parseNumber(arguments, version)) {
			invalidArgument(line);
		} else if (version >= 2) {
			emit("feature analyze=1 colors=0 draw=1 ics=1 name=1 pause=1 "
				 "ping=1 playother=1 reuse=1 san=0 setboard=1 sigint=0 "
				 "sigterm=1 time=1 usermove=1 variants=\"normal\" myname=\"" +
				 std::string(kName) + "\" done=1\n");
		}
	} else if (name == "ping") {
		emit("pong " + arguments + "\n");
	} else if (name == "new") {
		stopSearch();
		currentBoard.reset();
		parallelSearch.resetMemory();
		computerColor = black;
		forceMode = analyzeMode = illegalPosition = paused = false;
		depthLimit = kMaxSearchDepth;
		fixedMoveSeconds = 0.0;
		lastDrawOffer = -kDrawOfferFreq;
		positionChanged();
	} else if (name == "force") {
		stopSearch();
		forceMode = true;
		analyzeMode = false;
	} else if (name == "go" || name == "playother") {
		stopSearch();
		computerColor = name == "go" ? currentBoard.sideToMove()
									 : colorFlip(currentBoard.sideToMove());
		forceMode = analyzeMode = paused = false;
		positionChanged();
	} else if (name == "analyze") {
		stopSearch();
		analyzeMode = true;
		forceMode = paused = false;
		analysisCompleted = false;
	} else if (name == "exit") {
		stopSearch();
		analyzeMode = false;
		forceMode = true;
	} else if (name == "easy") {
		ponderEnabled = false;
		if (searchMode == SearchMode::ponder) {
			stopSearch();
		}
	} else if (name == "hard") {
		ponderEnabled = true;
		ponderCompleted = false;
	} else if (name == "post" || name == "nopost") {
		posting.store(name == "post");
	} else if (name == "pause") {
		stopSearch();
		paused = true;
	} else if (name == "resume") {
		paused = false;
		analysisCompleted = false;
		ponderCompleted = false;
	} else if (name == "draw") {
		drawOffered = true;
	} else if (name == "name") {
		lastOpponentName = opponentName;
		opponentName = arguments;
	} else if (name == "bk" || (name == "book" && arguments.empty())) {
		showBookMoves();
	} else if (name == "book") {
		if (arguments == "on" || arguments == "off") {
			stopSearch();
			useBook = arguments == "on";
		} else {
			invalidArgument(line);
		}
	} else if (name == "setboard") {
		stopSearch();
		if (currentBoard.loadFen(arguments.c_str())) {
			illegalPosition = false;
			positionChanged();
		} else {
			illegalPosition = true;
			emit("tellusererror Illegal position\n");
		}
	} else if (name == "undo" || name == "remove") {
		stopSearch();
		currentBoard.undoMove();
		if (name == "remove") {
			currentBoard.undoMove();
		}
		positionChanged();
	} else if (name == "sd") {
		int depth;
		if (!parseNumber(arguments, depth) || depth < 1 ||
			depth > kMaxSearchDepth) {
			invalidArgument(line);
		} else {
			depthLimit = depth;
		}
	} else if (name == "st") {
		double seconds;
		if (!parseNumber(arguments, seconds) || !std::isfinite(seconds) ||
			seconds <= 0.0) {
			invalidArgument(line);
		} else {
			fixedMoveSeconds = seconds;
		}
	} else if (name == "time" || name == "otim") {
		double centiseconds;
		if (!parseNumber(arguments, centiseconds) ||
			!std::isfinite(centiseconds) || centiseconds < 0.0) {
			invalidArgument(line);
		} else if (name == "time") {
			remainingSeconds = centiseconds / 100.0;
		} else {
			opponentSeconds = centiseconds / 100.0;
		}
	} else if (name == "level") {
		std::istringstream values(arguments);
		int moves;
		std::string base, extra;
		double increment;
		double minutes = 0.0, seconds = 0.0;
		bool valid = bool(values >> moves >> base >> increment) &&
					 !(values >> extra) && moves >= 0 &&
					 std::isfinite(increment) && increment >= 0.0;
		const auto colon = base.find(':');
		if (valid && colon == std::string::npos) {
			valid = parseNumber(base, minutes);
		} else if (valid) {
			valid =
				parseNumber(std::string_view(base).substr(0, colon), minutes) &&
				parseNumber(std::string_view(base).substr(colon + 1),
							seconds) &&
				seconds >= 0.0 && seconds < 60.0;
		}
		valid = valid && std::isfinite(minutes) && std::isfinite(seconds) &&
				minutes >= 0.0 && std::isfinite(60 * minutes + seconds) &&
				(60 * minutes + seconds) > 0.0;
		if (!valid) {
			invalidArgument(line);
		} else {
			movesPerControl = moves;
			remainingSeconds = 60 * minutes + seconds;
			incrementSeconds = increment;
			fixedMoveSeconds = 0.0;
		}
	} else if (name == "rating") {
		std::istringstream values(arguments);
		int ownRating, otherRating;
		std::string extra;
		if (!(values >> ownRating >> otherRating) || (values >> extra) ||
			ownRating < 0 || otherRating < 0) {
			invalidArgument(line);
		} else {
			stopSearch();
			engineRating = ownRating;
			opponentRating = otherRating;
			const long long factor =
				ownRating == 0 || otherRating == 0
					? 0
					: kContemptLevel *
						  (static_cast<long long>(otherRating) - ownRating) /
						  100;
			contemptFactor = static_cast<int>(std::clamp<long long>(
				factor, -kMinCheckmateValue / 2, kMinCheckmateValue / 2));
			parallelSearch.setContemptFactor(contemptFactor);
		}
	} else if (name == "result") {
		std::istringstream values(arguments);
		std::string resultText;
		values >> resultText;
		const Result result = parseGameResult(resultText);
		if (result == invalid) {
			invalidArgument(line);
		} else {
			stopSearch();
			gameResult = result;
			drawOffered = false;
			forceMode = true;
			analyzeMode = false;
			if (result != inProgress && engineRating > 0 &&
				opponentRating > 0) {
				parallelSearch.openingBook().learning(
					currentBoard, computerColor, result, opponentRating);
			}
			if (result != inProgress && icsActive) {
				if (opponentName != lastOpponentName) {
					emit("tellics rematch\n");
				}
				sendIcsSeeks();
			}
		}
	} else if (name == "?") {
		const auto oldMode = searchMode;
		const auto result = stopSearch();
		if (oldMode == SearchMode::move) {
			applySearchResult(result);
		}
	} else if (name == ".") {
		emit("stat01: 0 0 0 0 0\n");
	} else if (name == "variant") {
		if (arguments != "normal") {
			emit("Error (unsupported variant): " + arguments + "\n");
		}
	} else {
		const std::string moveText = name == "usermove" ? arguments : line;
		const Move move = Move::fromString(moveText.c_str());
		bool accepted = false;
		if (!move.isEmpty()) {
			stopSearch();
			accepted = !illegalPosition && currentBoard.isLegalMove(move) &&
					   currentBoard.makeMove(move);
		}
		if (accepted) {
			positionChanged(true);
			reportResult();
		} else if (name == "usermove" ||
				   (name.size() >= 2 && name[0] >= 'a' && name[0] <= 'h' &&
					name[1] >= '1' && name[1] <= '8')) {
			emit("Illegal move: " + moveText + "\n");
		} else {
			emit("Error (unknown command): " + name + "\n");
		}
	}
	return !quitRequested;
}

int Xboard::run(int inputFd) {
	std::string pending;
	while (!quitRequested) {
		pollSearch();
		pollfd descriptor{inputFd, POLLIN, 0};
		const int ready = ::poll(&descriptor, 1, 0);
		if (ready < 0 && errno != EINTR) {
			stopSearch();
			emit("Error (cannot poll input)\n");
			return 1;
		}
		if (ready > 0 && (descriptor.revents & (POLLIN | POLLHUP))) {
			std::array<char, 4096> buffer;
			const auto count = ::read(inputFd, buffer.data(), buffer.size());
			if (count == 0) {
				if (!pending.empty()) {
					processCommand(pending);
				}
				stopSearch();
				quitRequested = true;
				break;
			}
			if (count < 0) {
				if (errno != EINTR && errno != EAGAIN) {
					stopSearch();
					emit("Error (cannot read input)\n");
					return 1;
				}
			} else {
				pending.append(buffer.data(), static_cast<size_t>(count));
				size_t newline;
				while ((newline = pending.find('\n')) != std::string::npos) {
					const std::string command = pending.substr(0, newline);
					pending.erase(0, newline + 1);
					if (!processCommand(command)) {
						break;
					}
				}
				if (pending.size() > 65536) {
					stopSearch();
					emit("Error (input line too long)\n");
					return 1;
				}
			}
		} else if (ready > 0 && (descriptor.revents & (POLLERR | POLLNVAL))) {
			stopSearch();
			emit("Error (invalid input descriptor)\n");
			return 1;
		}
		if (!quitRequested) {
			hpx::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}
	return 0;
}

} // namespace morphy_chess
