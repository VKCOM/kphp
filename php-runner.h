#pragma once

#include <ucontext.h>

#include "PHP/php-engine-vars.h"
#include "PHP/php_script.h"
#include "drinkless/dl-utils-lite.h"

enum run_state_t {
  rst_finished,
  rst_uncleared,
  rst_query,
  rst_error,
  rst_ready,
  rst_before_init,
  rst_running,
  rst_empty,
  rst_query_running
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

struct query_int {
  query_type type; // MUST BE HERE
  int val, res;
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

void dump_query_stats(void);

int msq(int x);

void init_handlers(void);
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
void php_script_terminate(void *ptr, const char *error_message);
void php_script_set_timeout(double t);
const char *php_script_get_error(void *ptr);
long long php_script_memory_get_total_usage(void *ptr);

/** script **/
php_immediate_stats_t *get_immediate_stats();
php_immediate_stats_t *get_imm_stats();
void custom_server_status(const char *status, int status_len);
void server_status_rpc(int port, long long actor_id, double start_time);
void idle_server_status(void);
void wait_net_server_status(void);
void running_server_status(void);

class PHPScriptBase;

class PHPScriptBase {
  double cur_timestamp, net_time, script_time;
  int queries_cnt;

public:

  static PHPScriptBase *volatile current_script;
  static ucontext_t exit_context;
  volatile static bool is_running;
  volatile static bool tl_flag;
  volatile static bool ml_flag;

  run_state_t state;
  const char *error_message;
  void *query;
  char *run_stack, *protected_end, *run_stack_end, *run_mem;
  size_t mem_size, stack_size;
  ucontext_t run_context;

  script_t *run_main;
  php_query_data *data;
  script_result *res;

  static void cur_run();
  static void error(const char *error_message);

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
  void query_readed(void);
  void query_answered(void);

  void run(void);

  void update_net_time(void);
  void update_script_time(void);

  double get_net_time(void) const;
  double get_script_time(void);
  int get_net_queries_count(void) const;
  long long memory_get_total_usage(void) const;
};

//TODO: sometimes I need to call old handlers
void sigalrm_handler(int signum);
void sigsegv_handler(int signum, siginfo_t *info, void *data);

