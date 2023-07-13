// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <csetjmp>

#include "common/dl-utils-lite.h"
#include "common/mixin/not_copyable.h"
#include "common/sanitizer.h"

#include "server/php-engine-vars.h"
#include "server/php-init-scripts.h"
#include "server/php-queries-types.h"
#include "server/php-query-data.h"
#include "server/ucontext-portable.h"

enum class run_state_t {
  finished,
  uncleared,
  query,
  error,
  ready,
  before_init,
  running,
  empty,
  query_running,
};

enum class script_error_t : uint8_t {
  no_error = 0,
  memory_limit,
  timeout,
  exception,
  stack_overflow,
  php_assert,
  http_connection_close,
  rpc_connection_close,
  net_event_error,
  post_data_loading_error,
  unclassified_error,
  errors_count
};

struct query_stats_t {
  long long q_id;
  const char *desc;
  int port;
  const char *query;
  double timeout;
};

extern query_stats_t query_stats;
extern long long query_stats_id;

void dump_query_stats();

class PhpScriptStack : vk::not_copyable {
public:
  explicit PhpScriptStack(size_t stack_size) noexcept;
  ~PhpScriptStack() noexcept;

  bool is_protected(const char *x) const noexcept;
  bool check_stack_overflow(const char *x) const noexcept;
  void asan_stack_unpoison() const noexcept;
  void asan_stack_clear() const noexcept;

  char *get_stack_ptr() const noexcept { return run_stack_; }
  size_t get_stack_size() const noexcept { return stack_size_; }

private:
  const size_t stack_size_{0};
  char *run_stack_{nullptr};
  const char *protected_end_{nullptr};
  const char *run_stack_end_{nullptr};
};

/**
 * Represents current running script.
 * It stores state of the script: current execution point, pointers to allocated script memory, stack for script context, etc.
 */
class PhpScript {
  double cur_timestamp{0}, net_time{0}, script_time{0};
  double last_net_time_delta{0};
  int queries_cnt{0};
  int long_queries_cnt{0};

private:
  int swapcontext_helper(ucontext_t_portable *oucp, const ucontext_t_portable *ucp);

  void assert_state(run_state_t expected);

public:
  static PhpScript *volatile current_script;
  static ucontext_t_portable exit_context;
  volatile static bool in_script_context;
  volatile static bool time_limit_exceeded;
  volatile static bool memory_limit_exceeded;

  run_state_t state{run_state_t::empty};
  const char *error_message{nullptr};
  script_error_t error_type{script_error_t::no_error};
  php_query_base_t *query{nullptr};
  const size_t mem_size{0};
  double oom_handling_memory_ratio{0};
  char *run_mem{nullptr};
  PhpScriptStack script_stack;

  ucontext_t_portable run_context{};
  sigjmp_buf timeout_handler{};

  script_t *run_main{nullptr};
  php_query_data *data{nullptr};
  script_result *res{nullptr};

  static void script_context_entrypoint() noexcept;
  static void error(const char *error_message, script_error_t error_type) noexcept;

  PhpScript(size_t mem_size, double oom_handling_memory_ratio, size_t stack_size) noexcept;
  ~PhpScript() noexcept;

  void try_run_shutdown_functions_on_timeout() noexcept;
  void check_net_context_errors() noexcept;

  void init(script_t *script, php_query_data *data_to_set) noexcept;

  void pause() noexcept;
  void ask_query(php_query_base_t *q) noexcept;
  void set_script_result(script_result *res_to_set) noexcept;

  void resume() noexcept;
  run_state_t iterate() noexcept;

  void finish() noexcept;
  void clear() noexcept;
  void query_readed() noexcept;
  void query_answered() noexcept;

  void run() noexcept;

  void terminate(const char *error_message_, script_error_t error_type_) noexcept;

  void update_net_time() noexcept;
  void update_script_time() noexcept;

  void reset_script_timeout() noexcept;
  double get_net_time() const noexcept;
  double get_script_time() noexcept;
  int get_net_queries_count() const noexcept;
  long long memory_get_total_usage() const noexcept;

  void disable_timeout() noexcept;
  void set_timeout(double t) noexcept;
};
