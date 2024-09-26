// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

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
    return !value.fetch(tlb);
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

struct K2JobWorkerResponse final {
  int64_t job_id{};
  string body;

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

// ===== CRYPTO =====

// Actually it's "Maybe (Dictionary CertInfoItem)"
// But I now want to have this logic separately
struct GetPemCertInfoResponse final {
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

struct ConfdataValue final {
  string value;
  bool is_php_serialized{};
  bool is_json_serialized{};

  bool fetch(TLBuffer &tlb) noexcept;

  void store(TLBuffer &tlb) const noexcept;
};

} // namespace tl
