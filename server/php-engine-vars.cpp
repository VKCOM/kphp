// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-engine-vars.h"

#include <limits.h>
#include <stdlib.h>

/***
 DEFAULT GLOBAL VARIABLES
 ***/

/** engine variables **/

int initial_verbosity = 0;

char *logname_pattern = nullptr;
int logname_id = 0;
long long max_memory = 1 << 29;
double oom_handling_memory_ratio = 0.00;

int worker_id = -1;
int pid = -1;

int no_sql = 0;

ProcessType process_type = ProcessType::master;

int master_flag = 0; // 1 -- master, 0 -- single process, -1 -- child

bool run_once = false;
int run_once_return_code = 0;

int die_on_fail = 0;

/** rpc **/
int rpc_port = -1;
int rpc_sfd = -1;
long long rpc_client_actor = -1;

/** sigterm **/
double sigterm_time = 0;
bool sigterm_on = false;
int rpc_stopped = 0;

/** master **/
int master_port = -1;
int master_sfd = -1;
int master_sfd_inited = 0;

/***
  GLOBAL VARIABLES
 ***/
int sql_target_id = -1;
int script_timeout = 0;
double hard_timeout = 1.0;
int disable_access_log = 0;
int force_clear_sql_connection = 0;
long long static_buffer_length_limit = -1;
int use_madvise_dontneed = 0;
long long memory_used_to_recreate_script = LLONG_MAX;
double sigterm_wait_timeout = 0.1;

/***
  save of stdout/stderr fd
 ***/
int kstdout = 1;
int kstderr = 2;
