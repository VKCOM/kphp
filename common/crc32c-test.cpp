#include "common/crc32c.h"

#include <algorithm>
#include <cstdint>
#include <numeric>

#include <gtest/gtest.h>

TEST(crc32c, rfc3720) {
  {
    const std::uint8_t vector[32] = {0x00};
    const std::uint32_t expected_crc32c = 0x8a9136aa;

    EXPECT_EQ(compute_crc32c(vector, sizeof(vector)), expected_crc32c);
  }
  {
    std::uint8_t vector[32];
    std::fill(std::begin(vector), std::end(vector), 0xff);
    const std::uint32_t expected_crc32c = 0x62a8ab43;

    EXPECT_EQ(compute_crc32c(vector, sizeof(vector)), expected_crc32c);
  }
  {
    std::uint8_t vector[32];
    std::iota(std::begin(vector), std::end(vector), 0);
    const std::uint32_t expected_crc32c = 0x46dd794e;

    EXPECT_EQ(compute_crc32c(vector, sizeof(vector)), expected_crc32c);
  }
  {
    std::uint8_t vector[32];
    std::iota(std::begin(vector), std::end(vector), 0);
    std::reverse(std::begin(vector), std::end(vector));
    const std::uint32_t expected_crc32c = 0x113fdb5c;

    EXPECT_EQ(compute_crc32c(vector, sizeof(vector)), expected_crc32c);
  }
  {
    const std::uint8_t vector[] = {0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18,
                                   0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const std::uint32_t expected_crc32c = 0xd9963a56;

    EXPECT_EQ(compute_crc32c(vector, sizeof(vector)), expected_crc32c);
  }
}
