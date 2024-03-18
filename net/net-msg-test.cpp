// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <numeric>

#include "net/net-msg.h"

class stream_comparator_t {
public:
  stream_comparator_t(const std::uint8_t *begin, const std::uint8_t *end)
    : last_(begin)
    , end_(end) {}

  void compare(const void *data, int len) {
    EXPECT_LE(len, end_ - last_);
    EXPECT_EQ(memcmp(data, last_, len), 0);
    last_ += len;
  }

  static int compare(void *extra, const void *data, int len) {
    auto *stream_comparator = static_cast<stream_comparator_t *>(extra);
    stream_comparator->compare(data, len);
    return 0;
  }

  bool all_stream_seen() const {
    return last_ == end_;
  }

private:
  const std::uint8_t *last_;
  const std::uint8_t *end_;
};

TEST(net_msg, rwm_init) {
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    EXPECT_EQ(rwm.first, nullptr);
    EXPECT_EQ(rwm.first, rwm.last);
    EXPECT_EQ(rwm.total_bytes, 0);

    rwm_free(&rwm);
  }

  {
    raw_message_t rwm;
    rwm_init(&rwm, 42);

    EXPECT_NE(rwm.first, nullptr);
    EXPECT_EQ(rwm.first, rwm.last);
    EXPECT_EQ(rwm.total_bytes, 42);

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_create) {
  {
    raw_message_t rwm;

    rwm_create(&rwm, nullptr, 0);

    EXPECT_EQ(rwm.first, nullptr);
    EXPECT_EQ(rwm.last, nullptr);
    EXPECT_EQ(rwm.total_bytes, 0);

    rwm_free(&rwm);
  }

  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);

    raw_message_t rwm;
    rwm_create(&rwm, payload.data(), payload.size());

    EXPECT_NE(rwm.first, nullptr);
    EXPECT_EQ(rwm.total_bytes, payload.size());

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_clone) {
  {
    raw_message_t x, y;
    rwm_init(&x, 0);

    rwm_clone(&y, &x);

    EXPECT_EQ(y.total_bytes, 0);
    EXPECT_EQ(y.first, nullptr);
    EXPECT_EQ(y.first, y.last);

    rwm_free(&x);
    rwm_free(&y);
  }

  {
    raw_message_t x, y;
    rwm_init(&x, 16);

    rwm_clone(&y, &x);

    EXPECT_EQ(y.total_bytes, 16);
    EXPECT_NE(y.first, nullptr);
    EXPECT_NE(y.last, nullptr);
    EXPECT_EQ(x.first, y.first);
    EXPECT_EQ(x.last, y.last);

    rwm_free(&x);
    rwm_free(&y);
  }
}

TEST(net_msg, rwm_push_data) {
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data(&rwm, payload.data(), payload.size());

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 2048> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data(&rwm, payload.data(), 1024);
    rwm_push_data(&rwm, payload.data(), 1024);

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_push_data_front) {
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data_front(&rwm, &payload[512], 512);
    rwm_push_data_front(&rwm, &payload[0], 512);

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 2048> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data_front(&rwm, &payload[1024], 1024);
    rwm_push_data_front(&rwm, &payload[0], 1024);

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_fetch_data) {
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data(&rwm, payload.data(), payload.size());

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    std::array<std::uint8_t, 1024> fetched;
    rwm_fetch_data(&rwm, fetched.data(), fetched.size());

    EXPECT_EQ(payload, fetched);
    EXPECT_EQ(rwm.total_bytes, 0);

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_fetch_data_back) {
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data(&rwm, payload.data(), payload.size());

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    std::array<std::uint8_t, 1024> fetched;
    rwm_fetch_data_back(&rwm, fetched.data(), fetched.size());

    EXPECT_EQ(payload, fetched);
    EXPECT_EQ(rwm.total_bytes, 0);

    rwm_free(&rwm);
  }
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data(&rwm, payload.data(), payload.size());

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    std::array<std::uint8_t, 1023> fetched;
    rwm_fetch_data_back(&rwm, fetched.data(), fetched.size());
    EXPECT_EQ(rwm.total_bytes, 1);

    EXPECT_EQ(0, std::memcmp(&payload[1], fetched.data(), fetched.size()));
    EXPECT_EQ(rwm.total_bytes, 1);

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_fetch_lookup) {
  {
    raw_message_t rwm;
    rwm_init(&rwm, 0);

    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    rwm_push_data(&rwm, payload.data(), payload.size());

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_NE(rwm.first, nullptr);

    std::array<std::uint8_t, 1024> fetched;
    rwm_fetch_lookup(&rwm, fetched.data(), fetched.size());

    EXPECT_EQ(payload, fetched);
    EXPECT_EQ(rwm.total_bytes, fetched.size());

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_trunc) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);
    raw_message_t rwm;
    rwm_create(&rwm, payload.data(), payload.size());

    const std::size_t pivot = 512;
    rwm_trunc(&rwm, payload.size() - pivot);
    EXPECT_EQ(rwm.total_bytes, pivot);
    EXPECT_NE(rwm.first, nullptr);

    stream_comparator_t stream_comparator(&payload[0], &payload[pivot]);
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_union) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);

    const std::size_t pivot = 512;
    raw_message_t x;
    rwm_create(&x, &payload[0], pivot);

    raw_message_t y;
    rwm_create(&y, &payload[512], payload.size() - pivot);

    rwm_union(&x, &y);
    EXPECT_EQ(x.total_bytes, payload.size());
    EXPECT_NE(x.first, nullptr);

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&x, x.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&x);
  }
}

