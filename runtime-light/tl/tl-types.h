// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

#include "common/tl/constants/common.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

struct Bool final {
  bool value{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    const auto magic{tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO)};
    value = magic == TL_BOOL_TRUE;
    return magic == TL_BOOL_TRUE || magic == TL_BOOL_FALSE;
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(value ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }
};

struct i32 final {
  int32_t value{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    const auto opt_value{tlb.fetch_trivial<int32_t>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<int32_t>(value);
  }
};

struct I32 final {
  tl::i32 inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_INT && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(TL_INT);
    inner.store(tlb);
  }
};

struct i64 final {
  int64_t value{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    const auto opt_value{tlb.fetch_trivial<int64_t>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<int64_t>(value);
  }
};

struct I64 final {
  tl::i64 inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_LONG && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(TL_LONG);
    inner.store(tlb);
  }
};

struct f32 final {
  float value{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    const auto opt_value{tlb.fetch_trivial<float>()};
    value = opt_value.value_or(0.0);
    return opt_value.has_value();
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<float>(value);
  }
};

struct F32 final {
  tl::f32 inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_FLOAT && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(TL_FLOAT);
    inner.store(tlb);
  }
};

struct f64 final {
  double value{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    const auto opt_value{tlb.fetch_trivial<double>()};
    value = opt_value.value_or(0.0);
    return opt_value.has_value();
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<double>(value);
  }
};

struct F64 final {
  tl::f64 inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_DOUBLE && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(TL_DOUBLE);
    inner.store(tlb);
  }
};

template<typename T>
struct Maybe final {
  std::optional<T> opt_value{};

  bool fetch(tl::TLBuffer& tlb) noexcept
  requires tl::deserializable<T>
  {
    const auto magic{tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO)};
    if (magic == TL_MAYBE_TRUE) {
      opt_value.emplace();
      return (*opt_value).fetch(tlb);
    } else if (magic == TL_MAYBE_FALSE) {
      opt_value = std::nullopt;
      return true;
    }
    return false;
  }

  void store(tl::TLBuffer& tlb) const noexcept
  requires tl::serializable<T>
  {
    if (opt_value.has_value()) {
      tlb.store_trivial<uint32_t>(TL_MAYBE_TRUE);
      (*opt_value).store(tlb);
    } else {
      tlb.store_trivial<uint32_t>(TL_MAYBE_FALSE);
    }
  }
};

class string final {
  static constexpr auto SMALL_STRING_SIZE_LEN = 1;
  static constexpr auto MEDIUM_STRING_SIZE_LEN = 3;
  static constexpr auto LARGE_STRING_SIZE_LEN = 7;

  static constexpr uint64_t SMALL_STRING_MAX_LEN = 253;
  static constexpr uint64_t MEDIUM_STRING_MAX_LEN = (static_cast<uint64_t>(1) << 24) - 1;
  static constexpr uint64_t LARGE_STRING_MAX_LEN = (static_cast<uint64_t>(1) << 56) - 1;

  static constexpr uint8_t LARGE_STRING_MAGIC = 0xff;
  static constexpr uint8_t MEDIUM_STRING_MAGIC = 0xfe;

public:
  std::string_view value;

  bool fetch(tl::TLBuffer& tlb) noexcept;

  void store(tl::TLBuffer& tlb) const noexcept;
};

struct String final {
  tl::string inner;

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_STRING && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(TL_STRING);
    inner.store(tlb);
  }
};

template<typename T>
struct vector final {
  using vector_t = kphp::stl::vector<T, kphp::memory::script_allocator>;
  vector_t value;

  using iterator = vector_t::iterator;
  using const_iterator = vector_t::const_iterator;

