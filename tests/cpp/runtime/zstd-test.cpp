#include <gtest/gtest.h>

#include <zstd.h>

#include "runtime/string_functions.h"

TEST(zstd_test, test_bounds) {
  ASSERT_LE(ZSTD_CStreamOutSize(), PHP_BUF_LEN);
  ASSERT_LE(ZSTD_CStreamInSize(), PHP_BUF_LEN);

  ASSERT_LE(ZSTD_DStreamOutSize(), PHP_BUF_LEN);
  ASSERT_LE(ZSTD_DStreamInSize(), PHP_BUF_LEN);
}
