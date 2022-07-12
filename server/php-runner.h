// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <csetjmp>

#include "common/dl-utils-lite.h"
#include "common/sanitizer.h"

#include "server/php-engine-vars.h"
#include "server/php-query-data.h"
#include "server/php-queries-types.h"
#include "server/php-script.h"
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

#define SEGV_STACK_SIZE (MINSIGSTKSZ + 65536)

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

void init_handlers();
void php_script_finish(void *ptr);
void php_script_free(void *ptr);
void php_script_clear(void *ptr);
void php_script_init(void *ptr, script_t *f_run, php_query_data *data_to_set);
run_state_t php_script_iterate(void *ptr);
php_query_base_t *php_script_get_query(void *ptr);
script_result *php_script_get_res(void *ptr);
void php_script_query_readed(void *ptr);
void php_script_query_answered(void *ptr);
run_state_t php_script_get_state(void *ptr);
void *php_script_create(size_t mem_size, size_t stack_size);
void php_script_terminate(void *ptr, const char *error_message, script_error_t error_type);
void php_script_disable_timeout();
void php_script_set_timeout(double t);
const char *php_script_get_error(void *ptr);
script_error_t php_script_get_error_type(void *ptr);

long long php_script_memory_get_total_usage(void *ptr);

/** script **/
class PHPScriptBase {
  double cur_timestamp, net_time, script_time;
  int queries_cnt;
  int long_queries_cnt{0};

private:
#if ASAN7_ENABLED
  bool fiber_is_started = false;
#endif
  int swapcontext_helper(ucontext_t_portable *oucp, const ucontext_t_portable *ucp);

  void on_request_timeout_error();

  void assert_state(run_state_t expected);

public:

  static PHPScriptBase *volatile current_script;
  static ucontext_t_portable exit_context;
  volatile static bool is_running;
  volatile static bool tl_flag;
  volatile static bool ml_flag;

  run_state_t state;
  const char *error_message;
  script_error_t error_type;
  php_query_base_t *query;
  char *run_stack, *protected_end, *run_stack_end, *run_mem;
  size_t mem_size, stack_size;
  ucontext_t_portable run_context;

  sigjmp_buf timeout_handler;

  script_t *run_main;
  php_query_data *data;
  script_result *res;

  static void cur_run();
  static void error(const char *error_message, script_error_t error_type);

  void check_tl();

  bool is_protected(char *x);
  bool check_stack_overflow(char *x);

  PHPScriptBase(size_t mem_size, size_t stack_size);
  virtual ~PHPScriptBase();

  void init(script_t *script, php_query_data *data_to_set);

  void asan_stack_unpoison();

  //in php script
  void pause();
  void ask_query(php_query_base_t *q);
  void set_script_result(script_result *res_to_set);

  void resume();
  run_state_t iterate();

  void finish();
  void clear();
  void query_readed();
  void query_answered();

  void run();

  void update_net_time();
  void update_script_time();

  void reset_script_timeout();
  double get_net_time() const;
  double get_script_time();
  int get_net_queries_count() const;
  long long memory_get_total_usage() const;
};