  constexpr iterator begin() noexcept {
    return value.begin();
  }
  constexpr iterator end() noexcept {
    return value.end();
  }
  constexpr const_iterator begin() const noexcept {
    return value.begin();
  }
  constexpr const_iterator end() const noexcept {
    return value.end();
  }
  constexpr const_iterator cbegin() const noexcept {
    return value.cbegin();
  }
  constexpr const_iterator cend() const noexcept {
    return value.cend();
  }

  constexpr size_t size() const noexcept {
    return value.size();
  }

  bool fetch(tl::TLBuffer& tlb) noexcept
  requires tl::deserializable<T>
  {
    int64_t size{tlb.fetch_trivial<uint32_t>().value_or(-1)};
    if (size < 0) [[unlikely]] {
      return false;
    }

    value.clear();
    value.reserve(static_cast<size_t>(size));
    for (auto i = 0; i < size; ++i) {
      if (T t{}; t.fetch(tlb)) [[likely]] {
        value.emplace_back(std::move(t));
        continue;
      }
      return false;
    }

    return true;
  }

  void store(tl::TLBuffer& tlb) const noexcept
  requires tl::serializable<T>
  {
    tlb.store_trivial<int32_t>(static_cast<int32_t>(value.size()));
    std::for_each(value.cbegin(), value.cend(), [&tlb](const auto& elem) noexcept { elem.store(tlb); });
  }
};

template<typename T>
struct Vector final {
  tl::vector<T> inner{};

  using iterator = tl::vector<T>::iterator;
  using const_iterator = tl::vector<T>::const_iterator;

  constexpr iterator begin() noexcept {
    return inner.begin();
  }
  constexpr iterator end() noexcept {
    return inner.end();
  }
  constexpr const_iterator begin() const noexcept {
    return inner.begin();
  }
  constexpr const_iterator end() const noexcept {
    return inner.end();
  }
  constexpr const_iterator cbegin() const noexcept {
    return inner.cbegin();
  }
  constexpr const_iterator cend() const noexcept {
    return inner.cend();
  }

  constexpr size_t size() const noexcept {
    return inner.size();
  }

  bool fetch(tl::TLBuffer& tlb) noexcept
  requires tl::deserializable<T>
  {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_VECTOR && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept
  requires tl::serializable<T>
  {
    tlb.store_trivial<uint32_t>(TL_VECTOR);
    inner.store(tlb);
  }
};

template<typename T>
struct dictionaryField final {
  tl::string key;
  T value{};

  bool fetch(tl::TLBuffer& tlb) noexcept
  requires tl::deserializable<T>
  {
    return key.fetch(tlb) && value.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept
  requires tl::serializable<T>
  {
    key.store(tlb);
    value.store(tlb);
  }
};

template<typename T>
struct dictionary final {
  tl::vector<tl::dictionaryField<T>> value{};

  using iterator = tl::vector<tl::dictionaryField<T>>::iterator;
  using const_iterator = tl::vector<tl::dictionaryField<T>>::const_iterator;

  constexpr iterator begin() noexcept {
    return value.begin();
  }
  constexpr iterator end() noexcept {
    return value.end();
  }
  constexpr const_iterator begin() const noexcept {
    return value.begin();
  }
  constexpr const_iterator end() const noexcept {
    return value.end();
  }
  constexpr const_iterator cbegin() const noexcept {
    return value.cbegin();
  }
  constexpr const_iterator cend() const noexcept {
    return value.cend();
  }

  constexpr size_t size() const noexcept {
    return value.size();
  }

  bool fetch(tl::TLBuffer& tlb) noexcept
  requires tl::deserializable<T>
  {
    return value.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept
  requires tl::serializable<T>
  {
    value.store(tlb);
  }
};

template<typename T>
struct Dictionary final {
  tl::dictionary<T> inner{};

  using iterator = tl::dictionary<T>::iterator;
  using const_iterator = tl::dictionary<T>::const_iterator;

