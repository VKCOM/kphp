// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"

int64_t f$printf(const string &format, const array<mixed> &a) noexcept;

int64_t f$vprintf(const string &format, const array<mixed> &args) noexcept;

Optional<array<mixed>> f$str_getcsv(const string &s, const string &delimiter = StringLibConstants::get().COMMA_STR,
                                    const string &enclosure = StringLibConstants::get().QUOTE_STR,
                                    const string &escape = StringLibConstants::get().BACKSLASH_STR) noexcept;
