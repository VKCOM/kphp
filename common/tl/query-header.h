// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "common/pid.h"
#include "common/tl/constants/common.h"
#include "common/tl/fetch.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace exactlyOnce {
struct Uuid final {
  int64_t lo{};
  int64_t hi{};

  bool fetch() {
    lo = tl_fetch_long();
    hi = tl_fetch_long();
    return !tl_fetch_error();
  }

  void write() const {
    tl_store_long(lo);
    tl_store_long(hi);
  }
};
struct prepareRequest final {
  Uuid persistent_query_uuid{};

  bool fetch() {
    return persistent_query_uuid.fetch();
  }
};

struct commitRequest final {
  Uuid persistent_query_uuid{};
  Uuid persistent_slot_uuid{};

  bool fetch() {
    return persistent_query_uuid.fetch() && persistent_slot_uuid.fetch();
  }
};

class PersistentRequest final {
  static constexpr int32_t PREPARE_REQUEST_MAGIC = 0xc8d71b66;
  static constexpr int32_t COMMIT_REQUEST_MAGIC = 0x6836b983;

public:
  std::variant<prepareRequest, commitRequest> request;

  bool fetch() {
    const int32_t magic{tl_fetch_int()};
    if (tl_fetch_error()) {
      return false;
    }
    if (exactlyOnce::prepareRequest prepare_request{}; magic == PREPARE_REQUEST_MAGIC && prepare_request.fetch()) {
      request.emplace<prepareRequest>(prepare_request);
      return true;
    }
    if (exactlyOnce::commitRequest commit_request{}; magic == COMMIT_REQUEST_MAGIC && commit_request.fetch()) {
      request.emplace<commitRequest>(commit_request);
      return true;
    }
    return false;
  }

  void write() const {
    std::visit(
        [](const auto& value) {
          using value_t = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<value_t, exactlyOnce::prepareRequest>) {
            tl_store_int(PREPARE_REQUEST_MAGIC);
            value.persistent_query_uuid.write();
          } else if constexpr (std::is_same_v<value_t, exactlyOnce::commitRequest>) {
            tl_store_int(COMMIT_REQUEST_MAGIC);
            value.persistent_query_uuid.write();
            value.persistent_slot_uuid.write();
          } else {
            php_error("unsupported value type");
          }
        },
        request);
  }
};
} // namespace exactlyOnce

namespace tracing {
struct TraceID final {
  int64_t lo{};
  int64_t hi{};

  bool fetch() {
    lo = tl_fetch_long();
    hi = tl_fetch_long();
    return !tl_fetch_error();
  }

  void write() const {
    tl_store_long(lo);
    tl_store_long(hi);
  }
};

class TraceContext final {
  static constexpr uint32_t RETURN_RESERVED_STATUS_0_FLAG = vk::tl::common::tracing_traceContext::return_reserved_status_0;
  static constexpr uint32_t RETURN_RESERVED_STATUS_1_FLAG = vk::tl::common::tracing_traceContext::return_reserved_status_1;
  static constexpr uint32_t PARENT_ID_FLAG = vk::tl::common::tracing_traceContext::parent_id;
  static constexpr uint32_t SOURCE_ID_FLAG = vk::tl::common::tracing_traceContext::source_id;
  static constexpr uint32_t RETURN_RESERVED_LEVEL_0_FLAG = vk::tl::common::tracing_traceContext::return_reserved_level_0;
  static constexpr uint32_t RETURN_RESERVED_LEVEL_1_FLAG = vk::tl::common::tracing_traceContext::return_reserved_level_1;
  static constexpr uint32_t RETURN_RESERVED_LEVEL_2_FLAG = vk::tl::common::tracing_traceContext::return_reserved_level_2;
  static constexpr uint32_t RETURN_DEBUG_FLAG = vk::tl::common::tracing_traceContext::return_debug;

public:
  int32_t fields_mask{};
  tracing::TraceID trace_id{};
  std::optional<int64_t> opt_parent_id;
  std::optional<std::string> opt_source_id;

  // status = reserved_status_0 | (reserved_status_1 << 1)
  // status == 0 - drop
  // status == 1 - record
  // status == 2 - defer
  bool reserved_status_0{};
  bool reserved_status_1{};

  bool reserved_level_0{};
  bool reserved_level_1{};
  bool reserved_level_2{};

  bool debug_flag{};

  bool fetch() {
    fields_mask = tl_fetch_int();
    bool ok{!tl_fetch_error()};

    if (ok) {
      ok &= trace_id.fetch();
    }
    if (ok && static_cast<bool>(fields_mask & PARENT_ID_FLAG)) {
      opt_parent_id.emplace(tl_fetch_long());
      ok &= !tl_fetch_error();
    }
    if (ok && static_cast<bool>(fields_mask & SOURCE_ID_FLAG)) {
      std::string value;
      vk::tl::fetch_string(value, 1000);
      opt_source_id.emplace(std::move(value));
      ok &= !tl_fetch_error();
    }

    reserved_status_0 = static_cast<bool>(fields_mask & RETURN_RESERVED_STATUS_0_FLAG);
    reserved_status_1 = static_cast<bool>(fields_mask & RETURN_RESERVED_STATUS_1_FLAG);
    reserved_level_0 = static_cast<bool>(fields_mask & RETURN_RESERVED_LEVEL_0_FLAG);
    reserved_level_1 = static_cast<bool>(fields_mask & RETURN_RESERVED_LEVEL_1_FLAG);
    reserved_level_2 = static_cast<bool>(fields_mask & RETURN_RESERVED_LEVEL_2_FLAG);
    debug_flag = static_cast<bool>(fields_mask & RETURN_DEBUG_FLAG);

    return ok;
  }

  void write() const {
    tl_store_int(fields_mask);
    trace_id.write();
    if (opt_parent_id) {
      tl_store_long(opt_parent_id.value());
    }
    if (opt_source_id) {
      vk::tl::store_string(opt_source_id.value());
    }
  }
};
} // namespace tracing

enum class result_header_type { result, error, wrapped_error };

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
  exactlyOnce::PersistentRequest persistent_query{};
  tracing::TraceContext trace_context{};
  std::string execution_context;
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
  struct process_id PID {};
  int compression_version{};
  tl_stats_result_t stats_result{};
  int error_code;
  std::string error;
  bool is_error() const {
    return type == result_header_type::error || type == result_header_type::wrapped_error;
  }
};

bool tl_fetch_query_header(tl_query_header_t* header);
bool tl_fetch_query_answer_header(tl_query_answer_header_t* header);
void tl_store_header(const tl_query_header_t* header);
void tl_store_answer_header(const tl_query_answer_header_t* header);

void set_result_header_values(tl_query_answer_header_t* header, int flags);

#define RPC_REQ_ERROR_WRAPPED (TL_RPC_REQ_ERROR + 1)
