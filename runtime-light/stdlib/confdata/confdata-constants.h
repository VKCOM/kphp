// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

namespace kphp::confdata {

inline constexpr std::string_view IMAGE_NAME{"kphp-confdata"};

// K2 resolves component streams by the link alias from the caller's linking
// config, not by the target image or component name. KPHP images that use
// confdata must therefore expose the confdata component under this alias.
inline constexpr std::string_view COMPONENT_LINK_ALIAS{"kphp-confdata"};

} // namespace kphp::confdata
