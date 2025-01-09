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
#include <variant>

#include "common/tl/constants/common.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/core/std/containers.h"
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
  using vector_t = kphp::stl::vector<T, kphp::memory::script_allocator>;
  vector_t data;

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
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_VECTOR && data.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_trivial<uint32_t>(TL_VECTOR);
    data.store(tlb);
  }
};

template<typename T>
struct dictionaryField final {
  std::string_view key;
  T value{};

  bool fetch(TLBuffer &tlb) noexcept requires tl_deserializable<T> {
    key = tlb.fetch_string();
    return value.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_string(key);
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
    return tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) == TL_DICTIONARY && data.fetch(tlb);
  }

  void store(TLBuffer &tlb) const noexcept requires tl_serializable<T> {
    tlb.store_trivial<uint32_t>(TL_DICTIONARY);
    data.store(tlb);
  }
};

struct string final {
  std::string_view value;

  bool fetch(TLBuffer &tlb) noexcept {
    value = tlb.fetch_string();
    return true;
  }

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_string(value);
  }
};

struct String final {
  std::string_view value;

  bool fetch(TLBuffer &tlb) noexcept {
    if (tlb.fetch_trivial<uint32_t>().value_or(TL_ZERO) != TL_STRING) {
      return false;
    }
    value = tlb.fetch_string();
    return true;
  }

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_trivial<uint32_t>(TL_STRING);
    tlb.store_string(value);
  }
};

// ===== JOB WORKERS =====

class K2JobWorkerResponse final {
  static constexpr uint32_t MAGIC = 0x3afb'3a08;

public:
  int64_t job_id{};
  std::string_view body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== CRYPTO =====

class CertInfoItem final {
  enum Magic : uint32_t { LONG = 0x533f'f89f, STR = 0xc427'feef, DICT = 0x1ea8'a774 };
public:
  std::variant<int64_t, tl::string, dictionary<tl::string>> data;
  
  bool fetch(TLBuffer &tlb) noexcept;

  template<class... Ts>
  struct MakeVisitor : Ts... { using Ts::operator()...; };
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

enum CipherAlgorithm : uint32_t {
  AES128 = 0xe627'c460,
  AES256 = 0x4c98'c1f9,
};

enum BlockPadding : uint32_t {
  PKCS7 = 0x699e'c5de,
};

// ===== CONFDATA =====

struct confdataValue final {
  std::string_view value;
  bool is_php_serialized{};
  bool is_json_serialized{};

  bool fetch(TLBuffer &tlb) noexcept;
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
  std::optional<::string> opt_scheme;
  std::optional<::string> opt_host;
  ::string path;
  std::optional<::string> opt_query;

  bool fetch(TLBuffer &tlb) noexcept {
    const auto opt_flags{tlb.fetch_trivial<uint32_t>()};
    if (!opt_flags.has_value()) {
      return false;
    }

    const auto flags{*opt_flags};
    if (static_cast<bool>(flags & SCHEME_FLAG)) {
      const auto scheme_view{tlb.fetch_string()};
      opt_scheme.emplace(scheme_view.data(), static_cast<::string::size_type>(scheme_view.size()));
    }
    if (static_cast<bool>(flags & HOST_FLAG)) {
      const auto host_view{tlb.fetch_string()};
      opt_host.emplace(host_view.data(), static_cast<::string::size_type>(host_view.size()));
    }
    const auto path_view{tlb.fetch_string()};
    path = {path_view.data(), static_cast<::string::size_type>(path_view.size())};
    if (static_cast<bool>(flags & QUERY_FLAG)) {
      const auto query_view{tlb.fetch_string()};
      opt_query.emplace(query_view.data(), static_cast<::string::size_type>(query_view.size()));
    }
    return true;
  }
};

struct httpHeaderEntry final {
  Bool is_sensitive{};
  ::string name;
  ::string value;

  bool fetch(TLBuffer &tlb) noexcept {
    const auto ok{is_sensitive.fetch(tlb)};
    const auto name_view{tlb.fetch_string()};
    const auto value_view{tlb.fetch_string()};
    name = {name_view.data(), static_cast<::string::size_type>(name_view.size())};
    value = {value_view.data(), static_cast<::string::size_type>(value_view.size())};
    return ok;
  }

  void store(TLBuffer &tlb) const noexcept {
    is_sensitive.store(tlb);
    tlb.store_string({name.c_str(), static_cast<size_t>(name.size())});
    tlb.store_string({value.c_str(), static_cast<size_t>(value.size())});
  }
};

struct httpConnection final {
  ::string server_addr;
  uint32_t server_port{};
  ::string remote_addr;
  uint32_t remote_port{};

  bool fetch(TLBuffer &tlb) noexcept {
    const auto server_addr_view{tlb.fetch_string()};
    server_addr = {server_addr_view.data(), static_cast<::string::size_type>(server_addr_view.size())};
    const auto opt_server_port{tlb.fetch_trivial<uint32_t>()};
    const auto remote_addr_view{tlb.fetch_string()};
    remote_addr = {remote_addr_view.data(), static_cast<::string::size_type>(remote_addr_view.size())};
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
  vector<httpHeaderEntry> headers{};
  ::string body;

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_trivial<uint32_t>(0x0); // flags
    version.store(tlb);
    tlb.store_trivial<int32_t>(status_code);
    headers.store(tlb);
    tlb.store_bytes({body.c_str(), static_cast<size_t>(body.size())});
  }
};

class HttpResponse final {
  static constexpr uint32_t MAGIC = 0x8466'24dc;

public:
  httpResponse http_response{};

  void store(TLBuffer &tlb) const noexcept {
    tlb.store_trivial<uint32_t>(MAGIC);
    http_response.store(tlb);
  }
};

} // namespace tl
