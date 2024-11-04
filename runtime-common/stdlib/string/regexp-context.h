// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <pcre.h>
#include <re2/stringpiece.h>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

class regexp;

struct RegexpContext final : private vk::not_copyable {
  static constexpr size_t MAX_SUBPATTERNS = 512;

  pcre_extra extra{};
  int64_t pcre_last_error{};
  int64_t preg_replace_count_dummy{};
  int64_t regexp_last_query_num{-1};
  // refactor me please :(
  // for i-th match(capturing group)
  // submatch[2 * i]     - start position of match
  // submatch[2 * i + 1] - end position of match
  std::array<int32_t, 3 * MAX_SUBPATTERNS> submatch{};
  std::array<re2::StringPiece, MAX_SUBPATTERNS> RE2_submatch{};

  std::array<char, sizeof(array<regexp *>)> regexp_cache_storage{};
  array<regexp *> *regexp_cache{reinterpret_cast<array<regexp *> *>(regexp_cache_storage.data())};

  static RegexpContext &get() noexcept;
};
