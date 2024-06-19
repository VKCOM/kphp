#include "runtime-light/component/component.h"
#include "runtime-light/core/globals/php-init-scripts.h"

bool ComponentState::is_stream_already_being_processed(uint64_t stream_d) {
  return opened_streams.contains(stream_d);
}

bool ComponentState::is_stream_timer(uint64_t update_d) {
  return timer_callbacks.contains(update_d);
}

void ComponentState::process_new_input_stream(uint64_t stream_d) {
  bool already_pending = std::find(incoming_pending_queries.begin(), incoming_pending_queries.end(), stream_d)
                         != incoming_pending_queries.end();
  if (!already_pending) {
    php_debug("got new pending query %lu", stream_d);
    incoming_pending_queries.push_back(stream_d);
  }
  kphp_fork_context.scheduler.resume_fork_by_incoming_query();
}

void ComponentState::init_script_execution() {
  light_fork main_fork = light_fork();
  init_php_scripts_in_each_worker(php_script_mutable_globals_singleton, main_fork.task);
  main_fork.handle = main_fork.task.get_handle();
  kphp_fork_context.scheduler.register_main_fork(std::move(main_fork));
}

void ComponentState::process_timer(uint64_t stream_d) {
  get_platform_context()->free_descriptor(stream_d);
  timer_callbacks[stream_d]();
  timer_callbacks.erase(stream_d);
}
