// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

namespace vk {
namespace tl {
class TlHints;

struct tl_scheme;

size_t gen_php_code(const tl_scheme &scheme,
                    const TlHints &hints,
                    const std::string &out_php_dir,
                    bool forcibly_overwrite_dir,
                    bool generate_tests,
                    bool generate_tl_internals);

} // namespace tl
} // namespace vk
