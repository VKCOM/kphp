// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/msgpack/check_instance_depth.h"

namespace vk::msgpack {
size_t CheckInstanceDepth::depth = 0;
} // namespace vk::msgpack
