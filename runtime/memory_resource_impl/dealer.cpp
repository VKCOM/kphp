// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memory_resource_impl//dealer.h"

namespace memory_resource {

Dealer::Dealer() noexcept
    : current_script_resource_(&default_script_resource_) {
  set_script_resource_replacer(heap_resource_);
}

} // namespace memory_resource
