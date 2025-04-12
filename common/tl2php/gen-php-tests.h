// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

namespace vk {
namespace tl {

struct PhpClasses;

void gen_php_tests(const std::string &dir, const PhpClasses &classes);

} // namespace tl
} // namespace vk
