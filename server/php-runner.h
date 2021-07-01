// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <ucontext.h>

#include "common/dl-utils-lite.h"
#include "common/sanitizer.h"

#include "server/php-engine-vars.h"
#include "server/php-query-data.h"
#include "server/php-script.h"

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

enum query_type {
  q_memcached,
  q_rpc,
  q_int
};

#define SEGV_STACK_SIZE (MINSIGSTKSZ + 65536)

#pragma pack(push, 4)

struct query_base {
  query_type type;
};

#pragma pack(pop)

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
query_base *php_script_get_query(void *ptr);
script_result *php_script_get_res(void *ptr);
void php_script_query_readed(void *ptr);
void php_script_query_answered(void *ptr);
run_state_t php_script_get_state(void *ptr);
void *php_script_create(size_t mem_size, size_t stack_size);
void php_script_terminate(void *ptr, const char *error_message, script_error_t error_type);
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
  int swapcontext_helper(ucontext_t *oucp, const ucontext_t *ucp);

public:

  static PHPScriptBase *volatile current_script;
  static ucontext_t exit_context;
  volatile static bool is_running;
  volatile static bool tl_flag;
  volatile static bool ml_flag;

  run_state_t state;
  const char *error_message;
  script_error_t error_type;
  void *query;
  char *run_stack, *protected_end, *run_stack_end, *run_mem;
  size_t mem_size, stack_size;
  ucontext_t run_context;

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

  //in php script
  void pause();
  void ask_query(void *q);
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

  double get_net_time() const;
  double get_script_time();
  int get_net_queries_count() const;
  long long memory_get_total_usage() const;
};

