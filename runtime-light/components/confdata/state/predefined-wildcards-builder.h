// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <string_view>

#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

namespace kphp::confdata {

auto validate_predefined_wildcard(std::string_view wildcard) noexcept -> std::expected<void, predefined_wildcards_error>;

/** @return The number of bytes needed to encode sorted, unique `wildcards`. */
auto predefined_wildcards_metadata_size(std::span<const std::string_view> wildcards) noexcept -> std::expected<size_t, predefined_wildcards_error>;

/**
 * @brief Writes sorted, unique `wildcards` into relocatable immutable metadata at the start of `buffer`.
 * @return A read-only view over the written metadata.
 */
auto write_predefined_wildcards(std::span<std::byte> buffer,
                                std::span<const std::string_view> wildcards) noexcept -> std::expected<predefined_wildcards, predefined_wildcards_error>;

} // namespace kphp::confdata
