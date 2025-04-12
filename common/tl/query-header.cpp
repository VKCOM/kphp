// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl/query-header.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <optional>

#include "common/binlog/kdb-binlog-common.h"

#include "common/precise-time.h"
#include "common/rpc-error-codes.h"
#include "common/server/main-binlog.h"
#include "common/stats/provider.h"
#include "common/tl/constants/common.h"
#include "common/tl/fetch.h"
#include "common/tl/methods/compression.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"

static int tl_fetch_query_flags(tl_query_header_t *header) {
  namespace flag = vk::tl::common::rpc_invoke_req_extra_flags;
  int flags = tl_fetch_int();
  if (tl_fetch_error()) {
    return -1;
  }
  if (header->flags & flags) {
    tl_fetch_set_error_format(TL_ERROR_HEADER, "Duplicate flags in header 0x%08x", header->flags & flags);
    return -1;
  }
  if (flags & ~flag::ALL) {
    tl_fetch_set_error_format(TL_ERROR_HEADER, "Unsupported flags in header 0x%08x", (~flag::ALL) & flags);
    return -1;
  }
  header->flags |= flags;
  if (flags & flag::wait_binlog_pos) {
    header->wait_binlog_pos = tl_fetch_long();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::string_forward_keys) {
    vk::tl::fetch_vector(header->string_forward_keys, 10, vk::tl::fetcher<std::string>(1000));
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::int_forward_keys) {
    vk::tl::fetch_vector(header->int_forward_keys, 10);
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::string_forward) {
    vk::tl::fetch_string(header->string_forward, 1000);
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::int_forward) {
    header->int_forward = tl_fetch_long();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::custom_timeout_ms) {
    header->custom_timeout = tl_fetch_int();
    if (tl_fetch_error()) {
      return -1;
    }
    if (header->custom_timeout < 0) {
      header->custom_timeout = 0;
    }
    if (header->custom_timeout > 120000) {
      header->custom_timeout = 120000;
    }
  }
  if (flags & flag::supported_compression_version) {
    header->supported_compression_version = tl_fetch_int();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::random_delay) {
    header->random_delay = tl_fetch_double_in_range(0, 60.0);
    if (tl_fetch_error()) {
      return -1;
    }
  }
  return 0;
}

bool tl_fetch_query_header(tl_query_header_t *header) {
  assert (header);
  if (vk::tl::fetch_magic(TL_RPC_INVOKE_REQ)) {
    header->qid = tl_fetch_long();
    while (!tl_fetch_error() && tl_fetch_unread()) {
      uint32_t op = tl_fetch_lookup_int();
      if (op == TL_RPC_DEST_ACTOR) {
        assert (tl_fetch_int() == (int)TL_RPC_DEST_ACTOR);
        header->actor_id = tl_fetch_long();
      } else if (op == TL_RPC_DEST_ACTOR_FLAGS) {
        assert (tl_fetch_int() == (int)TL_RPC_DEST_ACTOR_FLAGS);
        header->actor_id = tl_fetch_long();
        tl_fetch_query_flags(header);
      } else if (op == TL_RPC_DEST_FLAGS) {
        assert (tl_fetch_int() == (int)TL_RPC_DEST_FLAGS);
        tl_fetch_query_flags(header);
      } else {
        break;
      }
    }
  } else {
    tl_fetch_reset_error();
    tl_fetch_set_error(TL_ERROR_HEADER, "Expected RPC_INVOKE_REQ");
  }
  if (tl_fetch_error()) {
    return false;
  }
  return true;
}

