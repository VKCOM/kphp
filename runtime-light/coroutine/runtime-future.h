// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <variant>

struct fork_future {
  int64_t awaited_fork_id;
  uint64_t timeout_d;

  explicit fork_future(int64_t fork_id)
    : awaited_fork_id(fork_id)
    , timeout_d(0) {}
  fork_future()
    : awaited_fork_id(-1)
    , timeout_d(0) {}
};

struct stream_future {
  uint64_t stream_d;
  uint64_t timeout_d;

  explicit stream_future(uint64_t stream)
    : stream_d(stream)
    , timeout_d(0) {}
  stream_future()
    : stream_d(0)
    , timeout_d(0) {}
};

namespace std {
template<>
struct hash<fork_future> {
  size_t operator()(const fork_future &f) const {
    return std::hash<int64_t>()(f.awaited_fork_id);
  }
};

template<>
struct hash<stream_future> {
  size_t operator()(const stream_future &s) const {
    return std::hash<uint64_t>()(s.stream_d);
  }
};
} // namespace std

inline bool operator==(const fork_future &lhs, const fork_future &rhs) {
  return lhs.awaited_fork_id == rhs.awaited_fork_id;
}

inline bool operator==(const stream_future &lhs, const stream_future &rhs) {
  return lhs.stream_d == rhs.stream_d;
}

using runtime_future = std::variant<fork_future, stream_future>;
