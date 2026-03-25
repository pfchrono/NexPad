#include "gtest/gtest.h"
#include "ConfigFile.h"
#include <windows.h>
#include <fstream>
#include <string>

// Helper: write content to a temp file, return its path.
// Caller must DeleteFileA() when done.
static std::string writeTempIni(const std::string& content) {
    char tempDir[MAX_PATH];
    char tempFile[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    GetTempFileNameA(tempDir, "npt", 0, tempFile);
    std::ofstream f(tempFile);
    f << content;
    return std::string(tempFile);
}

TEST(ConfigFile, ParsesKeyValue) {
    std::string path = writeTempIni("key = value\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<std::string>("key", ""), "value");
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, StripsHashComments) {
    std::string path = writeTempIni("# this is a comment\nkey = hello\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<std::string>("key", ""), "hello");
    EXPECT_FALSE(cfg.keyExists("# this is a comment"));
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, MissingKeyReturnsDefault) {
    std::string path = writeTempIni("key = value\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<std::string>("missing", "default"), "default");
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, ParsesInt) {
    std::string path = writeTempIni("count = 42\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<int>("count", 0), 42);
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, ParsesFloat) {
    std::string path = writeTempIni("speed = 0.5\n");
    ConfigFile cfg(path);
    EXPECT_FLOAT_EQ(cfg.getValueOfKey<float>("speed", 0.0f), 0.5f);
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, DecimalValueParsedAsDWORD) {
    std::string path = writeTempIni("button = 32\n");
    ConfigFile cfg(path);
    // Convert::string_to_T uses std::istringstream with >> operator, which parses decimal by default
    EXPECT_EQ(cfg.getValueOfKey<DWORD>("button", 0), 32u);
    DeleteFileA(path.c_str());
}