static int tl_fetch_query_answer_flags(tl_query_answer_header_t *header) {
  namespace flag = vk::tl::common::rpc_req_result_extra_flags;
  int flags = tl_fetch_int();
  if (tl_fetch_error()) {
    return -1;
  }
  if (header->flags & flags) {
    tl_fetch_set_error_format(TL_ERROR_HEADER, "Duplicate flags in header 0x%08x", header->flags & flags);
    return -1;
  }
  if (flags & ~flag::ALL) {
    tl_fetch_set_error_format(TL_ERROR_HEADER, "Unsupported flags in header 0x%08x", (~flag::ALL) & flags);
    return -1;
  }
  header->flags |= flags;
  if (flags & flag::binlog_pos) {
    header->binlog_pos = tl_fetch_long();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::binlog_time) {
    header->binlog_time = tl_fetch_long();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::engine_pid) {
    tl_fetch_raw_data(&header->PID, sizeof(header->PID));
    if (tl_fetch_error()) {
      return -1;
    }
  }
  static_assert(flag::request_size == flag::response_size, "same bit");
  if (flags & flag::request_size) {
    header->request_size = tl_fetch_int();
    header->result_size = tl_fetch_int();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::failed_subqueries) {
    header->failed_subqueries = tl_fetch_int();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::compression_version) {
    header->compression_version = tl_fetch_int();
    if (tl_fetch_error()) {
      return -1;
    }
  }
  if (flags & flag::stats) {
    int n = tl_fetch_int();
    if (tl_fetch_error() || n != 2) {
      tl_fetch_set_error_format(TL_ERROR_HEADER, "Unexpected number of stats: %d (expected 2)", n);
      return -1;
    }

    static const int max_len = 64;
    static char buf[max_len];

    auto fetch_stat = [](const char *expected_name, int idx) -> std::optional<double> {
      tl_fetch_string0(buf, max_len);
      if (tl_fetch_error() || strcmp(buf, expected_name)) {
        tl_fetch_set_error_format(TL_ERROR_HEADER, "Stat #%d should be '%s', not '%s'", idx, expected_name, buf);
        return std::nullopt;
      }
      double res;
      tl_fetch_string0(buf, max_len);
      if (tl_fetch_error() || sscanf(buf, "%lf", &res) != 1) {
        tl_fetch_set_error_format(TL_ERROR_HEADER, "Stat #%d value is expected to be double, not '%s'", idx, buf);
        return std::nullopt;
      }
      return std::make_optional(res);
    };

    auto proxy_got_query_ts = fetch_stat("proxy_got_query_ts", 1);
    if (!proxy_got_query_ts.has_value()) {
      return -1;
    }
    auto got_answer_from_engine_ts = fetch_stat("got_answer_from_engine_ts", 2);
    if (!got_answer_from_engine_ts.has_value()) {
      return -1;
    }

    header->stats_result.proxy_got_query_ts = proxy_got_query_ts.value();
    header->stats_result.got_answer_from_engine_ts = got_answer_from_engine_ts.value();
  }
  return 0;
}

bool tl_fetch_query_answer_header(tl_query_answer_header_t *header) {
  assert (header);
  int op = tl_fetch_int();
  if (op == TL_RPC_REQ_RESULT) {
    header->type = result_header_type::result;
  } else if (op == TL_RPC_REQ_ERROR) {
    header->type = result_header_type::error;
  } else {
    tl_fetch_set_error(TL_ERROR_HEADER, "Expected RPC_REQ_ERROR or RPC_REQ_RESULT");
    return false;
  }
  header->qid = tl_fetch_long();
  while (!tl_fetch_error() && tl_fetch_unread() && header->type != result_header_type::error) {
    uint32_t op = tl_fetch_lookup_int();
    if (op == TL_RPC_REQ_ERROR) {
      assert (tl_fetch_int() == TL_RPC_REQ_ERROR);
      header->type = result_header_type::wrapped_error;
      tl_fetch_long();
    } else if (op == RPC_REQ_ERROR_WRAPPED) {
      header->type = result_header_type::wrapped_error;
      assert (tl_fetch_int() == RPC_REQ_ERROR_WRAPPED);
    } else if (op == TL_REQ_RESULT_HEADER) {
      assert (tl_fetch_int() == (int)TL_REQ_RESULT_HEADER);
      tl_fetch_query_answer_flags(header);
      if (header->flags & vk::tl::common::rpc_req_result_extra_flags::compression_version) {
        if (!tl_fetch_error()) {
          tl_decompress_remaining(header->compression_version);
          header->flags &= ~vk::tl::common::rpc_req_result_extra_flags::compression_version;
          header->compression_version = COMPRESSION_VERSION_NONE;
        }
      }
    } else {
      break;
    }
  }
  if (tl_fetch_error()) {
    return false;
  }
  if (header->is_error()) {
    header->error_code = tl_fetch_int();
    vk::tl::fetch_string(header->error);
  } else {
  }
  return true;
}

static void tl_store_stats_result(const tl_stats_result_t *stats) {
  static auto double_to_string0 = [](double x) {
    const size_t buf_size = 30;
    static char buf[buf_size];
    snprintf(buf, buf_size, "%.9f", x);
    return buf;
  };

  tl_store_int(2);
  tl_store_string0("proxy_got_query_ts");
  tl_store_string0(double_to_string0(stats->proxy_got_query_ts));
  tl_store_string0("got_answer_from_engine_ts");
  tl_store_string0(double_to_string0(stats->got_answer_from_engine_ts));
}

