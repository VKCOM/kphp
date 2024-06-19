// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/binlog/binlog-snapshot.h"

#include <tuple>

#include "common/tl/fetch.h"

namespace kphp {
namespace tl {

namespace {

constexpr int32_t RESULT_TRUE_MAGIC{0x3f9c8ef8};

} // namespace

BarsicSnapshotHeader::BarsicSnapshotHeader()
  : fields_mask()
  , dependencies(DEPENDENCIES_BUFFER_SIZE)
  , payload_offset() {}

void BarsicSnapshotHeader::SnapshotDependency::tl_fetch() noexcept {
  std::basic_string<char> buffer{};
  buffer.reserve(STRING_BUFFER_SIZE);

  fields_mask = tl_fetch_int();
  // skip cluster_id
  vk::tl::fetch_string(buffer);
  // skip shard_id
  vk::tl::fetch_string(buffer);
  payload_offset = tl_fetch_long();
}

void BarsicSnapshotHeader::tl_fetch() noexcept {
  std::basic_string<char> buffer{};
  buffer.reserve(STRING_BUFFER_SIZE);

  fields_mask = tl_fetch_int();
  // skip cluster_id
  vk::tl::fetch_string(buffer);
  // skip shard_id
  vk::tl::fetch_string(buffer);
  // skip snapshot_meta
  vk::tl::fetch_string(buffer);
  // skip dependencies
  vk::tl::fetch_vector(dependencies);

  payload_offset = tl_fetch_long();

  // skip engine_version
  vk::tl::fetch_string(buffer);
  // skip creation_time_nano
  std::ignore = tl_fetch_long();
  // skip control_meta
  if (static_cast<bool>(fields_mask & 0x1)) {
    vk::tl::fetch_string(buffer);
  }
}

void TlEngineSnapshotHeader::tl_fetch() noexcept {
  fields_mask = tl_fetch_int();
  binlog_time_sec = tl_fetch_long();

  if (tl_fetch_int() == RESULT_TRUE_MAGIC) {
    file_binlog_crc = tl_fetch_int();
  }
}

} // namespace tl
} // namespace kphp
