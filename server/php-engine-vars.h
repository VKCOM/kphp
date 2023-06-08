// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// for: struct in_addr
#include <netinet/in.h>

#include "common/version-string.h"

// Names and versions
#define VERSION "0.01"
#define NAME_VERSION "php-engine-" VERSION


/***
 DEFAULT GLOBAL VARIABLES
 ***/

#define DEFAULT_SCRIPT_TIMEOUT 30
#define MAX_SCRIPT_TIMEOUT (60 * 7)

enum class ProcessType { master, http_worker, rpc_worker, job_worker };

/** engine variables **/
extern int initial_verbosity;

extern char *logname_pattern;
extern int logname_id;
extern long long max_memory;
extern double oom_handling_memory_ratio;

extern int worker_id;
extern int pid;

extern int no_sql;

extern ProcessType process_type;

extern int master_flag;

extern bool run_once;
extern int run_once_return_code;

extern int die_on_fail;

/** rpc **/
extern int rpc_port;
extern int rpc_sfd;
extern long long rpc_client_actor;

/** master **/
extern int master_port;
extern int master_sfd;
extern int master_sfd_inited;

/** sigterm **/
extern double sigterm_time;
extern bool sigterm_on;
extern int rpc_stopped;

/***
  GLOBAL VARIABLES
 ***/
extern int sql_target_id;
extern int script_timeout;
extern double hard_timeout;
extern int disable_access_log;
extern int force_clear_sql_connection;
extern long long static_buffer_length_limit;
extern int use_madvise_dontneed;
extern long long memory_used_to_recreate_script;

extern double sigterm_wait_timeout;
constexpr double SIGTERM_MAX_TIMEOUT = 10.0;

#if defined(__APPLE__)
#define SIGPHPASSERT (SIGCONT)
#define SIGSTACKOVERFLOW (SIGTSTP)
#else
#define SIGPHPASSERT (SIGRTMIN + 1)
#define SIGSTACKOVERFLOW (SIGRTMIN + 2)
#endif

/***
  save of stdout/stderr fd
 ***/
extern int kstdout;
extern int kstderr;

