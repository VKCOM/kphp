// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/msgpack/check_instance_depth.h"

namespace vk::msgpack {
size_t CheckInstanceDepth::depth = 0;
} // namespace vk::msgpack