  constexpr iterator begin() noexcept {
    return inner.begin();
  }
  constexpr iterator end() noexcept {
    return inner.end();
  }
  constexpr const_iterator begin() const noexcept {
    return inner.begin();
  }
  constexpr const_iterator end() const noexcept {
    return inner.end();
  }
  constexpr const_iterator cbegin() const noexcept {
    return inner.cbegin();
  }
  constexpr const_iterator cend() const noexcept {
    return inner.cend();
  }

  constexpr size_t size() const noexcept {
    return inner.size();
  }

  bool fetch(tl::TLBuffer& tlb) noexcept
  requires tl::deserializable<T>
  {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_DICTIONARY && inner.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept
  requires tl::serializable<T>
  {
    tlb.store_trivial<uint32_t>(TL_DICTIONARY);
    inner.store(tlb);
  }
};

// ===== JOB WORKERS =====

class K2JobWorkerResponse final {
  static constexpr uint32_t MAGIC = 0x3afb'3a08;

public:
  tl::i64 job_id{};
  tl::string body;

  bool fetch(tl::TLBuffer& tlb) noexcept;

  void store(tl::TLBuffer& tlb) const noexcept;
};

// ===== CRYPTO =====

class CertInfoItem final {
  enum Magic : uint32_t { LONG = 0x533f'f89f, STR = 0xc427'feef, DICT = 0x1ea8'a774 };

public:
  std::variant<tl::i64, tl::string, tl::dictionary<tl::string>> data;

  bool fetch(tl::TLBuffer& tlb) noexcept;

  template<class... Ts>
  struct MakeVisitor : Ts... {
    using Ts::operator()...;
  };
};

enum HashAlgorithm : uint32_t {
  DSS1 = 0xf572'd6b6,
  SHA1 = 0x215f'b97d,
  SHA224 = 0x8bce'55e9,
  SHA256 = 0x6c97'7f8c,
  SHA384 = 0xf54c'2608,
  SHA512 = 0x225d'f2b6,
  RMD160 = 0x1887'e6b4,
  MD5 = 0x257d'df13,
  MD4 = 0x317f'e3d1,
  MD2 = 0x5aca'6998,
};

enum CipherAlgorithm : uint32_t {
  AES128 = 0xe627'c460,
  AES256 = 0x4c98'c1f9,
};

enum BlockPadding : uint32_t {
  PKCS7 = 0x699e'c5de,
};

// ===== CONFDATA =====

struct confdataValue final {
  tl::string value;
  tl::Bool is_php_serialized{};
  tl::Bool is_json_serialized{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return value.fetch(tlb) && is_php_serialized.fetch(tlb) && is_json_serialized.fetch(tlb);
  }
};

// ===== HTTP =====

struct HttpVersion final {
private:
  static constexpr std::string_view V09_SV = "0.9";
  static constexpr std::string_view V10_SV = "1.0";
  static constexpr std::string_view V11_SV = "1.1";
  static constexpr std::string_view V2_SV = "2";
  static constexpr std::string_view V3_SV = "3";

public:
  enum class Version : uint32_t {
    Invalid = TL_ZERO,
    V09 = 0x9e15'd325,
    V10 = 0xfed2'5ac0,
    V11 = 0x89d5'6a56,
    V2 = 0x9022'9734,
    V3 = 0xe725'a7a2,
  };

  Version version{Version::Invalid};

  constexpr std::string_view string_view() const noexcept {
    switch (version) {
    case Version::V09:
      return V09_SV;
    case Version::V10:
      return V10_SV;
    case Version::V11:
      return V11_SV;
    case Version::V2:
      return V2_SV;
    case Version::V3:
      return V3_SV;
    default:
      return {};
    }
  }

  bool fetch(tl::TLBuffer& tlb) noexcept {
    switch (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO)) {
    case std::to_underlying(Version::V09): {
      version = Version::V09;
      break;
    }
    case std::to_underlying(Version::V10): {
      version = Version::V10;
      break;
    }
    case std::to_underlying(Version::V11): {
      version = Version::V11;
      break;
    }
    case std::to_underlying(Version::V2): {
      version = Version::V2;
      break;
    }
    case std::to_underlying(Version::V3): {
      version = Version::V3;
      break;
    }
    default: {
      version = Version::Invalid;
      break;
    }
    }

