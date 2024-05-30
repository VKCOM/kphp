// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace kphp {
namespace tl {

constexpr auto UNEXPECTED_TL_MAGIC_ERROR_FORMAT = "unexpected TL magic 0x%x, expected 0x%x\n";

constexpr auto COMMON_HEADER_META_SIZE = sizeof(int32_t) + sizeof(int64_t);
constexpr auto COMMON_HEADER_HASH_SIZE = 2 * sizeof(int64_t);

constexpr int32_t PMEMCACHED_OLD_INDEX_MAGIC = 0x53407fa0;
constexpr int32_t PMEMCACHED_INDEX_RAM_MAGIC_G3 = 0x65049e9e;
constexpr int32_t BARSIC_SNAPSHOT_HEADER_MAGIC = 0x1d0d1b74;
constexpr int32_t TL_ENGINE_SNAPSHOT_HEADER_MAGIC = 0x4bf8b614;
constexpr int32_t PERSISTENT_CONFIG_V2_SNAPSHOT_BLOCK = 0x501096b7;
constexpr int32_t RPC_QUERIES_SNAPSHOT_QUERY_COMMON = 0x9586c501;
constexpr int32_t SNAPSHOT_MAGIC = 0xf0ec39fb;
constexpr int32_t COMMON_INFO_END = 0x5a9ce5ec;

struct BarsicSnapshotHeader {
  struct SnapshotDependency {
    int32_t fields_mask;
    int64_t payload_offset;

    void tl_fetch() noexcept;
  };

  int32_t fields_mask;
  std::vector<SnapshotDependency> dependencies;
  int64_t payload_offset;

  void tl_fetch() noexcept;

  BarsicSnapshotHeader();

private:
  static constexpr auto STRING_BUFFER_SIZE = 512;
  static constexpr auto DEPENDENCIES_BUFFER_SIZE = 128;
};

struct TlEngineSnapshotHeader {
  int32_t fields_mask{};
  int64_t binlog_time_sec{};
  std::optional<int32_t> file_binlog_crc;

  void tl_fetch() noexcept;
};

} // namespace tl
} // namespace kphp
