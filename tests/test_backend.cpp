#include "../backend/Backend.hpp"
#include "RNG.hpp"
#include <gtest/gtest.h>

TEST(BackendTest, BellStateHistogram) {
  Backend backend;
  backend.load(String(QUELL_TEST_DATA_DIR "/bell.quell"));

  RNG rng(1234);
  backend.run(rng);

  const auto &hist = backend.get_histogram();

  // We only expect outcomes 00 (0) and 11 (3) for a Bell state.
  EXPECT_TRUE(hist.contains(0) || hist.contains(3));
  if (hist.contains(0)) {
    EXPECT_GT(hist.at(0), 0u);
  }
  if (hist.contains(3)) {
    EXPECT_GT(hist.at(3), 0u);
  }

  // Should not contain 01 or 10
  EXPECT_FALSE(hist.contains(1));
  EXPECT_FALSE(hist.contains(2));
}