    return version != Version::Invalid;
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(std::to_underlying(version));
  }
};

class httpUri final {
  static constexpr auto SCHEME_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto HOST_FLAG = static_cast<uint32_t>(1U << 1U);
  static constexpr auto QUERY_FLAG = static_cast<uint32_t>(1U << 2U);

public:
  std::optional<tl::string> opt_scheme;
  std::optional<tl::string> opt_host;
  tl::string path;
  std::optional<tl::string> opt_query;

  bool fetch(tl::TLBuffer& tlb) noexcept {
    const auto opt_flags{tlb.fetch_trivial<uint32_t>()};
    bool ok{opt_flags.has_value()};

    const auto flags{opt_flags.value_or(0x0)};
    if (ok && static_cast<bool>(flags & SCHEME_FLAG)) [[likely]] {
      ok &= opt_scheme.emplace().fetch(tlb);
    }
    if (ok && static_cast<bool>(flags & HOST_FLAG)) {
      ok &= opt_host.emplace().fetch(tlb);
    }
    ok &= path.fetch(tlb);
    if (ok && static_cast<bool>(flags & QUERY_FLAG)) {
      ok &= opt_query.emplace().fetch(tlb);
    }
    return ok;
  }
};

struct httpHeaderEntry final {
  tl::Bool is_sensitive{};
  tl::string name;
  tl::string value;

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return is_sensitive.fetch(tlb) && name.fetch(tlb) && value.fetch(tlb);
  }

  void store(tl::TLBuffer& tlb) const noexcept {
    is_sensitive.store(tlb);
    name.store(tlb);
    value.store(tlb);
  }
};

struct httpConnection final {
  tl::string server_addr;
  tl::i32 server_port{};
  tl::string remote_addr;
  tl::i32 remote_port{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    bool ok{server_addr.fetch(tlb)};
    ok &= server_port.fetch(tlb);
    ok &= remote_addr.fetch(tlb);
    ok &= remote_port.fetch(tlb);
    return ok;
  }
};

struct httpResponse final {
  tl::HttpVersion version{};
  tl::i32 status_code{};
  tl::vector<tl::httpHeaderEntry> headers{};
  std::string_view body;

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(0x0); // flags
    version.store(tlb);
    status_code.store(tlb);
    headers.store(tlb);
    tlb.store_bytes(body);
  }
};

class HttpResponse final {
  static constexpr uint32_t MAGIC = 0x8466'24dc;

public:
  tl::httpResponse http_response{};

