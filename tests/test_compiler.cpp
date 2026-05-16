#include "../compiler/Compiler.hpp"
#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include <gtest/gtest.h>

#include <fstream>
#include <stdexcept>
#include <string>

static DynamicArray<String> read_lines(const char* filepath) {
  std::ifstream file(filepath);
  if (!file) throw std::runtime_error("Failed to open file");

  DynamicArray<String> lines;
  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(String(line.c_str()));
  }
  return lines;
}

TEST(CompilerTest, EmitsBellQCorrectly) {
  Compiler c;

  CompileOptions opt;
  opt.emit_shots_header = false;
  opt.emit_end_measure = false;

  CompileResult r =
      c.compile_file_to_q_lines(String(QUELL_TEST_DATA_DIR "/bell.ql"), opt);

  ASSERT_TRUE(r.ok);

  DynamicArray<String> expected = read_lines(QUELL_TEST_DATA_DIR "/bell_expected.q");

  ASSERT_EQ(r.q_lines.get_size(), expected.get_size());
  for (size_t i = 0; i < expected.get_size(); ++i) {
    EXPECT_STREQ(r.q_lines[i].c_str(), expected[i].c_str());
  }
}

TEST(CompilerTest, LowersSwapAndReset) {
  Compiler c;

  CompileOptions opt;
  opt.emit_shots_header = false;
  opt.emit_end_measure = false;

  CompileResult r =
      c.compile_file_to_q_lines(String(QUELL_TEST_DATA_DIR "/swap_reset.ql"), opt);

  ASSERT_TRUE(r.ok);

  DynamicArray<String> expected = read_lines(QUELL_TEST_DATA_DIR "/swap_reset_expected.q");

  ASSERT_EQ(r.q_lines.get_size(), expected.get_size());
  for (size_t i = 0; i < expected.get_size(); ++i) {
    EXPECT_STREQ(r.q_lines[i].c_str(), expected[i].c_str());
  }
}
