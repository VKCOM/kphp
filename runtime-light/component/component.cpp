// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/component/component.h"
#include "runtime-light/core/globals/php-init-scripts.h"

bool ComponentState::is_descriptor_stream(uint64_t update_d) {
  if (const auto it = opened_descriptors.find(update_d); it != opened_descriptors.end()) {
    return it->second == DescriptorRuntimeStatus::Stream;
  }
  return false;
}

bool ComponentState::is_descriptor_timer(uint64_t update_d) {
  if (const auto it = opened_descriptors.find(update_d); it != opened_descriptors.end()) {
    return it->second == DescriptorRuntimeStatus::Timer;
  }
  return false;
}

void ComponentState::process_new_input_stream(uint64_t stream_d) {
  bool already_pending = std::find(incoming_pending_queries.begin(), incoming_pending_queries.end(), stream_d) != incoming_pending_queries.end();
  if (!already_pending) {
    php_debug("got new pending query %lu", stream_d);
    incoming_pending_queries.push_back(stream_d);
  }
  kphp_fork_context.scheduler.mark_fork_ready_by_incoming_query();
}

void ComponentState::init_script_execution() {
  kphp_core_context.init();

  light_fork main_fork = light_fork();
  init_php_scripts_in_each_worker(php_script_mutable_globals_singleton, main_fork.task);
  main_fork.handle = main_fork.task.get_handle();
  kphp_fork_context.scheduler.register_main_fork(std::move(main_fork));
}