  void store(tl::TLBuffer& tlb) const noexcept {
    tlb.store_trivial<uint32_t>(MAGIC);
    http_response.store(tlb);
  }
};

// ===== RPC =====

class rpcInvokeReqExtra final {
  static constexpr auto RETURN_BINLOG_POS_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto RETURN_BINLOG_TIME_FLAG = static_cast<uint32_t>(1U << 1U);
  static constexpr auto RETURN_PID_FLAG = static_cast<uint32_t>(1U << 2U);
  static constexpr auto RETURN_REQUEST_SIZES_FLAG = static_cast<uint32_t>(1U << 3U);
  static constexpr auto RETURN_FAILED_SUBQUERIES_FLAG = static_cast<uint32_t>(1U << 4U);
  static constexpr auto RETURN_QUERY_STATS_FLAG = static_cast<uint32_t>(1U << 6U);
  static constexpr auto NORESULT_FLAG = static_cast<uint32_t>(1U << 7U);
  static constexpr auto WAIT_BINLOG_POS_FLAG = static_cast<uint32_t>(1U << 16U);
  static constexpr auto STRING_FORWARD_KEYS_FLAG = static_cast<uint32_t>(1U << 18U);
  static constexpr auto INT_FORWARD_KEYS_FLAG = static_cast<uint32_t>(1U << 19U);
  static constexpr auto STRING_FORWARD_FLAG = static_cast<uint32_t>(1U << 20U);
  static constexpr auto INT_FORWARD_FLAG = static_cast<uint32_t>(1U << 21U);
  static constexpr auto CUSTOM_TIMEOUT_MS_FLAG = static_cast<uint32_t>(1U << 23U);
  static constexpr auto SUPPORTED_COMPRESSION_VERSION_FLAG = static_cast<uint32_t>(1U << 25U);
  static constexpr auto RANDOM_DELAY_FLAG = static_cast<uint32_t>(1U << 26U);
  static constexpr auto RETURN_VIEW_NUMBER_FLAG = static_cast<uint32_t>(1U << 27U);

public:
  uint32_t flags{};
  bool return_binlog_pos{};
  bool return_binlog_time{};
  bool return_pid{};
  bool return_request_sizes{};
  bool return_failed_subqueries{};
  bool return_query_stats{};
  bool no_result{};
  std::optional<tl::i64> opt_wait_binlog_pos;
  std::optional<tl::vector<tl::string>> opt_string_forward_keys;
  std::optional<tl::vector<tl::i64>> opt_int_forward_keys;
  std::optional<tl::string> opt_string_forward;
  std::optional<tl::i64> opt_int_forward;
  std::optional<tl::i32> opt_custom_timeout_ms;
  std::optional<tl::i32> opt_supported_compression_version;
  std::optional<tl::f64> opt_random_delay;
  bool return_view_number{};

  bool fetch(tl::TLBuffer& tlb) noexcept;
};

struct RpcInvokeReqExtra final {
  tl::rpcInvokeReqExtra inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_RPC_INVOKE_REQ_EXTRA && inner.fetch(tlb);
  }
};

struct rpcDestActor final {
  tl::i64 actor_id{};
  std::string_view query;

  bool fetch(TLBuffer& tlb) noexcept {
    bool ok{actor_id.fetch(tlb)};
    const auto opt_query{tlb.fetch_bytes(tlb.remaining())};
    query = opt_query.value_or(std::string_view{});
    return ok && opt_query.has_value();
  }
};

struct RpcDestActor final {
  tl::rpcDestActor inner{};

  bool fetch(TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_RPC_DEST_ACTOR && inner.fetch(tlb);
  }
};

struct rpcDestFlags final {
  tl::rpcInvokeReqExtra extra{};
  std::string_view query;

  bool fetch(tl::TLBuffer& tlb) noexcept {
    bool ok{extra.fetch(tlb)};
    const auto opt_query{tlb.fetch_bytes(tlb.remaining())};
    query = opt_query.value_or(std::string_view{});
    return ok && opt_query.has_value();
  }
};

struct RpcDestFlags final {
  tl::rpcDestFlags inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_RPC_DEST_FLAGS && inner.fetch(tlb);
  }
};

struct rpcDestActorFlags final {
  tl::i64 actor_id{};
  tl::rpcInvokeReqExtra extra{};
  std::string_view query;

  bool fetch(tl::TLBuffer& tlb) noexcept {
    bool ok{actor_id.fetch(tlb)};
    std::ignore = tlb.fetch_trivial<uint32_t>(); // skip flags
    ok &= extra.fetch(tlb);
    const auto opt_query{tlb.fetch_bytes(tlb.remaining())};
    query = opt_query.value_or(std::string_view{});
    return ok && opt_query.has_value();
  }
};

struct RpcDestActorFlags final {
  tl::rpcDestActorFlags inner{};

  bool fetch(tl::TLBuffer& tlb) noexcept {
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_RPC_DEST_ACTOR_FLAGS && inner.fetch(tlb);
  }
};

} // namespace tl
