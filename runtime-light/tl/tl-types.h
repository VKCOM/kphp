// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "common/tl/constants/common.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/tl/tl-core.h"

namespace tl {

struct magic final {
  using underlying_type = uint32_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  bool expect(underlying_type expected) const noexcept {
    return expected == value;
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct mask final {
  using underlying_type = uint32_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct Bool final {
  bool value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && (magic.expect(TL_BOOL_TRUE) || magic.expect(TL_BOOL_FALSE))};
    value = magic.expect(TL_BOOL_TRUE);
    return ok;
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = value ? TL_BOOL_TRUE : TL_BOOL_FALSE}.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = value ? TL_BOOL_TRUE : TL_BOOL_FALSE}.footprint();
  }
};

struct u8 final {
  using underlying_type = uint8_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct i32 final {
  using underlying_type = int32_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct I32 final {
  tl::i32 inner{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_INT) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = TL_INT}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = TL_INT}.footprint() + inner.footprint();
  }
};

struct u32 final {
  using underlying_type = uint32_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct i64 final {
  using underlying_type = int64_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct I64 final {
  tl::i64 inner{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_LONG) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = TL_LONG}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = TL_LONG}.footprint() + inner.footprint();
  }
};

struct u64 final {
  using underlying_type = uint64_t;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct f32 final {
  using underlying_type = float;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0.0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct F32 final {
  tl::f32 inner{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_FLOAT) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = TL_FLOAT}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = TL_FLOAT}.footprint() + inner.footprint();
  }
};

struct f64 final {
  using underlying_type = double;
  underlying_type value{};

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto opt_value{tlf.fetch_trivial<underlying_type>()};
    value = opt_value.value_or(0.0);
    return opt_value.has_value();
  }

  void store(tl::storer& tls) const noexcept {
    tls.store_trivial<underlying_type>(value);
  }

  constexpr size_t footprint() const noexcept {
    return sizeof(underlying_type);
  }
};

struct F64 final {
  tl::f64 inner{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_DOUBLE) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = TL_DOUBLE}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = TL_DOUBLE}.footprint() + inner.footprint();
  }
};

