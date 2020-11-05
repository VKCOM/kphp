// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk {
namespace tl {

struct tl_scheme;

void perform_flat_optimization(tl_scheme &scheme);

} // namespace tl
} // namespace vk