// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "server/php-query-data.h"
#include "server/php-runner.h"
#include "server/php-queries.h"


enum php_worker_mode_t {
  http_worker,
  rpc_worker,
  once_worker,
  job_worker
};

enum php_worker_state_t {
  phpq_try_start,
  phpq_init_script,
  phpq_run,
  phpq_free_script,
  phpq_finish
};

struct php_worker {
  struct connection *conn;

  php_query_data *data;

  bool paused;
  bool terminate_flag;
  script_error_t terminate_reason;
  const char *error_message;

  //for wait query
  int waiting;
  int wakeup_flag;
  double wakeup_time;

  double init_time;
  double start_time;
  double finish_time;

  php_worker_state_t state;
  php_worker_mode_t mode;

  long long req_id;
  int target_fd;
};

extern php_worker *active_worker;

php_worker *php_worker_create(php_worker_mode_t mode, connection *c, http_query_data *http_data, rpc_query_data *rpc_data, job_query_data *job_data,
                              long long req_id, double timeout);
void php_worker_free(php_worker *worker);

double php_worker_main(php_worker *worker);
void php_worker_terminate(php_worker *worker, int flag, script_error_t terminate_reason, const char *error_message);
void php_worker_on_wakeup(php_worker *worker);
/** trying to start query **/
void php_worker_try_start(php_worker *worker);

void php_worker_init_script(php_worker *worker);
void php_worker_run(php_worker *worker);
void php_worker_wait(php_worker *worker, int timeout_ms);
void php_worker_run_rpc_answer_query(php_worker *worker, php_query_rpc_answer *ans);
void php_worker_http_load_post(php_worker *worker, php_query_http_load_post_t *query);
void php_worker_answer_query(php_worker *worker, void *ans);
void php_worker_wakeup(php_worker *worker);
void php_worker_run_query(php_worker *worker);
void php_worker_set_result(php_worker *worker, script_result *res);
void php_worker_free_script(php_worker *worker);
void php_worker_finish(php_worker *worker);
double php_worker_get_timeout(php_worker *worker);
