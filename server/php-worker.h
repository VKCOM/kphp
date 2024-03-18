// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <optional>

#include "server/php-queries.h"
#include "server/php-query-data.h"
#include "server/php-runner.h"

enum php_worker_mode_t { http_worker, rpc_worker, once_worker, job_worker };

enum php_worker_state_t { phpq_try_start, phpq_init_script, phpq_run, phpq_free_script, phpq_finish };

/**
 * Logical PHP worker, not related to physical worker process.
 * Created at the begin of any request processing which requires to run generated from PHP code.
 * Deleted at the end of request processing.
 * It stores connection and other network related stuff. This is some kind of bridge between event loop and script context.
 * It runs generated from PHP code by using PhpScript.
 */
class PhpWorker {
public:
  struct connection *conn;

  php_query_data_t data;

  bool paused;
  bool flushed_http_connection;
  bool terminate_flag;
  script_error_t terminate_reason;
  const char *error_message;

  // for wait query
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

  PhpWorker(php_worker_mode_t mode_, connection *c, php_query_data_t php_query_data, long long req_id_, double timeout);
  ~PhpWorker() = default;

  std::optional<double> enter_lifecycle() noexcept;

  void terminate(int flag, script_error_t terminate_reason_, const char *error_message_) noexcept;
  double get_timeout() const noexcept;

  void answer_query(void *ans) noexcept;
  void wait(int timeout_ms) noexcept;
  void wakeup() noexcept;
  void run_query() noexcept;
  void on_wakeup() noexcept;
  void set_result(script_result *res) noexcept;

private:
  void state_try_start() noexcept;
  void state_init_script() noexcept;
  void state_run() noexcept;
  void state_free_script() noexcept;
  void state_finish() noexcept;
};

extern std::optional<PhpWorker> php_worker;
