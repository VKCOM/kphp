// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/msgpack-serialization.h"

#include "msgpack/adaptors.h"

namespace msgpack::adaptor {

size_t CheckInstanceDepth::depth = 0;

} // namespace msgpack::adaptor
