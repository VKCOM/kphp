// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "common/pid.h"
#include "common/tl/constants/common.h"

enum class result_header_type {
  result,
  error,
  wrapped_error
};

struct tl_stats_result_t {
  double proxy_got_query_ts;
  double got_answer_from_engine_ts;
};

struct tl_query_header_t {
  long long qid{};
  long long actor_id{};
  int flags{};
  long long int_forward{};
  long long wait_binlog_pos{};
  int custom_timeout{};
  std::string string_forward;
  std::vector<long long> int_forward_keys;
  std::vector<std::string> string_forward_keys;
  int supported_compression_version{};
  double random_delay{};
};

struct tl_query_answer_header_t {
  result_header_type type{};
  long long qid{};
  int flags{};
  long long binlog_pos{};
  long long binlog_time{};
  int request_size{};
  int result_size{};
  int failed_subqueries{};
  struct process_id PID{};
  int compression_version{};
  tl_stats_result_t stats_result{};
  int error_code;
  std::string error;
  bool is_error() const {
    return type == result_header_type::error || type == result_header_type::wrapped_error;
  }
};

bool tl_fetch_query_header(tl_query_header_t *header);
bool tl_fetch_query_answer_header(tl_query_answer_header_t *header);
void tl_store_header(const tl_query_header_t *header);
void tl_store_answer_header(const tl_query_answer_header_t *header);

void set_result_header_values(tl_query_answer_header_t *header, int flags);

#define RPC_REQ_ERROR_WRAPPED           (TL_RPC_REQ_ERROR + 1)