void tl_store_header(const tl_query_header_t *header) {
  assert (tl_store_check(0) >= 0);
  tl_store_int(TL_RPC_INVOKE_REQ);
  tl_store_long(header->qid);
  if (header->actor_id || header->flags) {
    namespace flag = vk::tl::common::rpc_invoke_req_extra_flags;
    int flags = header->flags;
    if (header->string_forward_keys.empty()) {
      flags &= ~flag::string_forward_keys;
    }
    if (header->int_forward_keys.empty()) {
      flags &= ~flag::int_forward_keys;
    }
    if (flags) {
      tl_store_int(TL_RPC_DEST_ACTOR_FLAGS);
      tl_store_long(header->actor_id);
      tl_store_int(flags);
      if (flags & flag::wait_binlog_pos) {
        tl_store_long(header->wait_binlog_pos);
      }
      if (flags & flag::string_forward_keys) {
        vk::tl::store_vector(header->string_forward_keys);
      }
      if (flags & flag::int_forward_keys) {
        vk::tl::store_vector(header->int_forward_keys);
      }
      if (flags & flag::string_forward) {
        vk::tl::store_string(header->string_forward);
      }
      if (flags & flag::int_forward) {
        tl_store_long(header->int_forward);
      }
      if (flags & flag::custom_timeout_ms) {
        tl_store_int(header->custom_timeout);
      }
      if (flags & flag::supported_compression_version) {
        tl_store_int(header->supported_compression_version);
      }
      if (flags & flag::random_delay) {
        tl_store_double(header->random_delay);
      }
    } else if (header->actor_id) {
      tl_store_int(TL_RPC_DEST_ACTOR);
      tl_store_long(header->actor_id);
    }
  }
}

void tl_store_answer_header(const tl_query_answer_header_t *header) {
  assert (tl_store_check(0) >= 0);
  switch (header->type) {
    case result_header_type::wrapped_error: {
      tl_store_int(TL_RPC_REQ_ERROR);
      tl_store_long(tl_current_query_id());
    }
      /* fallthrough */
    case result_header_type::error: {
      tl_store_int(header->error_code);
      vk::tl::store_string(header->error);
      break;
    }
    case result_header_type::result: {
      if (header->flags) {
        namespace flag = vk::tl::common::rpc_req_result_extra_flags;
        tl_store_int(TL_REQ_RESULT_HEADER);
        tl_store_int(header->flags);
        if (header->flags & flag::binlog_pos) {
          tl_store_long(header->binlog_pos);
        }
        if (header->flags & flag::binlog_time) {
          tl_store_long(header->binlog_time);
        }
        if (header->flags & flag::engine_pid) {
          tl_store_raw_data(&header->PID, sizeof(header->PID));
        }
        if (header->flags & flag::request_size) {
          tl_store_int(header->request_size);
          tl_store_int(header->result_size);
        }
        if (header->flags & flag::failed_subqueries) {
          tl_store_int(header->failed_subqueries);
        }
        if (header->flags & flag::compression_version) {
          tl_store_int(header->compression_version);
        }
        if (header->flags & flag::stats) {
          tl_store_stats_result(&header->stats_result);
        }
      }
      break;
    }
  }
}


void set_result_header_values(tl_query_answer_header_t *header, int flags) {
  namespace request_flag = vk::tl::common::rpc_invoke_req_extra_flags;
  namespace result_flag = vk::tl::common::rpc_req_result_extra_flags;

  header->type = result_header_type::result;
  if (flags & request_flag::return_binlog_pos) {
    header->flags |= result_flag::binlog_pos;
    header->binlog_pos = log_last_pos();
  }
  if (flags & request_flag::return_binlog_time) {
    header->flags |= result_flag::binlog_time;
    header->binlog_time = precise_time_to_binlog_time(get_precise_time(1000000));
  }
  if (flags & request_flag::return_pid) {
    header->flags |= result_flag::engine_pid;
    memcpy(&header->PID, &PID, sizeof(PID));
  }
  if (flags & request_flag::return_request_sizes) {
    header->flags |= result_flag::request_size;
    header->request_size = tl_bytes_fetched();
    header->result_size = tl_bytes_stored();
  }
  if (flags & request_flag::return_failed_subqueries) {
    header->flags |= result_flag::failed_subqueries;
    header->failed_subqueries = 0;
  }

  // TODO: process request_flag::return_query_stats
  //if (flags & request_flag::return_query_stats) {
  //  header->flags |= result_flag::stats;
  //  header->stats_result = ...???;
  //}
}
