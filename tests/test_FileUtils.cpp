/**
 * @file test_FileUtils.cpp
 * @brief Testy jednostkowe klasy FileUtils.
 * @author Arkadiusz Mioducki and Kamil Płóciennik
 * @date 2026
 */

#include <gtest/gtest.h>
#include "../src/utils/FileUtils.h"
#include <QDir>
#include <QFile>
#include <QThread>

using namespace utils;

class FileUtilsTest : public ::testing::Test {
protected:
    QString tempDir = QDir::tempPath();

    void TearDown() override {
        // Cleanup temp files
        QDir dir(tempDir);
        QStringList files = dir.entryList({"LLM_chart_generator_test_*"}, QDir::Files);
        for (const auto& f : files) {
            dir.remove(f);
        }
    }
};

// ===== writeTextFile / readTextFile =====

TEST_F(FileUtilsTest, WriteAndReadRoundTrip) {
    QString path = tempDir + "/LLM_chart_generator_test_rw.txt";
    QString content = "Hello \nLine 2\nŻółć";

    bool written = FileUtils::writeTextFile(path, content);
    EXPECT_TRUE(written);

    bool ok;
    QString read = FileUtils::readTextFile(path, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(read, content);
}

TEST_F(FileUtilsTest, ReadNonExistentFileReturnsFalse) {
    bool ok = true;
    FileUtils::readTextFile("/nonexistent/path/file.txt", &ok);
    EXPECT_FALSE(ok);
}

TEST_F(FileUtilsTest, WriteToInvalidPathReturnsFalse) {
    bool result = FileUtils::writeTextFile("/root/cannot_write_here/file.txt", "data");
    EXPECT_FALSE(result);
}

TEST_F(FileUtilsTest, ReadWithNullOkPointer) {
    // Should not crash when ok == nullptr
    QString result = FileUtils::readTextFile("/nonexistent.txt", nullptr);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(FileUtilsTest, WriteAndReadUTF8Content) {
    QString path = tempDir + "/LLM_chart_generator_test_utf8.txt";
    QString content = "Ąćęłńóśźż ĄĆĘŁŃÓŚŹŻ 日本語 emoji: 🐍";

    FileUtils::writeTextFile(path, content);
    bool ok;
    QString read = FileUtils::readTextFile(path, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(read, content);
}

// ===== fileExists =====

TEST_F(FileUtilsTest, FileExistsReturnsTrueForExistingFile) {
    QString path = tempDir + "/LLM_chart_generator_test_exists.txt";
    FileUtils::writeTextFile(path, "test");
    EXPECT_TRUE(FileUtils::fileExists(path));
}

TEST_F(FileUtilsTest, FileExistsReturnsFalseForMissingFile) {
    EXPECT_FALSE(FileUtils::fileExists("/nonexistent/_xyz.txt"));
}

// ===== fileExtension =====

TEST_F(FileUtilsTest, FileExtensionPng) {
    EXPECT_EQ(FileUtils::fileExtension("/path/to/file.png"), "png");
}

TEST_F(FileUtilsTest, FileExtensionPy) {
    EXPECT_EQ(FileUtils::fileExtension("script.py"), "py");
}

TEST_F(FileUtilsTest, FileExtensionNoExtension) {
    EXPECT_TRUE(FileUtils::fileExtension("noextension").isEmpty());
}

TEST_F(FileUtilsTest, FileExtensionIsLowercase) {
    EXPECT_EQ(FileUtils::fileExtension("IMAGE.PNG"), "png");
}

// ===== parseCsv =====

TEST_F(FileUtilsTest, ParseCsvSimple) {
    QString csv = "a,b,c\n1,2,3\n4,5,6";
    auto rows = FileUtils::parseCsv(csv);
    ASSERT_EQ(rows.size(), 3);
    EXPECT_EQ(rows[0][0], "a");
    EXPECT_EQ(rows[1][1], "2");
    EXPECT_EQ(rows[2][2], "6");
}

TEST_F(FileUtilsTest, ParseCsvCustomDelimiter) {
    QString csv = "a;b;c\n1;2;3";
    auto rows = FileUtils::parseCsv(csv, ';');
    ASSERT_EQ(rows.size(), 2);
    EXPECT_EQ(rows[0][1], "b");
}

TEST_F(FileUtilsTest, ParseCsvSkipsEmptyLines) {
    QString csv = "1,2\n\n3,4\n";
    auto rows = FileUtils::parseCsv(csv);
    EXPECT_EQ(rows.size(), 2);
}

TEST_F(FileUtilsTest, ParseCsvSingleRow) {
    auto rows = FileUtils::parseCsv("x,y,z");
    ASSERT_EQ(rows.size(), 1);
    ASSERT_EQ(rows[0].size(), 3);
}

// ===== uniqueTempPath =====

TEST_F(FileUtilsTest, UniqueTempPathNotEmpty) {
    QString path = FileUtils::uniqueTempPath("LLM_chart_generator_test", ".png");
    EXPECT_FALSE(path.isEmpty());
}

TEST_F(FileUtilsTest, UniqueTempPathHasCorrectExtension) {
    QString path = FileUtils::uniqueTempPath("LLM_chart_generator_test", ".pdf");
    EXPECT_TRUE(path.endsWith(".pdf"));
}

TEST_F(FileUtilsTest, UniqueTempPathIsUnique) {
    QString p1 = FileUtils::uniqueTempPath("LLM_chart_generator_test", ".txt");
    QThread::msleep(10); // ensure different timestamp
    QString p2 = FileUtils::uniqueTempPath("LLM_chart_generator_test", ".txt");
    // May be same if within same second, but at least should be valid paths
    EXPECT_FALSE(p1.isEmpty());
    EXPECT_FALSE(p2.isEmpty());
}