template<typename T>
struct Maybe final {
  std::optional<T> opt_value{};

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T>
  {
    tl::magic magic{};
    if (!magic.fetch(tlf) || (!magic.expect(TL_MAYBE_TRUE) && !magic.expect(TL_MAYBE_FALSE))) [[unlikely]] {
      return false;
    }
    return magic.expect(TL_MAYBE_TRUE) ? opt_value.emplace().fetch(tlf) : (opt_value = std::nullopt, true);
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T>
  {
    opt_value ? tl::magic{.value = TL_MAYBE_TRUE}.store(tls), (*opt_value).store(tls) : tl::magic{.value = TL_MAYBE_FALSE}.store(tls);
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T>
  {
    return opt_value ? tl::magic{.value = TL_MAYBE_TRUE}.footprint() + (*opt_value).footprint() : tl::magic{.value = TL_MAYBE_FALSE}.footprint();
  }
};

template<typename T, typename U>
struct Either final {
  std::variant<T, U> value;

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T> && tl::deserializable<U>
  {
    T t{};
    U u{};
    const auto initial_pos{tlf.pos()};
    if (t.fetch(tlf)) {
      value.template emplace<T>(std::move(t));
      return true;
    } else if (tlf.reset(initial_pos); u.fetch(tlf)) {
      value.template emplace<U>(std::move(u));
      return true;
    }
    return false;
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T> && tl::serializable<U>
  {
    if (std::holds_alternative<T>(value)) {
      static_cast<T>(value).store(tls);
    } else {
      static_cast<U>(value).store(tls);
    }
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T> && tl::footprintable<U>
  {
    return std::visit([](const auto& v) noexcept { return v.footprint(); }, value);
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

  bool fetch(tl::fetcher& tlf) noexcept;

  void store(tl::storer& tls) const noexcept;

  constexpr size_t footprint() const noexcept {
    size_t str_len{value.size()};
    size_t size_len{};
    if (str_len <= SMALL_STRING_MAX_LEN) {
      size_len = SMALL_STRING_SIZE_LEN;
    } else if (str_len <= MEDIUM_STRING_MAX_LEN) {
      size_len = MEDIUM_STRING_SIZE_LEN + 1;
    } else {
      size_len = LARGE_STRING_SIZE_LEN + 1;
    }

    const auto total_len{size_len + str_len};
    const auto total_len_with_padding{(total_len + 3) & ~3};
    return total_len_with_padding;
  }
};

struct String final {
  tl::string inner;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_STRING) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = TL_STRING}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = TL_STRING}.footprint() + inner.footprint();
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

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T>
  {
    tl::u32 size{};
    if (!size.fetch(tlf)) [[unlikely]] {
      return false;
    }

    value.clear();
    value.reserve(static_cast<size_t>(size.value));
    for (auto i = 0; i < size.value; ++i) {
      if (T t{}; t.fetch(tlf)) [[likely]] {
        value.emplace_back(std::move(t));
        continue;
      }
      return false;
    }

    return true;
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T>
  {
    tl::u32{.value = static_cast<uint32_t>(value.size())}.store(tls);
    std::for_each(value.cbegin(), value.cend(), [&tls](const auto& elem) noexcept { elem.store(tls); });
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T>
  {
    auto footprint_view{value | std::views::transform([](const auto& elem) noexcept { return elem.footprint(); })};
    return std::accumulate(footprint_view.begin(), footprint_view.end(), tl::u32{.value = static_cast<uint32_t>(value.size())}.footprint(), std::plus<>{});
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

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T>
  {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_VECTOR) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T>
  {
    tl::magic{.value = TL_VECTOR}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T>
  {
    return tl::magic{.value = TL_VECTOR}.footprint() + inner.footprint();
  }
};

template<typename T>
struct dictionaryField final {
  tl::string key;
  T value{};

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T>
  {
    return key.fetch(tlf) && value.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T>
  {
    key.store(tls);
    value.store(tls);
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T>
  {
    return key.footprint() + value.footprint();
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

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T>
  {
    return value.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T>
  {
    value.store(tls);
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T>
  {
    return value.footprint();
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

  bool fetch(tl::fetcher& tlf) noexcept
  requires tl::deserializable<T>
  {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_DICTIONARY) && inner.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept
  requires tl::serializable<T>
  {
    tl::magic{.value = TL_DICTIONARY}.store(tls);
    inner.store(tls);
  }

  constexpr size_t footprint() const noexcept
  requires tl::footprintable<T>
  {
    return tl::magic{.value = TL_DICTIONARY}.footprint() + inner.footprint();
  }
};

class netPid final {
  static constexpr uint32_t PORT_MASK = 0x0000'ffff;
  static constexpr uint32_t PID_MASK = 0xffff'0000;

public:
  tl::u32 ip{};
  tl::u32 port_pid{};
  tl::u32 utime{};

  uint32_t get_ip() const noexcept {
    return ip.value;
  }

  uint16_t get_port() const noexcept {
    return static_cast<uint16_t>(port_pid.value & PORT_MASK);
  }

  uint16_t get_pid() const noexcept {
    return static_cast<uint16_t>((port_pid.value & PID_MASK) >> 16);
  }

  uint32_t get_utime() const noexcept {
    return utime.value;
  }

  bool fetch(tl::fetcher& tlf) noexcept {
    return ip.fetch(tlf) && port_pid.fetch(tlf) && utime.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    ip.store(tls), port_pid.store(tls), utime.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return ip.footprint() + port_pid.footprint() + utime.footprint();
  }
};

// ===== JOB WORKERS =====

class K2JobWorkerResponse final {
  static constexpr uint32_t MAGIC = 0x3afb'3a08;

public:
  tl::i64 job_id{};
  tl::string body;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(MAGIC)};
    ok &= tl::mask{}.fetch(tlf);
    ok &= job_id.fetch(tlf);
    ok &= body.fetch(tlf);
    return ok;
  }

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = MAGIC}.store(tls);
    tl::mask{}.store(tls);
    job_id.store(tls);
    body.store(tls);
  }
};

// ===== CRYPTO =====

class CertInfoItem final {
  enum Magic : uint32_t { LONG = 0x533f'f89f, STR = 0xc427'feef, DICT = 0x1ea8'a774 };

public:
  std::variant<tl::i64, tl::string, tl::dictionary<tl::string>> data;

  bool fetch(tl::fetcher& tlf) noexcept;

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

  bool fetch(tl::fetcher& tlf) noexcept {
    return value.fetch(tlf) && is_php_serialized.fetch(tlf) && is_json_serialized.fetch(tlf);
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

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    if (!magic.fetch(tlf)) [[unlikely]] {
      return false;
    }

    switch (magic.value) {
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

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = std::to_underlying(version)}.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = std::to_underlying(version)}.footprint();
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

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::mask flags{};
    bool ok{flags.fetch(tlf)};

    if (ok && static_cast<bool>(flags.value & SCHEME_FLAG)) [[likely]] {
      ok &= opt_scheme.emplace().fetch(tlf);
    }
    if (ok && static_cast<bool>(flags.value & HOST_FLAG)) {
      ok &= opt_host.emplace().fetch(tlf);
    }
    ok &= path.fetch(tlf);
    if (ok && static_cast<bool>(flags.value & QUERY_FLAG)) {
      ok &= opt_query.emplace().fetch(tlf);
    }
    return ok;
  }
};

struct httpHeaderEntry final {
  tl::Bool is_sensitive{};
  tl::string name;
  tl::string value;

  bool fetch(tl::fetcher& tlf) noexcept {
    return is_sensitive.fetch(tlf) && name.fetch(tlf) && value.fetch(tlf);
  }

  void store(tl::storer& tls) const noexcept {
    is_sensitive.store(tls);
    name.store(tls);
    value.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return is_sensitive.footprint() + name.footprint() + value.footprint();
  }
};

struct httpConnection final {
  tl::string server_addr;
  tl::i32 server_port{};
  tl::string remote_addr;
  tl::i32 remote_port{};

  bool fetch(tl::fetcher& tlf) noexcept {
    bool ok{server_addr.fetch(tlf)};
    ok &= server_port.fetch(tlf);
    ok &= remote_addr.fetch(tlf);
    ok &= remote_port.fetch(tlf);
    return ok;
  }
};

struct httpResponse final {
  tl::HttpVersion version{};
  tl::i32 status_code{};
  tl::vector<tl::httpHeaderEntry> headers{};
  std::span<const std::byte> body;

  void store(tl::storer& tls) const noexcept {
    tl::mask{}.store(tls);
    version.store(tls);
    status_code.store(tls);
    headers.store(tls);
    tls.store_bytes(body);
  }

  constexpr size_t footprint() const noexcept {
    return tl::mask{}.footprint() + version.footprint() + status_code.footprint() + headers.footprint() + body.size();
  }
};

class HttpResponse final {
  static constexpr uint32_t MAGIC = 0x8466'24dc;

public:
  tl::httpResponse http_response{};

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = MAGIC}.store(tls);
    http_response.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = MAGIC}.footprint() + http_response.footprint();
  }
};

// ===== RPC =====

class rpcInvokeReqExtra final {
  static constexpr uint32_t RETURN_BINLOG_POS_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_binlog_pos;
  static constexpr uint32_t RETURN_BINLOG_TIME_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_binlog_time;
  static constexpr uint32_t RETURN_PID_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_pid;
  static constexpr uint32_t RETURN_REQUEST_SIZES_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_request_sizes;
  static constexpr uint32_t RETURN_FAILED_SUBQUERIES_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_failed_subqueries;
  static constexpr uint32_t RETURN_QUERY_STATS_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_query_stats;
  static constexpr uint32_t NORESULT_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::no_result;
  static constexpr uint32_t WAIT_BINLOG_POS_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::wait_binlog_pos;
  static constexpr uint32_t STRING_FORWARD_KEYS_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::string_forward_keys;
  static constexpr uint32_t INT_FORWARD_KEYS_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::int_forward_keys;
  static constexpr uint32_t STRING_FORWARD_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::string_forward;
  static constexpr uint32_t INT_FORWARD_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::int_forward;
  static constexpr uint32_t CUSTOM_TIMEOUT_MS_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::custom_timeout_ms;
  static constexpr uint32_t SUPPORTED_COMPRESSION_VERSION_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::supported_compression_version;
  static constexpr uint32_t RANDOM_DELAY_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::random_delay;
  static constexpr uint32_t RETURN_VIEW_NUMBER_FLAG = vk::tl::common::rpc_invoke_req_extra_flags::return_view_number;

public:
  tl::mask flags{};
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

  bool fetch(tl::fetcher& tlf) noexcept;
};

struct RpcInvokeReqExtra final {
  tl::rpcInvokeReqExtra inner{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(TL_RPC_INVOKE_REQ_EXTRA) && inner.fetch(tlf);
  }
};

class rpcReqResultExtra final {
  static constexpr uint32_t BINLOG_POS_FLAG = vk::tl::common::rpc_req_result_extra_flags::binlog_pos;
  static constexpr uint32_t BINLOG_TIME_FLAG = vk::tl::common::rpc_req_result_extra_flags::binlog_time;
  static constexpr uint32_t ENGINE_PID_FLAG = vk::tl::common::rpc_req_result_extra_flags::engine_pid;
  static constexpr uint32_t REQUEST_SIZE_FLAG = vk::tl::common::rpc_req_result_extra_flags::request_size;
  static constexpr uint32_t RESPONSE_SIZE_FLAG = vk::tl::common::rpc_req_result_extra_flags::response_size;
  static constexpr uint32_t FAILED_SUBQUERIES_FLAG = vk::tl::common::rpc_req_result_extra_flags::failed_subqueries;
  static constexpr uint32_t COMPRESSION_VERSION_FLAG = vk::tl::common::rpc_req_result_extra_flags::compression_version;
  static constexpr uint32_t STATS_FLAG = vk::tl::common::rpc_req_result_extra_flags::stats;
  static constexpr uint32_t EPOCH_NUMBER_FLAG = vk::tl::common::rpc_req_result_extra_flags::epoch_number;
  static constexpr uint32_t VIEW_NUMBER_FLAG = vk::tl::common::rpc_req_result_extra_flags::view_number;

public:
  tl::mask flags{};
  tl::i64 binlog_pos{};
  tl::i64 binlog_time{};
  tl::netPid engine_pid{};
  tl::i32 request_size{};
  tl::i32 response_size{};
  tl::i32 failed_subqueries{};
  tl::i32 compression_version{};
  tl::dictionary<tl::string> stats{};
  tl::i64 epoch_number{};
  tl::i64 view_number{};

  void store(tl::storer& tls) const noexcept;

  size_t footprint() const noexcept;
};

struct RpcReqResultExtra final {
  tl::rpcReqResultExtra inner{};

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = TL_RPC_REQ_RESULT_EXTRA}.store(tls), inner.store(tls);
  }
};

struct k2RpcResponseError final {
  tl::i32 error_code{};
  tl::string error{};

  void store(tl::storer& tls) const noexcept {
    error_code.store(tls), error.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return error_code.footprint() + error.footprint();
  }
};

struct k2RpcResponseHeader final {
  tl::mask flags{};
  tl::rpcReqResultExtra extra{};
  std::span<const std::byte> result;

  void store(tl::storer& tls) const noexcept {
    flags.store(tls), extra.store(tls), tls.store_bytes(result);
  }

  constexpr size_t footprint() const noexcept {
    return flags.footprint() + extra.footprint() + result.size();
  }
};

class K2RpcResponse final {
  static constexpr uint32_t K2_RPC_RESPONSE_ERROR_MAGIC = 0xa08a'331b;
  static constexpr uint32_t K2_RPC_RESPONSE_HEADER_MAGIC = 0x70e6'2ff0;

public:
  std::variant<tl::k2RpcResponseError, tl::k2RpcResponseHeader> value;

  void store(tl::storer& tls) const noexcept {
    std::visit(
        [&tls](const auto& value) noexcept {
          using value_t = std::remove_cvref_t<decltype(value)>;
          if constexpr (std::same_as<value_t, tl::k2RpcResponseError>) {
            tl::magic{.value = K2_RPC_RESPONSE_ERROR_MAGIC}.store(tls);
          } else if constexpr (std::same_as<value_t, tl::k2RpcResponseHeader>) {
            tl::magic{.value = K2_RPC_RESPONSE_HEADER_MAGIC}.store(tls);
          } else {
            static_assert(false, "non-exhaustive visitor!");
          }
          value.store(tls);
        },
        value);
  }

  constexpr size_t footprint() const noexcept {
    return std::visit(
        [](const auto& value) noexcept {
          using value_t = std::remove_cvref_t<decltype(value)>;
          if constexpr (std::same_as<value_t, tl::k2RpcResponseError>) {
            return tl::magic{.value = K2_RPC_RESPONSE_ERROR_MAGIC}.footprint() + value.footprint();
          } else if constexpr (std::same_as<value_t, tl::k2RpcResponseHeader>) {
            return tl::magic{.value = K2_RPC_RESPONSE_HEADER_MAGIC}.footprint() + value.footprint();
          } else {
            static_assert(false, "non-exhaustive visitor!");
          }
        },
        value);
  }
};

// ===== INTER COMPONENT SESSION PROTOCOL =====

class InterComponentSessionRequestHeader final {
  static constexpr uint32_t INTER_COMPONENT_SESSION_REQUEST_HEADER_MAGIC = 0x24A3'16FF;

public:
  tl::u64 size;

  void store(tl::storer& tls) const noexcept {
    tl::magic{.value = INTER_COMPONENT_SESSION_REQUEST_HEADER_MAGIC}.store(tls);
    size.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = INTER_COMPONENT_SESSION_REQUEST_HEADER_MAGIC}.footprint() + size.footprint();
  }
};

class InterComponentSessionResponseHeader final {
  static constexpr uint32_t INTER_COMPONENT_SESSION_RESPONSE_HEADER_MAGIC = 0x24A3'16EE;

public:
  tl::u64 id;
  tl::u64 size;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(INTER_COMPONENT_SESSION_RESPONSE_HEADER_MAGIC) && id.fetch(tlf) && size.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = INTER_COMPONENT_SESSION_RESPONSE_HEADER_MAGIC}.footprint() + id.footprint() + size.footprint();
  }
};

