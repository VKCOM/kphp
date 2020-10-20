#pragma once

#include <string>

namespace vk {
namespace tl {

struct PhpClasses;

void gen_php_tests(const std::string &dir, const PhpClasses &classes);

} // namespace tl
} // namespace vk
