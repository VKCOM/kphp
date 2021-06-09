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

/** engine variables **/
extern int initial_verbosity;

extern char *logname_pattern;
extern int logname_id;
extern long long max_memory;

extern int worker_id;
extern int pid;

extern int no_sql;

extern int master_flag;

extern int run_once;
extern int run_once_return_code;

extern int die_on_fail;

/** http **/
extern int http_port;
extern int http_sfd;

/** rpc **/
extern int rpc_port;
extern int rpc_sfd;
extern long long rpc_client_actor;

/** master **/
extern int master_port;
extern int master_sfd;
extern int master_sfd_inited;
extern int master_pipe_write;
extern int master_pipe_fast_write;

/** sigterm **/
extern double sigterm_time;
extern int sigterm_on;
extern int rpc_stopped;

/** script **/
struct php_immediate_stats_t {
  bool is_running;
  bool is_wait_net;
  bool is_ready_for_accept;
};

/***
  GLOBAL VARIABLES
 ***/
extern int sql_target_id;
extern int script_timeout;
extern int disable_access_log;
extern int force_clear_sql_connection;
extern long long static_buffer_length_limit;
extern int use_madvise_dontneed;
extern long long memory_used_to_recreate_script;

#define RPC_PHP_IMMEDIATE_STATS 0x3d27a21b
#define RPC_PHP_FULL_STATS 0x1f8ae120

#define SPOLL_SEND_STATS 0x32d20000
#define SPOLL_SEND_IMMEDIATE_STATS 1
#define SPOLL_SEND_FULL_STATS 2

#define SIGTERM_MAX_TIMEOUT 10
#define SIGTERM_WAIT_TIMEOUT 0.1

#if defined(__APPLE__)
#define SIGSTAT (SIGINFO)
#define SIGPHPASSERT (SIGCONT)
#define SIGSTACKOVERFLOW (SIGTSTP)
#else
#define SIGSTAT (SIGRTMIN)
#define SIGPHPASSERT (SIGRTMIN + 1)
#define SIGSTACKOVERFLOW (SIGRTMIN + 2)
#endif

/***
  save of stdout/stderr fd
 ***/
extern int kstdout;
extern int kstderr;

