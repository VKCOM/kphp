// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>

#include "common/mixin/not_copyable.h"

struct QueriesStat {
  void register_query(int32_t size) noexcept {
    ++queries_count_;
    outgoing_bytes_ += std::max(0, size);
  }

  void register_answer(int32_t size) noexcept {
    incoming_bytes_ += std::max(0, size);
  }

  QueriesStat &operator+=(const QueriesStat &other) noexcept {
    queries_count_ += other.queries_count_;
    incoming_bytes_ += other.incoming_bytes_;
    outgoing_bytes_ += other.outgoing_bytes_;
    return *this;
  }

  QueriesStat &operator-=(const QueriesStat &other) noexcept {
    queries_count_ -= other.queries_count_;
    incoming_bytes_ -= other.incoming_bytes_;
    outgoing_bytes_ -= other.outgoing_bytes_;
    return *this;
  }

  friend inline QueriesStat operator-(const QueriesStat &lhs, const QueriesStat &rhs) noexcept {
    QueriesStat res = lhs;
    res -= rhs;
    return res;
  }

  uint64_t queries_count() const noexcept {
    return queries_count_;
  }
  uint64_t incoming_bytes() const noexcept {
    return incoming_bytes_;
  }
  uint64_t outgoing_bytes() const noexcept {
    return outgoing_bytes_;
  }

private:
  uint64_t queries_count_{0};
  uint64_t incoming_bytes_{0};
  uint64_t outgoing_bytes_{0};
};

class PhpQueriesStats : vk::not_copyable {
public:
  static QueriesStat &get_rpc_queries_stat() noexcept {
    static QueriesStat rpc_stat;
    return rpc_stat;
  }

  static QueriesStat &get_sql_queries_stat() noexcept {
    static QueriesStat sql_stat;
    return sql_stat;
  }

  static QueriesStat &get_mc_queries_stat() noexcept {
    static QueriesStat mc_stat;
    return mc_stat;
  }
};