// ===== WEB TRANSFER LIB =====

class WebError final {
  static constexpr uint32_t WEB_ERROR_MAGIC = 0x99A3'16EE;

public:
  tl::i64 code;
  tl::string description;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(WEB_ERROR_MAGIC) && code.fetch(tlf) && description.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = WEB_ERROR_MAGIC}.footprint() + code.footprint() + description.footprint();
  }
};

struct webPropertyValue final {
  using value_type = std::variant<tl::Bool, tl::I64, tl::F64, tl::String, tl::Vector<tl::webPropertyValue>, tl::Dictionary<tl::webPropertyValue>>;
  value_type value;

  void store(tl::storer& tls) const noexcept {
    std::visit([&tls](const auto& v) noexcept { v.store(tls); }, value);
  }

  bool fetch(tl::fetcher& tlf) noexcept {
    const auto initial_pos{tlf.pos()};
    if (tl::Bool b{}; b.fetch(tlf)) {
      value.template emplace<tl::Bool>(b);
      return true;
    }
    tlf.reset(initial_pos);
    if (tl::I64 i{}; i.fetch(tlf)) {
      value.template emplace<tl::I64>(i);
      return true;
    }
    tlf.reset(initial_pos);
    if (tl::F64 i{}; i.fetch(tlf)) {
      value.template emplace<tl::F64>(i);
      return true;
    }
    tlf.reset(initial_pos);
    if (tl::String s{}; s.fetch(tlf)) {
      value.template emplace<tl::String>(s);
      return true;
    }
    tlf.reset(initial_pos);
    if (tl::Vector<tl::webPropertyValue> v{}; v.fetch(tlf)) {
      value.template emplace<tl::Vector<tl::webPropertyValue>>(std::move(v));
      return true;
    }
    tlf.reset(initial_pos);
    if (tl::Dictionary<tl::webPropertyValue> d{}; d.fetch(tlf)) {
      value.template emplace<tl::Dictionary<tl::webPropertyValue>>(std::move(d));
      return true;
    }
    return false;
  }

