#pragma once

#include "PHP/common-net-functions.h"

typedef enum {
  http_worker,
  rpc_worker,
  once_worker
} php_worker_mode_t;
typedef enum {
  phpq_try_start,
  phpq_init_script,
  phpq_run,
  phpq_free_script,
  phpq_finish
} php_worker_state_t;
typedef struct {
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
} php_worker;

