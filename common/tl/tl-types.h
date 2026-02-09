// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
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
    if (tl_fetch_error()) {
      return false;
    }
    hi = tl_fetch_long();
    return !tl_fetch_error();
  }

  void store() const {
    tl_store_long(lo);
    tl_store_long(hi);
  }
};

struct prepareRequest final {
  exactlyOnce::uuid persistent_query_uuid{};

  bool fetch() {
    return persistent_query_uuid.fetch();
  }

  void store() const {
    persistent_query_uuid.store();
  }
};

struct commitRequest final {
  exactlyOnce::uuid persistent_query_uuid{};
  exactlyOnce::uuid persistent_slot_uuid{};

  bool fetch() {
    return persistent_query_uuid.fetch() && persistent_slot_uuid.fetch();
  }

  void store() const {
    persistent_query_uuid.store();
    persistent_slot_uuid.store();
  }
};

class PersistentRequest final {
  static constexpr int32_t PREPARE_REQUEST_MAGIC = 0xc8d7'1b66;
  static constexpr int32_t COMMIT_REQUEST_MAGIC = 0x6836'b983;

public:
  std::variant<exactlyOnce::prepareRequest, exactlyOnce::commitRequest> request;

  bool fetch() {
    const int32_t magic{tl_fetch_int()};
    if (tl_fetch_error()) {
      return false;
    }
    if (exactlyOnce::prepareRequest prepare_request{}; magic == PREPARE_REQUEST_MAGIC && prepare_request.fetch()) {
      request.emplace<exactlyOnce::prepareRequest>(prepare_request);
      return true;
    }
    if (exactlyOnce::commitRequest commit_request{}; magic == COMMIT_REQUEST_MAGIC && commit_request.fetch()) {
      request.emplace<exactlyOnce::commitRequest>(commit_request);
      return true;
    }
    return false;
  }

  void store() const {
    std::visit(
        [](const auto& value) {
          using value_t = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<value_t, exactlyOnce::prepareRequest>) {
            tl_store_int(PREPARE_REQUEST_MAGIC);
          } else if constexpr (std::is_same_v<value_t, exactlyOnce::commitRequest>) {
            tl_store_int(COMMIT_REQUEST_MAGIC);
          } else {
            // condition is strange because of c++17 compiler. It is equivalent to `false`
            static_assert(sizeof(value_t) && false, "exactlyOnce::PersistentRequest only supports prepareRequest and commitRequest");
          }
          value.store();
        },
        request);
  }
};
} // namespace exactlyOnce

namespace tracing {

struct traceID final {
  int64_t lo{};
  int64_t hi{};

  bool fetch() {
    lo = tl_fetch_long();
    if (tl_fetch_error()) {
      return false;
    }
    hi = tl_fetch_long();
    return !tl_fetch_error();
  }

  void store() const {
    tl_store_long(lo);
    tl_store_long(hi);
  }
};

class traceContext final {
  static constexpr uint32_t RESERVED_STATUS_0_FLAG = vk::tl::common::tracing::trace_context_flags::reserved_status_0;
  static constexpr uint32_t RESERVED_STATUS_1_FLAG = vk::tl::common::tracing::trace_context_flags::reserved_status_1;
  static constexpr uint32_t PARENT_ID_FLAG = vk::tl::common::tracing::trace_context_flags::parent_id;
  static constexpr uint32_t SOURCE_ID_FLAG = vk::tl::common::tracing::trace_context_flags::source_id;
  static constexpr uint32_t RESERVED_LEVEL_0_FLAG = vk::tl::common::tracing::trace_context_flags::reserved_level_0;
  static constexpr uint32_t RESERVED_LEVEL_1_FLAG = vk::tl::common::tracing::trace_context_flags::reserved_level_1;
  static constexpr uint32_t RESERVED_LEVEL_2_FLAG = vk::tl::common::tracing::trace_context_flags::reserved_level_2;
  static constexpr uint32_t DEBUG_FLAG = vk::tl::common::tracing::trace_context_flags::debug;

public:
  tracing::traceID trace_id{};
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
    const int32_t fields_mask{tl_fetch_int()};
    bool ok{!tl_fetch_error()};

    ok = ok && trace_id.fetch();
    if (ok && static_cast<bool>(fields_mask & PARENT_ID_FLAG)) {
      opt_parent_id.emplace(tl_fetch_long());
      ok = ok && !tl_fetch_error();
    }
    if (ok && static_cast<bool>(fields_mask & SOURCE_ID_FLAG)) {
      std::string value;
      vk::tl::fetch_string(value);
      opt_source_id.emplace(std::move(value));
      ok = ok && !tl_fetch_error();
    }

    reserved_status_0 = static_cast<bool>(fields_mask & RESERVED_STATUS_0_FLAG);
    reserved_status_1 = static_cast<bool>(fields_mask & RESERVED_STATUS_1_FLAG);
    reserved_level_0 = static_cast<bool>(fields_mask & RESERVED_LEVEL_0_FLAG);
    reserved_level_1 = static_cast<bool>(fields_mask & RESERVED_LEVEL_1_FLAG);
    reserved_level_2 = static_cast<bool>(fields_mask & RESERVED_LEVEL_2_FLAG);
    debug_flag = static_cast<bool>(fields_mask & DEBUG_FLAG);

    return ok;
  }

  void store() const {
    tl_store_int(get_flags());
    trace_id.store();
    if (opt_parent_id) {
      tl_store_long(opt_parent_id.value());
    }
    if (opt_source_id) {
      vk::tl::store_string(opt_source_id.value());
    }
  }

  uint32_t get_flags() const noexcept {
    uint32_t flags{};
    flags |= static_cast<uint32_t>(reserved_status_0);
    flags |= static_cast<uint32_t>(reserved_status_1) << 1;
    flags |= static_cast<uint32_t>(reserved_level_0) << 4;
    flags |= static_cast<uint32_t>(reserved_level_1) << 5;
    flags |= static_cast<uint32_t>(reserved_level_2) << 6;
    flags |= static_cast<uint32_t>(debug_flag) << 7;

    flags |= static_cast<uint32_t>(opt_parent_id.has_value()) << 2;
    flags |= static_cast<uint32_t>(opt_source_id.has_value()) << 3;
    return flags;
  }
};
} // namespace tracing
