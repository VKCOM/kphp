// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/serialization/serialization-context.h"

static SerializationLibContext serialize_lib_context{};

SerializationLibContext& SerializationLibContext::get() noexcept {
  return serialize_lib_context;
}
