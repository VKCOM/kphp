// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include "common/tl/constants/common.h"
#include "runtime-core/allocator/runtime-allocator.h"
#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/runtime-core.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

// ===== GENERAL =====

struct Bool final {
  bool value{};

  bool fetch(TLBuffer &tlb) noexcept {
    const auto magic{tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO)};
    value = magic == TL_BOOL_TRUE;
    return magic == TL_BOOL_TRUE || magic == TL_BOOL_FALSE;
  }

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_trivial<uint32_t>(value ? TL_BOOL_TRUE : TL_BOOL_FALSE);
  }
};

template<typename T>
struct Maybe final {
  std::optional<T> opt_value{};

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
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

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    if (opt_value.has_value()) {
      tlb.store_trivial<uint32_t>(TL_MAYBE_TRUE);
      (*opt_value).store(tlb);
    } else {
      tlb.store_trivial<uint32_t>(TL_MAYBE_FALSE);
    }
  }
};

template<typename T>
struct vector final {
  using vector_t = memory_resource::stl::vector<T, memory_resource::unsynchronized_pool_resource>;
  vector_t data{typename vector_t::allocator_type(RuntimeAllocator::current().memory_resource)};

  using iterator = vector_t::iterator;
  using const_iterator = vector_t::const_iterator;

  iterator begin() noexcept {
    return data.begin();
  }
  iterator end() noexcept {
    return data.end();
  }
  const_iterator begin() const noexcept {
    return data.begin();
  }
  const_iterator end() const noexcept {
    return data.end();
  }
  const_iterator cbegin() const noexcept {
    return data.cbegin();
  }
  const_iterator cend() const noexcept {
    return data.cend();
  }

  size_t size() const noexcept {
    return data.size();
  }

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
    int64_t size{};
    if (const auto opt_size{tlb.fetch_trivial<uint32_t>()}; opt_size.has_value()) {
      size = *opt_size;
    } else {
      return false;
    }

    data.clear();
    data.reserve(static_cast<size_t>(size));
    for (auto i = 0; i < size; ++i) {
      T t{};
      if (!t.fetch(tlb)) {
        return false;
      }
      data.emplace_back(std::move(t));
    }

    return true;
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_trivial<uint32_t>(static_cast<uint32_t>(data.size()));
    std::for_each(data.cbegin(), data.cend(), [&tlb](auto &&elem) { std::forward<decltype(elem)>(elem).store(tlb); });
  }
};

template<typename T>
struct Vector final {
  vector<T> data{};

  using iterator = vector<T>::iterator;
  using const_iterator = vector<T>::const_iterator;

  iterator begin() noexcept {
    return data.begin();
  }
  iterator end() noexcept {
    return data.end();
  }
  const_iterator begin() const noexcept {
    return data.begin();
  }
  const_iterator end() const noexcept {
    return data.end();
  }
  const_iterator cbegin() const noexcept {
    return data.cbegin();
  }
  const_iterator cend() const noexcept {
    return data.cend();
  }

  size_t size() const noexcept {
    return data.size();
  }

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
    if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != TL_VECTOR) {
      return false;
    }
    return data.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_trivial<uint32_t>(TL_VECTOR);
    data.store(tlb);
  }
};

template<typename T>
struct dictionaryField final {
  string key;
  T value{};

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
    const auto key_view{tlb.fetch_string()};
    key = {key_view.data(), static_cast<string::size_type>(key_view.size())};
    return value.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_string({key.c_str(), static_cast<size_t>(key.size())});
    value.store(tlb);
  }
};

template<typename T>
struct dictionary final {
  vector<dictionaryField<T>> data{};

  using iterator = vector<dictionaryField<T>>::iterator;
  using const_iterator = vector<dictionaryField<T>>::const_iterator;

  iterator begin() noexcept {
    return data.begin();
  }
  iterator end() noexcept {
    return data.end();
  }
  const_iterator begin() const noexcept {
    return data.begin();
  }
  const_iterator end() const noexcept {
    return data.end();
  }
  const_iterator cbegin() const noexcept {
    return data.cbegin();
  }
  const_iterator cend() const noexcept {
    return data.cend();
  }

  size_t size() const noexcept {
    return data.size();
  }

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
    return data.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    data.store(tlb);
  }
};

template<typename T>
struct Dictionary final {
  dictionary<T> data{};

  using iterator = dictionary<T>::iterator;
  using const_iterator = dictionary<T>::const_iterator;

  iterator begin() noexcept {
    return data.begin();
  }
  iterator end() noexcept {
    return data.end();
  }
  const_iterator begin() const noexcept {
    return data.begin();
  }
  const_iterator end() const noexcept {
    return data.end();
  }
  const_iterator cbegin() const noexcept {
    return data.cbegin();
  }
  const_iterator cend() const noexcept {
    return data.cend();
  }

  size_t size() const noexcept {
    return data.size();
  }

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
    if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != TL_DICTIONARY) {
      return false;
    }
    return data.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_trivial<uint32_t>(TL_DICTIONARY);
    data.store(tlb);
  }
};

// ===== JOB WORKERS =====

