#include <morphychess/xboard.hpp>

#include <chrono>
#include <gtest/gtest.h>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/threading.hpp>
#include <sstream>
#include <string>
#include <unistd.h>

namespace morphy_chess {
namespace {

class XboardTest : public ::testing::Test {
protected:
	std::ostringstream output;
	Xboard protocol{1, false, "", output, 4096};

	void clearOutput() {
		output.str("");
		output.clear();
	}
	bool
	finishSearch(std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (protocol.searching() &&
			   std::chrono::steady_clock::now() < deadline) {
			hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
			protocol.pollSearch();
		}
		return !protocol.searching();
	}
};

TEST_F(XboardTest, AnnouncesCapabilitiesAndTreatsPingAsLiteralText) {
	EXPECT_TRUE(protocol.processCommand("xboard"));
	EXPECT_TRUE(protocol.processCommand("protover 2"));
	EXPECT_NE(output.str().find("ping=1"), std::string::npos);
	EXPECT_NE(output.str().find("usermove=1"), std::string::npos);
	EXPECT_NE(output.str().find("done=1"), std::string::npos);
	clearOutput();
	EXPECT_TRUE(protocol.processCommand("ping %s %n 123"));
	EXPECT_EQ(output.str(), "pong %s %n 123\n");
	EXPECT_TRUE(protocol.processCommand("rejected unsupported"));
	EXPECT_FALSE(protocol.processCommand("quit"));
	EXPECT_TRUE(protocol.quitting());
}

TEST_F(XboardTest, AppliesAndUndoesMovesInForceMode) {
	protocol.processCommand("new");
	protocol.processCommand("force");
	EXPECT_TRUE(protocol.processCommand("e2e4"));
	EXPECT_TRUE(protocol.processCommand("usermove e7e5"));
	EXPECT_EQ(protocol.board().halfMoveCount(), 2);
	EXPECT_EQ(protocol.board().pieceValue(28), pawn);
	EXPECT_EQ(protocol.board().pieceValue(36), -pawn);
	protocol.processCommand("undo");
	EXPECT_EQ(protocol.board().halfMoveCount(), 1);
	protocol.processCommand("usermove e7e5");
	protocol.processCommand("remove");
	EXPECT_EQ(protocol.board().halfMoveCount(), 0);
	const auto key = protocol.board().positionKey();
	protocol.processCommand("e2e5");
	EXPECT_EQ(protocol.board().positionKey(), key);
	EXPECT_NE(output.str().find("Illegal move: e2e5"), std::string::npos);
}

TEST_F(XboardTest, RetainsOpponentDrawOfferUntilResponseOrGameReset) {
	protocol.processCommand("new");
	protocol.processCommand("force");
	protocol.processCommand("draw");
	ASSERT_TRUE(protocol.hasDrawOffer());
	protocol.processCommand("usermove e2e4");
	EXPECT_TRUE(protocol.hasDrawOffer());
	protocol.processCommand("undo");
	EXPECT_FALSE(protocol.hasDrawOffer());
	protocol.processCommand("draw");
	protocol.processCommand("result 1/2-1/2 {Draw}");
	EXPECT_FALSE(protocol.hasDrawOffer());
}

TEST_F(XboardTest, UnknownCommandsDoNotCancelActiveSearch) {
	protocol.processCommand("easy");
	protocol.processCommand("sd 8");
	protocol.processCommand("st 1");
	protocol.processCommand("go");
	protocol.pollSearch();
	ASSERT_TRUE(protocol.searching());
	protocol.processCommand("unknown_command");
	EXPECT_TRUE(protocol.searching());
	EXPECT_NE(output.str().find("Error (unknown command): unknown_command"),
			  std::string::npos);
	protocol.processCommand("usermove invalid");
	EXPECT_TRUE(protocol.searching());
	EXPECT_NE(output.str().find("Illegal move: invalid"), std::string::npos);
	protocol.processCommand("force");
	EXPECT_FALSE(protocol.searching());
}

TEST_F(XboardTest, LoadsPositionsAndRejectsBadFenWithoutReplacingBoard) {
	protocol.processCommand("setboard 8/8/8/8/8/8/4K3/7k w - - 0 1");
	EXPECT_EQ(protocol.board().pieceValue(12), king);
	const auto key = protocol.board().positionKey();
	protocol.processCommand("setboard 8/8");
	EXPECT_EQ(protocol.board().positionKey(), key);
	EXPECT_NE(output.str().find("Illegal position"), std::string::npos);
	protocol.processCommand("new");
	EXPECT_EQ(protocol.board().pieceValue(4), king);
}

TEST_F(XboardTest, HandlesTimeControlOptionsAndValidation) {
	protocol.processCommand("sd 1");
	protocol.processCommand("st 0.05");
	protocol.processCommand("level 40 5:30 2");
	protocol.processCommand("level 0 3 1");
	protocol.processCommand("time 500");
	protocol.processCommand("otim 400");
	EXPECT_TRUE(output.str().empty());
	for (const auto *command : {"sd 0", "st 0", "st nan", "time -1",
								"level -1 5 0", "level 40 5:61 0"}) {
		protocol.processCommand(command);
	}
	EXPECT_NE(output.str().find("Error (invalid argument): sd 0"),
			  std::string::npos);
	EXPECT_NE(output.str().find("Error (invalid argument): level 40 5:61 0"),
			  std::string::npos);
}

TEST_F(XboardTest, ProducesMoveWithOneHpxWorkerAndCanStopBeforeMutation) {
	protocol.processCommand("easy");
	protocol.processCommand("sd 1");
	protocol.processCommand("st 0.05");
	protocol.processCommand("go");
	protocol.pollSearch();
	ASSERT_TRUE(finishSearch());
	EXPECT_EQ(protocol.board().halfMoveCount(), 1);
	EXPECT_NE(output.str().find("move "), std::string::npos);
	protocol.processCommand("sd 8");
	protocol.processCommand("go");
	protocol.pollSearch();
	EXPECT_TRUE(protocol.searching());
	protocol.processCommand("force");
	EXPECT_FALSE(protocol.searching());
	protocol.processCommand("undo");
	EXPECT_EQ(protocol.board().halfMoveCount(), 0);
}

TEST_F(XboardTest, ResignationUsesEnginePerspectiveForEitherColor) {
	for (const auto *fen :
		 {"7k/8/8/8/8/8/8/3QKR2 b - - 0 1", "3qkr2/8/8/8/8/8/8/7K w - - 0 1"}) {
		protocol.processCommand(std::string("setboard ") + fen);
		protocol.processCommand("easy");
		protocol.processCommand("sd 1");
		protocol.processCommand("st 0.05");
		protocol.processCommand("go");
		protocol.pollSearch();
		ASSERT_TRUE(finishSearch());
		EXPECT_EQ(output.str(), "resign\n");
		EXPECT_EQ(protocol.board().halfMoveCount(), 0);
		clearOutput();
	}
}

TEST_F(XboardTest, PonderingAndAnalysisNeverApplyMovesOrRestartForever) {
	protocol.processCommand("new");
	protocol.processCommand("sd 1");
	protocol.processCommand("hard");
	protocol.pollSearch();
	ASSERT_TRUE(finishSearch());
	EXPECT_EQ(protocol.board().halfMoveCount(), 0);
	EXPECT_EQ(output.str().find("move "), std::string::npos);
	protocol.pollSearch();
	EXPECT_FALSE(protocol.searching());
	protocol.processCommand("analyze");
	protocol.processCommand("post");
	protocol.pollSearch();
	ASSERT_TRUE(finishSearch());
	EXPECT_EQ(protocol.board().halfMoveCount(), 0);
	EXPECT_FALSE(output.str().empty());
	protocol.pollSearch();
	EXPECT_FALSE(protocol.searching());
	protocol.processCommand("exit");
	protocol.pollSearch();
	EXPECT_FALSE(protocol.searching());
}

TEST_F(XboardTest, PausesResumesAndMovesImmediatelyOnRequest) {
	protocol.processCommand("easy");
	protocol.processCommand("sd 8");
	protocol.processCommand("go");
	protocol.processCommand("pause");
	protocol.pollSearch();
	EXPECT_FALSE(protocol.searching());
	protocol.processCommand("resume");
	protocol.pollSearch();
	EXPECT_TRUE(protocol.searching());
	hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
	protocol.processCommand("?");
	EXPECT_FALSE(protocol.searching());
	EXPECT_EQ(protocol.board().halfMoveCount(), 1);
	EXPECT_NE(output.str().find("move "), std::string::npos);
}

TEST_F(XboardTest, SendsRematchAndThreeSeeksOnlyForIcsSessions) {
	protocol.processCommand("name FirstOpponent");
	protocol.processCommand("result 1/2-1/2 {Draw}");
	EXPECT_TRUE(output.str().empty());
	protocol.processCommand("ics example.invalid");
	protocol.processCommand("result 1-0 {White wins}");
	std::istringstream lines(output.str());
	std::string line;
	ASSERT_TRUE(bool(std::getline(lines, line)));
	EXPECT_EQ(line, "tellics rematch");
	for (int index = 0; index < 3; ++index) {
		ASSERT_TRUE(bool(std::getline(lines, line)));
		std::istringstream seek(line);
		std::string prefix, command, mode;
		int minutes = 0, increment = 0;
		seek >> prefix >> command >> minutes >> increment >> mode;
		EXPECT_EQ(prefix, "tellics");
		EXPECT_EQ(command, "seek");
		EXPECT_GE(minutes, 1);
		EXPECT_LE(minutes, 18);
		EXPECT_GE(increment, 1);
		EXPECT_LE(increment, 18);
		EXPECT_EQ(mode, "f");
	}
	EXPECT_FALSE(bool(std::getline(lines, line)));
	clearOutput();
	protocol.processCommand("ics -");
	protocol.processCommand("result 0-1 {Black wins}");
	EXPECT_TRUE(output.str().empty());
}

TEST_F(XboardTest, InputLoopHandlesEofAndUnterminatedLastCommand) {
	int descriptors[2];
	ASSERT_EQ(::pipe(descriptors), 0);
	const std::string commands = "xboard\nprotover 2\nping 17";
	ASSERT_EQ(::write(descriptors[1], commands.data(), commands.size()),
			  static_cast<ssize_t>(commands.size()));
	::close(descriptors[1]);
	EXPECT_EQ(protocol.run(descriptors[0]), 0);
	::close(descriptors[0]);
	EXPECT_TRUE(protocol.quitting());
	EXPECT_NE(output.str().find("pong 17\n"), std::string::npos);
}

TEST_F(XboardTest, InputLoopYieldsSoOneWorkerCanDeliverCommands) {
	int descriptors[2];
	ASSERT_EQ(::pipe(descriptors), 0);
	auto writer = hpx::async([writeFd = descriptors[1]] {
		hpx::this_thread::sleep_for(std::chrono::milliseconds(5));
		const std::string commands = "new\neasy\nsd 1\nst 0.01\ngo\n";
		const auto count = ::write(writeFd, commands.data(), commands.size());
		hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
		const std::string end = "ping 29\nquit\n";
		const auto endCount = ::write(writeFd, end.data(), end.size());
		EXPECT_EQ(endCount, static_cast<ssize_t>(end.size()));
		::close(writeFd);
		return count;
	});
	EXPECT_EQ(protocol.run(descriptors[0]), 0);
	::close(descriptors[0]);
	EXPECT_GT(writer.get(), 0);
	EXPECT_NE(output.str().find("pong 29\n"), std::string::npos);
	EXPECT_NE(output.str().find("move "), std::string::npos);
}

} // namespace
} // namespace morphy_chess
