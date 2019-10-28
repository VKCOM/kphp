#pragma once

#include "server/php-query-data.h"

enum php_worker_mode_t {
  http_worker,
  rpc_worker,
  once_worker
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

  int paused;
  int terminate_flag;
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

