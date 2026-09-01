// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

namespace kphp::confdata {

inline constexpr std::string_view IMAGE_NAME{"confdata"};

// K2 resolves component streams by the link alias from the caller's linking
// config, not by the target image or component name. KPHP images that use
// confdata must therefore expose the confdata component under this alias.
inline constexpr std::string_view COMPONENT_LINK_ALIAS{"confdata"};

inline constexpr std::string_view SHARED_MEMORY_NAME{"#kphp-confdata"};

} // namespace kphp::confdata
