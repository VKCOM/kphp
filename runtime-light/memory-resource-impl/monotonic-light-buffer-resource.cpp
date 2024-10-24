// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/memory-resource/monotonic_buffer_resource.h"

namespace memory_resource {
// todo:k2 How it should work in k2?
void monotonic_buffer_resource::critical_dump(void *, size_t) const noexcept {}

void monotonic_buffer_resource::raise_oom(size_t) const noexcept {}
} // namespace memory_resource