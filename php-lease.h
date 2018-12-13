#pragma once

#include "common/pid.h"

#include "PHP/php-worker.h"

void lease_on_worker_finish(php_worker *worker);
void lease_set_ready(void);
void lease_on_stop();
void run_rpc_lease(void);
void do_rpc_stop_lease(void);
int do_rpc_start_lease(process_id_t pid, double timeout);
void lease_cron(void);
void set_main_target(int target);

