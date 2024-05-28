#include "runtime-light/component/component.h"
#include "runtime-light/core/globals/php-init-scripts.h"

void ComponentState::resume_if_was_rescheduled() {
  if (poll_status == PollStatus::PollReschedule) {
    // If component was suspended by please yield and there is no awaitable streams
   main_thread();
  }
}

bool ComponentState::is_stream_already_being_processed(uint64_t stream_d) {
  return opened_streams.contains(stream_d);
}

void ComponentState::resume_if_wait_stream(uint64_t stream_d, StreamStatus status) {
  if (is_stream_timer(stream_d)) {
    process_timer(stream_d);
  } else {
    process_stream(stream_d, status);
  }
}

void ComponentState::process_new_input_stream(uint64_t stream_d) {
  bool already_pending = std::find(incoming_pending_queries.begin(), incoming_pending_queries.end(), stream_d)
                         != incoming_pending_queries.end();
  if (!already_pending) {
    php_debug("got new pending query %lu", stream_d);
    incoming_pending_queries.push_back(stream_d);
  }
  if (wait_incoming_stream) {
    php_debug("start process pending query %lu", stream_d);
    main_thread();
  }
}

void ComponentState::init_script_execution() {
  //todo: вынести в image state??
  init_php_scripts_in_each_worker(php_script_mutable_globals_singleton, k_main);
  main_thread = k_main.get_handle();
}

bool ComponentState::is_stream_timer(uint64_t stream_d) {
  return timer_callbacks.contains(stream_d);
}

void ComponentState::process_timer(uint64_t stream_d) {
  get_platform_context()->free_descriptor(stream_d);
  timer_callbacks[stream_d]();
  timer_callbacks.erase(stream_d);
  opened_streams.erase(stream_d);
}

void ComponentState::process_stream(uint64_t stream_d, StreamStatus status) {
  auto expected_status = opened_streams[stream_d];
  if ((expected_status == WBlocked && status.write_status != IOBlocked) ||
      (expected_status == RBlocked && status.read_status != IOBlocked)) {
    php_debug("resume on waited query %lu", stream_d);
    auto suspend_point = awaiting_coroutines[stream_d];
    awaiting_coroutines.erase(stream_d);
    php_assert(awaiting_coroutines.empty());
    suspend_point();
  }
}