TEST(net_msg, rwm_split) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);

    raw_message_t x;
    rwm_create(&x, payload.data(), payload.size());

    raw_message_t y;
    rwm_init(&y, 0);

    const std::size_t pivot = 512;
    rwm_split(&x, &y, pivot);

    EXPECT_NE(x.first, nullptr);
    EXPECT_EQ(x.total_bytes, pivot);
    EXPECT_NE(y.first, nullptr);
    EXPECT_EQ(y.total_bytes, pivot);

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&x, x.total_bytes, stream_comparator_t::compare, &stream_comparator);
    rwm_process(&y, y.total_bytes, stream_comparator_t::compare, &stream_comparator);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&x);
  }
}

TEST(net_msg, fork_message_chain) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(std::begin(payload), std::end(payload), 0);

    raw_message_t x;
    rwm_create(&x, payload.data(), payload.size());
    rwm_push_data(&x, payload.data(), payload.size());
    rwm_push_data(&x, payload.data(), payload.size());

    raw_message_t y;
    rwm_init(&y, 0);
    rwm_clone(&y, &x);

    EXPECT_NE(x.first, nullptr);
    EXPECT_EQ(x.first, y.first);
    EXPECT_EQ(x.first->refcnt, 2);

    fork_message_chain(&x);
    for (auto *mp = x.first; mp != x.last; mp = mp->next) {
      EXPECT_EQ(mp->refcnt, 1);
    }
    for (auto *mp = y.first; mp != y.last; mp = mp->next) {
      EXPECT_EQ(mp->refcnt, 1);
    }
    EXPECT_EQ(x.total_bytes, payload.size() * 3);
    EXPECT_EQ(y.total_bytes, payload.size() * 3);
    EXPECT_NE(x.first, y.first);
  }
}

TEST(net_msg, rwm_prepend_alloc) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(std::begin(payload), std::end(payload), 0);

    raw_message_t rwm;
    rwm_create(&rwm, payload.data(), payload.size());

    EXPECT_NE(rwm.first, nullptr);
    EXPECT_NE(rwm.last, nullptr);
    EXPECT_EQ(rwm.total_bytes, payload.size());

    EXPECT_NE(rwm_prepend_alloc(&rwm, payload.size()), nullptr);
    EXPECT_EQ(rwm.total_bytes, 2 * payload.size());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_postpone_alloc) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(std::begin(payload), std::end(payload), 0);

    raw_message_t rwm;
    rwm_create(&rwm, payload.data(), payload.size());

    EXPECT_NE(rwm.first, nullptr);
    EXPECT_NE(rwm.last, nullptr);
    EXPECT_EQ(rwm.total_bytes, payload.size());

    EXPECT_NE(rwm_postpone_alloc(&rwm, payload.size()), nullptr);
    EXPECT_EQ(rwm.total_bytes, 2 * payload.size());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_process) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);

    raw_message_t rwm;
    rwm_create(&rwm, payload.data(), payload.size());

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);

    EXPECT_EQ(rwm.total_bytes, payload.size());
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(net_msg, rwm_process_and_advance) {
  {
    std::array<std::uint8_t, 1024> payload;
    std::iota(payload.begin(), payload.end(), 0);

    raw_message_t rwm;
    rwm_create(&rwm, payload.data(), payload.size());

    stream_comparator_t stream_comparator(std::begin(payload), std::end(payload));
    rwm_process_and_advance(&rwm, rwm.total_bytes, stream_comparator_t::compare, &stream_comparator);

    EXPECT_EQ(rwm.total_bytes, 0);
    EXPECT_TRUE(stream_comparator.all_stream_seen());

    rwm_free(&rwm);
  }
}

TEST(DISABLED_net_msg, rwm_fork_deep) {
  {
    std::array<std::uint8_t, 512> payload;
    std::iota(std::begin(payload), std::end(payload), 0);

    constexpr std::size_t rwm_count = 4;
    raw_message_t rwm_x[rwm_count], rwm_y[rwm_count];
    for (auto &rwm : rwm_x) {
      rwm_create(&rwm, payload.data(), payload.size());
    }
    for (std::size_t i = 0; i < rwm_count; ++i) {
      rwm_clone(&rwm_y[i], &rwm_x[i]);
    }
    for (const auto &rwm : rwm_x) {
      EXPECT_NE(rwm.first, nullptr);
      EXPECT_NE(rwm.last, nullptr);
      EXPECT_EQ(msg_part_use_count(rwm.first), 2);
    }

    raw_message_t rwm_united;
    rwm_init(&rwm_united, 0);
    for (auto &rwm : rwm_x) {
      raw_message_t rwm_cloned;
      rwm_clone(&rwm_cloned, &rwm);
      rwm_union(&rwm_united, &rwm_cloned);
    }
    msg_part_t *mp = rwm_united.first;
    for (;;) {
      if (!mp) {
        break;
      }
      EXPECT_EQ(msg_part_use_count(mp), 3);
      if (mp == rwm_united.last) {
        break;
      }
      mp = mp->next;
    }

    rwm_fork_deep(&rwm_united);

    mp = rwm_united.first;
    for (;;) {
      if (!mp) {
        break;
      }
      EXPECT_EQ(msg_part_use_count(mp), 1);
      if (mp == rwm_united.last) {
        break;
      }
      mp = mp->next;
    }

    rwm_free(&rwm_united);

    for (const auto &rwm : rwm_x) {
      EXPECT_EQ(msg_part_use_count(rwm.first), 2);
    }
  }
}

