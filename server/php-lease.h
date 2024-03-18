// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/kphp-tasks-lease/lease-worker-mode.h"
#include "common/kphp-tasks-lease/lease-worker-settings.h"
#include "common/kprintf.h"
#include "common/pid.h"

#include "server/lease-rpc-client.h"
#include "server/php-worker.h"

DECLARE_VERBOSITY(lease);

void lease_on_worker_finish(PhpWorker *worker);
void lease_set_ready();
void lease_on_stop();
void run_rpc_lease();

void do_rpc_stop_lease();
int do_rpc_start_lease(process_id_t pid, double timeout, int actor_id);
void do_rpc_finish_lease();
void lease_cron();
void set_main_target(const LeaseRpcClient &client);
int get_current_target();
connection *get_lease_connection();
process_id_t get_lease_pid();
process_id_t get_rpc_main_target_pid();

lease_worker_settings try_fetch_lookup_custom_worker_settings();
