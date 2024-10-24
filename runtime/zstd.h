// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

constexpr int DEFAULT_COMPRESS_LEVEL = 3;

Optional<string> f$zstd_compress(const string &data, int64_t level = DEFAULT_COMPRESS_LEVEL) noexcept;

Optional<string> f$zstd_uncompress(const string &data) noexcept;

Optional<string> f$zstd_compress_dict(const string &data, const string &dict) noexcept;

Optional<string> f$zstd_uncompress_dict(const string &data, const string &dict) noexcept;