  constexpr size_t footprint() const noexcept {
    return std::visit([](const auto& v) noexcept { return v.footprint(); }, value);
  }
};

struct webProperty final {
  tl::u64 id;
  tl::webPropertyValue value;

  void store(tl::storer& tls) const noexcept {
    id.store(tls);
    value.store(tls);
  }

  bool fetch(tl::fetcher& tlf) noexcept {
    return id.fetch(tlf) && value.fetch(tlf);
  }

  constexpr size_t footprint() const noexcept {
    return id.footprint() + value.footprint();
  }
};

class WebTransferGetPropertiesResultOk final {
  static constexpr uint32_t WEB_TRANSFER_GET_PROPERTIES_RESULT_OK_MAGIC = 0x48A7'16CC;

public:
  tl::vector<tl::webProperty> properties;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(WEB_TRANSFER_GET_PROPERTIES_RESULT_OK_MAGIC) && properties.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = WEB_TRANSFER_GET_PROPERTIES_RESULT_OK_MAGIC}.footprint();
  }
};

// ===== Simple =====
struct simpleWebTransferConfig final {
  tl::vector<tl::webProperty> properties;

  void store(tl::storer& tls) const noexcept {
    properties.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return properties.footprint();
  }
};

class SimpleWebTransferOpenResultOk final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_OPEN_RESULT_OK_MAGIC = 0x24A8'98FF;

