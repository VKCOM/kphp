#pragma once

#include "common/kphp-tasks-lease/lease-worker-mode.h"
#include "common/kphp-tasks-lease/lease-worker-settings.h"
#include "common/pid.h"
#include "common/wrappers/optional.h"

#include "server/lease-rpc-client.h"
#include "server/php-worker.h"

void lease_on_worker_finish(php_worker *worker);
void lease_set_ready();
void lease_on_stop();
void run_rpc_lease();
void do_rpc_stop_lease();
int do_rpc_start_lease(process_id_t pid, double timeout);
void lease_cron();
void set_main_target(const LeaseRpcClient &client);
int get_current_target();
connection *get_lease_connection();
process_id_t get_lease_pid();
process_id_t get_rpc_main_target_pid();

lease_worker_settings try_fetch_lookup_custom_worker_settings();
