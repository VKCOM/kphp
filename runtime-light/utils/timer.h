// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

#include "runtime-core/memory-resource/resource_allocator.h"
#include "runtime-light/component/component.h"

uint64_t set_timer_without_callback(int64_t timeout_ns);
