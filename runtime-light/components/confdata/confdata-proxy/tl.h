// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace tl::confdata {

struct keyValuePair final {
  tl::string key{};
  tl::string value{};
  tl::Bool is_php_serialized{};
  tl::Bool is_json_serialized{};

  bool fetch(tl::fetcher& tlf) noexcept {
    return key.fetch(tlf) && value.fetch(tlf) && is_php_serialized.fetch(tlf) && is_json_serialized.fetch(tlf);
  }

  constexpr size_t footprint() const noexcept {
    return key.footprint() + value.footprint() + is_php_serialized.footprint() + is_json_serialized.footprint();
  }
};

class KeyValuePair final {
  static constexpr tl::magic MAGIC{.value = 0xff1c'b454};

public:
  tl::confdata::keyValuePair inner{};

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    return magic.fetch(tlf) && magic.expect(MAGIC) && inner.fetch(tlf);
  }

  constexpr size_t footprint() const noexcept {
    return MAGIC.footprint() + inner.footprint();
  }
};

struct subscribeResponseOk final {
  tl::vector<tl::confdata::KeyValuePair> events{};
  tl::string new_page{};
  tl::i64 new_offset{};
  tl::Bool new_has_synced{};

  bool fetch(tl::fetcher& tlf) noexcept {
    return events.fetch(tlf) && new_page.fetch(tlf) && new_offset.fetch(tlf) && new_has_synced.fetch(tlf);
  }

  constexpr size_t footprint() const noexcept {
    return events.footprint() + new_page.footprint() + new_offset.footprint() + new_has_synced.footprint();
  }
};

struct subscribeResponseOldOffsetError final {
  bool fetch(tl::fetcher& /*unused*/) noexcept {
    return true;
  }

  constexpr size_t footprint() const noexcept {
    return 0;
  }
};

class SubscribeResponse final {
  static constexpr tl::magic SUBSCRIBE_RESPONSE_OK_MAGIC{.value = 0x2709'63e8};
  static constexpr tl::magic SUBSCRIBE_RESPONSE_OLD_OFFSET_ERROR_MAGIC{.value = 0x11eb'eb02};

public:
  std::variant<tl::confdata::subscribeResponseOk, tl::confdata::subscribeResponseOldOffsetError> value;

  bool fetch(tl::fetcher& tlf) noexcept {
    tl::magic magic{};
    if (!magic.fetch(tlf)) {
      return false;
    }

    if (tl::confdata::subscribeResponseOk response_ok{}; magic.expect(SUBSCRIBE_RESPONSE_OK_MAGIC) && response_ok.fetch(tlf)) {
      value.emplace<tl::confdata::subscribeResponseOk>(std::move(response_ok));
      return true;
    }
    if (tl::confdata::subscribeResponseOldOffsetError old_offset_error{};
        magic.expect(SUBSCRIBE_RESPONSE_OLD_OFFSET_ERROR_MAGIC) && old_offset_error.fetch(tlf)) {
      value.emplace<tl::confdata::subscribeResponseOldOffsetError>(old_offset_error);
      return true;
    }
    return false;
  }

  constexpr size_t footprint() const noexcept {
    return std::visit(
        [](const auto& value) noexcept {
          using value_t = std::remove_cvref_t<decltype(value)>;
          if constexpr (std::same_as<value_t, tl::confdata::subscribeResponseOk>) {
            return SUBSCRIBE_RESPONSE_OK_MAGIC.footprint() + value.footprint();
          } else if constexpr (std::same_as<value_t, tl::confdata::subscribeResponseOldOffsetError>) {
            return SUBSCRIBE_RESPONSE_OLD_OFFSET_ERROR_MAGIC.footprint() + value.footprint();
          } else {
            static_assert(false, "non-exhaustive visitor!");
          }
        },
        value);
  }
};

class Subscribe final {
  static constexpr tl::magic MAGIC{.value = 0xfebd'1230};

public:
  tl::mask fields_mask{};
  tl::string access_token{};
  tl::string page{};
  tl::i64 offset{};
  tl::Bool has_synced{};
  tl::vector<tl::string> prefixes{};

  void store(tl::storer& tls) const noexcept {
    MAGIC.store(tls), fields_mask.store(tls), access_token.store(tls), page.store(tls), offset.store(tls), has_synced.store(tls), prefixes.store(tls);
  }

  constexpr size_t footprint() const noexcept {
    return MAGIC.footprint() + fields_mask.footprint() + access_token.footprint() + page.footprint() + offset.footprint() + has_synced.footprint() +
           prefixes.footprint();
  }
};

} // namespace tl::confdata
