#include <gtest/gtest.h>

#include "zstd/zstd.h"

#include "runtime-common/stdlib/string/string-context.h"

TEST(zstd_test, test_bounds) {
  ASSERT_LE(ZSTD_CStreamOutSize(), StringLibContext::STATIC_BUFFER_LENGTH);
  ASSERT_LE(ZSTD_CStreamInSize(), StringLibContext::STATIC_BUFFER_LENGTH);

  ASSERT_LE(ZSTD_DStreamOutSize(), StringLibContext::STATIC_BUFFER_LENGTH);
  ASSERT_LE(ZSTD_DStreamInSize(), StringLibContext::STATIC_BUFFER_LENGTH);
}
