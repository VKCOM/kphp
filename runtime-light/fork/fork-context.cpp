// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/fork/fork-context.h"

#include <cstdint>
#include <unordered_map>

#include "runtime-core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/fork/fork.h"
#include "runtime-light/utils/context.h"

ForkComponentContext::ForkComponentContext(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
  : forks_(unordered_map<int64_t, task_t<fork_result>>::allocator_type{memory_resource}) {}

ForkComponentContext &ForkComponentContext::get() noexcept {
  return get_component_context()->fork_component_context;
}

int64_t ForkComponentContext::register_fork(task_t<fork_result> &&task) noexcept {
  const auto fork_id{next_fork_id_++};
  forks_.emplace(fork_id, std::move(task));
  php_debug("ForkComponentContext: register fork %" PRId64, fork_id);
  return fork_id;
}

void ForkComponentContext::unregister_fork(int64_t fork_id) noexcept {
  forks_.erase(fork_id);
  php_debug("ForkComponentContext: unregister fork %" PRId64, fork_id);
}