public:
  tl::u64 descriptor;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(SIMPLE_WEB_TRANSFER_OPEN_RESULT_OK_MAGIC) && descriptor.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_OPEN_RESULT_OK_MAGIC}.footprint() + descriptor.footprint();
  }
};

class SimpleWebTransferPerformResultOk final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_PERFORM_RESULT_OK_MAGIC = 0x77A8'98FF;

public:
  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(SIMPLE_WEB_TRANSFER_PERFORM_RESULT_OK_MAGIC)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_PERFORM_RESULT_OK_MAGIC}.footprint();
  }
};

class SimpleWebTransferCloseResultOk final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_CLOSE_RESULT_OK_MAGIC = 0x63A7'16FF;

public:
  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(SIMPLE_WEB_TRANSFER_CLOSE_RESULT_OK_MAGIC)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_CLOSE_RESULT_OK_MAGIC}.footprint();
  }
};

class SimpleWebTransferResetResultOk final {
  static constexpr uint32_t SIMPLE_WEB_TRANSFER_RESET_RESULT_OK_MAGIC = 0x36C8'98CC;

public:
  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(SIMPLE_WEB_TRANSFER_RESET_RESULT_OK_MAGIC)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = SIMPLE_WEB_TRANSFER_RESET_RESULT_OK_MAGIC}.footprint();
  }
};