class K2JobWorkerResponse final {
  static constexpr uint32_t MAGIC = 0x3afb'3a08;

public:
  int64_t job_id{};
  string body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== CRYPTO =====

// Actually it's "Maybe (Dictionary CertInfoItem)"
// But I now want to have this logic separately
class GetPemCertInfoResponse final {
  enum CertInfoItem : uint32_t { LONG_MAGIC = 0x533f'f89f, STR_MAGIC = 0xc427'feef, DICT_MAGIC = 0x1ea8'a774 };

public:
  array<mixed> data;

  bool fetch(TLBuffer &tlb) noexcept;
};

enum DigestAlgorithm : uint32_t {
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

// ===== CONFDATA =====

struct confdataValue final {
  string value;
  bool is_php_serialized{};
  bool is_json_serialized{};

  bool fetch(TLBuffer &tlb) noexcept;
};

// ===== HTTP =====

struct HttpVersion final {
  enum class Version : uint32_t {
    Invalid = TL_ZERO,
    V09 = 0x9e15'd325,
    V10 = 0xfed2'5ac0,
    V11 = 0x89d5'6a56,
    V2 = 0x9022'9734,
    V3 = 0xe725'a7a2,
  };

  Version version{Version::Invalid};

  bool fetch(TLBuffer &tlb) noexcept {
    using version_utype = std::underlying_type_t<Version>;

    switch (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO)) {
      case static_cast<version_utype>(Version::V09): {
        version = Version::V09;
        break;
      }
      case static_cast<version_utype>(Version::V10): {
        version = Version::V10;
        break;
      }
      case static_cast<version_utype>(Version::V11): {
        version = Version::V11;
        break;
      }
      case static_cast<version_utype>(Version::V2): {
        version = Version::V2;
        break;
      }
      case static_cast<version_utype>(Version::V3): {
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

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_trivial<uint32_t>(static_cast<std::underlying_type_t<Version>>(version));
  }
};

class httpUri final {
  static constexpr auto SCHEME_FLAG = static_cast<uint32_t>(1U << 0U);
  static constexpr auto HOST_FLAG = static_cast<uint32_t>(1U << 1U);
  static constexpr auto QUERY_FLAG = static_cast<uint32_t>(1U << 2U);

public:
  std::optional<string> opt_scheme;
  std::optional<string> opt_host;
  string path;
  std::optional<string> opt_query;

  bool fetch(TLBuffer &tlb) noexcept {
    const auto opt_flags{tlb.fetch_trivial<uint32_t>()};
    if (!opt_flags.has_value()) {
      return false;
    }

    const auto flags{*opt_flags};
    if (static_cast<bool>(flags & SCHEME_FLAG)) {
      const auto scheme_view{tlb.fetch_string()};
      opt_scheme.emplace(scheme_view.data(), static_cast<string::size_type>(scheme_view.size()));
    }
    if (static_cast<bool>(flags & HOST_FLAG)) {
      const auto host_view{tlb.fetch_string()};
      opt_host.emplace(host_view.data(), static_cast<string::size_type>(host_view.size()));
    }
    const auto path_view{tlb.fetch_string()};
    path = {path_view.data(), static_cast<string::size_type>(path_view.size())};
    if (static_cast<bool>(flags & QUERY_FLAG)) {
      const auto query_view{tlb.fetch_string()};
      opt_query.emplace(query_view.data(), static_cast<string::size_type>(query_view.size()));
    }
    return true;
  }
};

struct httpHeaderValue final {
  Bool is_sensitive{};
  string value;

  bool fetch(TLBuffer &tlb) noexcept {
    const auto ok{is_sensitive.fetch(tlb)};
    const auto value_view{tlb.fetch_string()};
    value = {value_view.data(), static_cast<string::size_type>(value_view.size())};
    return ok;
  }

  void store(TLBuffer &tlb) const noexcept {
    is_sensitive.store(tlb);
    tlb.store_string({value.c_str(), static_cast<size_t>(value.size())});
  }
};

struct httpConnection final {
  string server_addr;
  uint32_t server_port{};
  string remote_addr;
  uint32_t remote_port{};

  bool fetch(TLBuffer &tlb) noexcept {
    const auto server_addr_view{tlb.fetch_string()};
    server_addr = {server_addr_view.data(), static_cast<string::size_type>(server_addr_view.size())};
    const auto opt_server_port{tlb.fetch_trivial<uint32_t>()};
    const auto remote_addr_view{tlb.fetch_string()};
    remote_addr = {remote_addr_view.data(), static_cast<string::size_type>(remote_addr_view.size())};
    const auto opt_remote_port{tlb.fetch_trivial<uint32_t>()};

    if (!opt_server_port.has_value() || !opt_remote_port.has_value()) {
      return false;
    }

    server_port = *opt_server_port;
    remote_port = *opt_remote_port;
    return true;
  }
};

struct httpResponse final {
  HttpVersion version{};
  int32_t status_code{};
  dictionary<httpHeaderValue> headers{};
  string body;

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_trivial<uint32_t>(0x0); // flags
    version.store(tlb);
    tlb.store_trivial<int32_t>(status_code);
    headers.store(tlb);
    tlb.store_bytes({body.c_str(), static_cast<size_t>(body.size())});
  }
};

} // namespace tl
