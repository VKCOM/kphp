// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string_view>

namespace kphp::compiler_interface {

std::string_view get_main_file_name() noexcept;

} // namespace kphp::compiler_interface
