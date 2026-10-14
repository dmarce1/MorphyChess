#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <morphychess/book.hpp>
#include <string>
#include <vector>

namespace morphy_chess {
namespace {

class TemporaryBookFiles {
public:
	TemporaryBookFiles() {
		static std::atomic<unsigned long> counter{0};
		const auto now =
			std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() /
					("morphychess_book_" + std::to_string(now) + "_" +
					 std::to_string(counter++));
		std::filesystem::create_directory(directory);
	}
	~TemporaryBookFiles() {
		std::error_code error;
		std::filesystem::remove_all(directory, error);
	}
	std::string path(const char *name) const {
		return (directory / name).string();
	}
	void writePgn(const std::string &text) const {
		std::ofstream output(path("eco.pgn"));
		output << text;
	}

private:
	std::filesystem::path directory;
};

TEST(Book, EmptyBookReturnsNoMoveAndLearningDoesNotAccessMissingEntry) {
	Book book;
	Board board;
	EXPECT_EQ(book.entryCount(), 0);
	EXPECT_EQ(book.findEntry(board), nullptr);
	EXPECT_TRUE(book.findMove(board).isEmpty());
	ASSERT_TRUE(board.makeMove(Move(12, 28)));
	ASSERT_TRUE(board.makeMove(Move(52, 36)));
	const auto key = board.positionKey();
	book.learning(board, white, whiteWin, 2000);
	EXPECT_EQ(book.entryCount(), 0);
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_EQ(board.halfMoveCount(), 2);
}

TEST(Book, FindsSingleEntryAndRestoresBoardAfterChoosingMove) {
	TemporaryBookFiles files;
	files.writePgn("[ECO \"B00\"]\n1.e4 *\n");
	Book book;
	ASSERT_EQ(book.digest(files.path("eco.pgn")), 1);
	ASSERT_EQ(book.entryCount(), 1);
	Board board;
	const auto key = board.positionKey();
	EXPECT_EQ(book.findMove(board), Move(12, 28));
	EXPECT_EQ(book.lastEco(), "B00");
	EXPECT_EQ(board.positionKey(), key);
	EXPECT_EQ(board.halfMoveCount(), 0);
	ASSERT_TRUE(board.makeMove(Move(12, 28)));
	const auto *entry = book.findEntry(board);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->key(), board.positionKey());
	EXPECT_EQ(entry->color(), black);
	EXPECT_EQ(entry->wins(), 1);
	EXPECT_EQ(entry->losses(), 0);
	EXPECT_EQ(entry->draws(), 0);
	EXPECT_STREQ(entry->eco(), "B00");
}

TEST(Book, FindsBothEndpointsOfTwoEntryBook) {
	TemporaryBookFiles files;
	files.writePgn("[ECO \"B00\"]\n1.e4 *\n[ECO \"A40\"]\n1.d4 *\n");
	Book book;
	ASSERT_EQ(book.digest(files.path("eco.pgn")), 2);
	ASSERT_EQ(book.entryCount(), 2);
	Board first;
	Board second;
	ASSERT_TRUE(first.makeMove(Move(12, 28)));
	ASSERT_TRUE(second.makeMove(Move(11, 27)));
	ASSERT_NE(book.findEntry(first), nullptr);
	ASSERT_NE(book.findEntry(second), nullptr);
	EXPECT_STREQ(book.findEntry(first)->eco(), "B00");
	EXPECT_STREQ(book.findEntry(second)->eco(), "A40");
}

TEST(Book, PreservesLegacyBinaryLayoutAndRoundTrips) {
	TemporaryBookFiles files;
	files.writePgn("[ECO \"B00\"]\n1.e4 *\n");
	Book book;
	ASSERT_EQ(book.digest(files.path("eco.pgn")), 1);
	ASSERT_TRUE(book.save(files.path("book.bin")));
	EXPECT_EQ(std::filesystem::file_size(files.path("book.bin")), 131080);
	std::ifstream input(files.path("book.bin"), std::ios::binary);
	const std::vector<unsigned char> bytes(
		std::istreambuf_iterator<char>(input), {});
	EXPECT_EQ(bytes[8], 1);
	EXPECT_EQ(bytes[20], black);
	EXPECT_EQ(bytes[24], 'B');
	EXPECT_EQ(bytes[25], '0');
	EXPECT_EQ(bytes[26], '0');
	EXPECT_EQ(bytes[27], 0);
	EXPECT_EQ(bytes[28], 0xd0);
	EXPECT_EQ(bytes[29], 0x07);
	EXPECT_EQ(bytes[4096 * 32], 1);
	Book reloaded;
	ASSERT_TRUE(reloaded.open(files.path("book.bin")));
	EXPECT_EQ(reloaded.entryCount(), 1);
	Board board;
	EXPECT_EQ(reloaded.findMove(board), Move(12, 28));
	ASSERT_TRUE(reloaded.close());
}

TEST(Book, ZeroWeightsStillChooseLegalBookMove) {
	TemporaryBookFiles files;
	files.writePgn("1.e4 *\n");
	Book original;
	ASSERT_TRUE(original.open(files.path("book.bin")));
	{
		std::fstream file(files.path("book.bin"),
						  std::ios::binary | std::ios::in | std::ios::out);
		file.seekp(8);
		const char zero[4]{};
		file.write(zero, sizeof(zero));
	}
	Book book;
	ASSERT_TRUE(book.open(files.path("book.bin")));
	Board board;
	EXPECT_EQ(book.findMove(board), Move(12, 28));
	EXPECT_EQ(board.halfMoveCount(), 0);
}

TEST(Book, CreatesMissingBookFromAdjacentPgn) {
	TemporaryBookFiles files;
	files.writePgn("[ECO \"A40\"]\n1.d4 *\n");
	Book book;
	ASSERT_TRUE(book.open(files.path("book.bin")));
	EXPECT_TRUE(std::filesystem::exists(files.path("book.bin")));
	EXPECT_EQ(book.entryCount(), 1);
	Board board;
	EXPECT_EQ(book.findMove(board), Move(11, 27));
}

TEST(Book, LearningUpdatesMatchingPositionsAndPersists) {
	TemporaryBookFiles files;
	files.writePgn("[ECO \"B00\"]\n1.e4 *\n");
	Book book;
	ASSERT_TRUE(book.open(files.path("book.bin")));
	Board board;
	ASSERT_TRUE(board.makeMove(Move(12, 28)));
	const Board opening = board;
	ASSERT_TRUE(board.makeMove(Move(52, 36)));
	book.learning(board, white, whiteWin, 2000);
	ASSERT_NE(book.findEntry(opening), nullptr);
	EXPECT_EQ(book.findEntry(opening)->wins(), 2);
	EXPECT_EQ(book.findEntry(opening)->rating(), 2016);
	Book reloaded;
	ASSERT_TRUE(reloaded.open(files.path("book.bin")));
	EXPECT_EQ(reloaded.findEntry(opening)->wins(), 2);
}

TEST(Book, FailedMissingBookFallbackPreservesPreviousFile) {
	TemporaryBookFiles files;
	files.writePgn("1.e4 *\n");
	{
		Book book;
		ASSERT_TRUE(book.open(files.path("original.bin")));
		ASSERT_EQ(book.entryCount(), 1);
		files.writePgn("1.d4 d5 *\n");
		std::filesystem::create_symlink(
			files.path("missing_directory/book.bin"),
			files.path("unwritable.bin"));
		EXPECT_FALSE(book.open(files.path("unwritable.bin")));
		EXPECT_EQ(book.entryCount(), 1);
		Board board;
		EXPECT_EQ(book.findMove(board), Move(12, 28));
	}
	Book original;
	ASSERT_TRUE(original.open(files.path("original.bin")));
	EXPECT_EQ(original.entryCount(), 1);
	Board board;
	EXPECT_EQ(original.findMove(board), Move(12, 28));
}

TEST(Book, RejectsTruncatedAndInvalidFilesWithoutLosingLoadedBook) {
	TemporaryBookFiles files;
	files.writePgn("1.e4 *\n");
	Book book;
	ASSERT_TRUE(book.open(files.path("book.bin")));
	{
		std::ofstream output(files.path("bad.bin"), std::ios::binary);
		output << "short";
	}
	EXPECT_FALSE(book.open(files.path("bad.bin")));
	EXPECT_EQ(book.entryCount(), 1);
	{
		std::vector<char> invalid(131080, 0);
		invalid[4096 * 32 + 1] = 0x20;
		std::ofstream output(files.path("bad.bin"), std::ios::binary);
		output.write(invalid.data(), invalid.size());
	}
	EXPECT_FALSE(book.open(files.path("bad.bin")));
	EXPECT_EQ(book.entryCount(), 1);
	files.writePgn("1.e4 Qa9 *\n");
	EXPECT_EQ(book.digest(files.path("eco.pgn")), -1);
	EXPECT_EQ(book.entryCount(), 1);
}

} // namespace
} // namespace morphy_chess