TEST(net_msg, rwm_fork_deep2) {
  {
    std::array<std::uint8_t, 2048 + 4> payload;
    std::iota(std::begin(payload), std::end(payload), 0);

    constexpr std::size_t rwm_count = 4;
    raw_message_t rwm_x[rwm_count], rwm_y[rwm_count];
    for (auto &rwm : rwm_x) {
      rwm_create(&rwm, payload.data(), payload.size());
    }
    for (std::size_t i = 0; i < rwm_count; ++i) {
      rwm_clone(&rwm_y[i], &rwm_x[i]);
    }
    for (const auto &rwm : rwm_x) {
      EXPECT_NE(rwm.first, nullptr);
      EXPECT_NE(rwm.last, nullptr);
    }

    raw_message_t rwm_united;
    rwm_init(&rwm_united, 0);
    for (auto &rwm : rwm_x) {
      raw_message_t rwm_cloned;
      rwm_clone(&rwm_cloned, &rwm);
      rwm_union(&rwm_united, &rwm_cloned);
    }
    msg_part_t *mp = rwm_united.first;
    for (;;) {
      if (!mp) {
        break;
      }
      if (mp == rwm_united.last) {
        break;
      }
      mp = mp->next;
    }

    rwm_fork_deep(&rwm_united);

    mp = rwm_united.first;
    for (;;) {
      if (!mp) {
        break;
      }
      EXPECT_EQ(msg_part_use_count(mp), 1);
      if (mp == rwm_united.last) {
        break;
      }
      mp = mp->next;
    }

    rwm_check(&rwm_united);
    rwm_free(&rwm_united);
  }
}

TEST(net_msg, rwm_prepare_iovec) {
  {
    constexpr std::size_t payload_parts = 10;
    std::vector<std::uint8_t> payload;

    raw_message_t rwm;
    rwm_init(&rwm, 0);
    for (std::size_t i = 0; i < payload_parts; ++i) {
      const std::size_t payload_part_size = 1 << i;
      std::vector<std::uint8_t> payload_part(payload_part_size);
      std::iota(payload_part.begin(), payload_part.end(), 0);
      payload.insert(payload.end(), payload_part.begin(), payload_part.end());
      rwm_push_data(&rwm, payload_part.data(), payload_part.size());
    }

    stream_comparator_t stream_comparator(payload.data(), payload.data() + payload.size());
    iovec iovector[payload_parts];
    const std::size_t iovector_size = rwm_prepare_iovec(&rwm, iovector, payload_parts, rwm.total_bytes);
    for (std::size_t i = 0; i < iovector_size; ++i) {
      stream_comparator.compare(iovector[i].iov_base, iovector[i].iov_len);
    }

    EXPECT_TRUE(stream_comparator.all_stream_seen());
  }
}

TEST(net_msg, rwm_union_large) {
  const int msg_size = 16 * 1024 * 1024;
  const int payload_size = 1000;
  const int buffer_size = 512;

  std::vector<raw_message_t> rwms(msg_size / payload_size);

  for (int i = 0; i < rwms.size(); ++i) {
    rwm_create(&rwms[i], nullptr, 0);

    rwms[i].total_bytes = payload_size;
    rwms[i].first_offset = 92;
    rwms[i].last_offset = 68;

    auto *buffer = alloc_msg_buffer(buffer_size);
    auto *mp = new_msg_part(buffer);
    mp->offset = 0;
    mp->len = buffer_size;
    rwms[i].first = mp;

    buffer = alloc_msg_buffer(buffer_size);
    mp = new_msg_part(buffer);
    mp->offset = 0;
    mp->len = buffer_size;
    rwms[i].first->next = mp;

    buffer = alloc_msg_buffer(buffer_size);
    mp = new_msg_part(buffer);
    mp->offset = 0;
    mp->len = 108;
    mp->next = nullptr;
    rwms[i].first->next->next = mp;
    rwms[i].last = mp;
  }

  fork_message_chain(&rwms[0]);
  for (int i = 1; i < rwms.size(); ++i) {
    fork_message_chain(&rwms[i]);
    rwm_union_unique(&rwms[0], &rwms[i]);
  }

  rwm_free(&rwms[0]);
}
