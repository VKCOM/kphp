// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "common/tl/constants/common.h"
#include "common/tl/fetch.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"

namespace exactlyOnce {

struct uuid final {
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
class prepareRequest final {
  static constexpr int32_t PREPARE_REQUEST_MAGIC = 0xc8d71b66;

public:
  uuid persistent_query_uuid{};

  bool fetch() {
    const int32_t magic{tl_fetch_int()};
    return !tl_fetch_error() && magic == PREPARE_REQUEST_MAGIC && persistent_query_uuid.fetch();
  }

  void write() const {
    tl_store_int(PREPARE_REQUEST_MAGIC);
    persistent_query_uuid.write();
  }
};

class commitRequest final {
  static constexpr int32_t COMMIT_REQUEST_MAGIC = 0x6836b983;

public:
  uuid persistent_query_uuid{};
  uuid persistent_slot_uuid{};

  bool fetch() {
    const int32_t magic{tl_fetch_int()};
    return !tl_fetch_error() && magic == COMMIT_REQUEST_MAGIC && persistent_query_uuid.fetch() && persistent_slot_uuid.fetch();
  }

  void write() const {
    tl_store_int(COMMIT_REQUEST_MAGIC);
    persistent_query_uuid.write();
    persistent_slot_uuid.write();
  }
};

struct PersistentRequest final {
  std::variant<prepareRequest, commitRequest> request;

  bool fetch() {
    tl_fetch_mark();
    if (exactlyOnce::prepareRequest prepare_request{}; prepare_request.fetch()) {
      request.emplace<prepareRequest>(prepare_request);
      return true;
    }
    tl_fetch_mark_restore();
    if (exactlyOnce::commitRequest commit_request{}; commit_request.fetch()) {
      request.emplace<commitRequest>(commit_request);
      return true;
    }
    return false;
  }

  void write() const {
    std::visit([](const auto& value) { value.write(); }, request);
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
  static constexpr uint32_t RETURN_RESERVED_STATUS_0_FLAG = vk::tl::common::tracing::traceContext::return_reserved_status_0;
  static constexpr uint32_t RETURN_RESERVED_STATUS_1_FLAG = vk::tl::common::tracing::traceContext::return_reserved_status_1;
  static constexpr uint32_t PARENT_ID_FLAG = vk::tl::common::tracing::traceContext::parent_id;
  static constexpr uint32_t SOURCE_ID_FLAG = vk::tl::common::tracing::traceContext::source_id;
  static constexpr uint32_t RETURN_RESERVED_LEVEL_0_FLAG = vk::tl::common::tracing::traceContext::return_reserved_level_0;
  static constexpr uint32_t RETURN_RESERVED_LEVEL_1_FLAG = vk::tl::common::tracing::traceContext::return_reserved_level_1;
  static constexpr uint32_t RETURN_RESERVED_LEVEL_2_FLAG = vk::tl::common::tracing::traceContext::return_reserved_level_2;
  static constexpr uint32_t RETURN_DEBUG_FLAG = vk::tl::common::tracing::traceContext::return_debug;

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
      vk::tl::fetch_string(value);
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