// ===== Composite =====

struct compositeWebTransferConfig final {
  tl::vector<tl::webProperty> properties;

  void store(tl::storer& tls) const noexcept {
    properties.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return properties.footprint();
  }
};

class CompositeWebTransferOpenResultOk final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_OPEN_RESULT_OK_MAGIC = 0x428A'89DD;

public:
  tl::u64 descriptor;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(COMPOSITE_WEB_TRANSFER_OPEN_RESULT_OK_MAGIC) && descriptor.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_OPEN_RESULT_OK_MAGIC}.footprint() + descriptor.footprint();
  }
};

class CompositeWebTransferAddResultOk final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_ADD_RESULT_OK_MAGIC = 0x3161'22DD;

public:
  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(COMPOSITE_WEB_TRANSFER_ADD_RESULT_OK_MAGIC)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_ADD_RESULT_OK_MAGIC}.footprint();
  }
};

class CompositeWebTransferRemoveResultOk final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_REMOVE_RESULT_OK_MAGIC = 0x8981'22FF;

public:
  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(COMPOSITE_WEB_TRANSFER_REMOVE_RESULT_OK_MAGIC)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_REMOVE_RESULT_OK_MAGIC}.footprint();
  }
};

class CompositeWebTransferPerformResultOk final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_PERFORM_RESULT_OK_MAGIC = 0xFF42'24DD;

public:
  tl::u64 remaining;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(COMPOSITE_WEB_TRANSFER_PERFORM_RESULT_OK_MAGIC) && remaining.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_PERFORM_RESULT_OK_MAGIC}.footprint() + remaining.footprint();
  }
};

class CompositeWebTransferCloseResultOk final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_CLOSE_RESULT_OK_MAGIC = 0xAB71'6222;

public:
  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(COMPOSITE_WEB_TRANSFER_CLOSE_RESULT_OK_MAGIC)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_CLOSE_RESULT_OK_MAGIC}.footprint();
  }
};

class CompositeWebTransferWaitUpdatesResultOk final {
  static constexpr uint32_t COMPOSITE_WEB_TRANSFER_WAIT_UPDATES_RESULT_OK_MAGIC = 0x2007'1997;

public:
  tl::u64 awaiters_num;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    bool ok{magic.fetch(tlf) && magic.expect(COMPOSITE_WEB_TRANSFER_WAIT_UPDATES_RESULT_OK_MAGIC) && awaiters_num.fetch(tlf)};
    return ok;
  }

  constexpr size_t footprint() const noexcept {
    return tl::magic{.value = COMPOSITE_WEB_TRANSFER_WAIT_UPDATES_RESULT_OK_MAGIC}.footprint() && awaiters_num.footprint();
  }
};

} // namespace tl
