#pragma once

// for: struct in_addr
#include <netinet/in.h>

#include "common/version-string.h"

// Names and versions
#define VERSION "0.01"
#define NAME "php-engine"
#define NAME_VERSION NAME "-" VERSION


/***
 DEFAULT GLOBAL VARIABLES
 ***/

#define MAX_VALUE_LEN (1 << 24)
#define MAX_KEY_LEN 1000

#define DEFAULT_SCRIPT_TIMEOUT 30
#define MAX_SCRIPT_TIMEOUT (60 * 7)

/** engine variables **/
extern char *logname_pattern;
extern int logname_id;
extern long long max_memory;
extern int http_port;

extern int worker_id;
extern int pid;

extern int no_sql;

extern int master_flag;
extern int workers_n;

extern int run_once;
extern int run_once_return_code;

extern int die_on_fail;

/** stats **/
extern double load_time;
//uptime
typedef struct {
  long tot_queries;
  double worked_time;
  double net_time;
  double script_time;
  long tot_script_queries;
  double tot_idle_time;
  double tot_idle_percent;
  double a_idle_percent;
  int cnt;
} acc_stats_t;
extern acc_stats_t worker_acc_stats;

/** http **/
extern int http_port;
extern int http_sfd;

/** rcp **/
extern long long rpc_failed, rpc_sent, rpc_received, rpc_received_news_subscr, rpc_received_news_redirect;
extern int rpc_port;
extern int rpc_sfd;
extern int rpc_client_port;
extern const char *rpc_client_host;
extern int rpc_client_target;

/** master **/
extern const char *cluster_name;
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
#define IMM_STATS_DESC_LEN 128
typedef struct php_immediate_stats {
  int is_running;
  int is_wait_net;
  double timestamp;
  char desc[IMM_STATS_DESC_LEN];
  double custom_timestamp;
  char custom_desc[IMM_STATS_DESC_LEN];

  int port;
  long long actor_id;
  double rpc_timestamp;
} php_immediate_stats_t;


/***
  GLOBAL VARIABLES
 ***/
extern int sql_target_id;
extern int in_ready;
extern int script_timeout;
extern int disable_access_log;
extern int force_clear_sql_connection;
extern long long static_buffer_length_limit;
extern int use_madvise_dontneed;
extern long long memory_used_to_recreate_script;

#define RPC_PHP_IMMEDIATE_STATS 0x3d27a21b
#define RPC_PHP_FULL_STATS 0x1f8ae120

#define TL_KPHP_START_LEASE 0x61344739
#define TL_KPHP_LEASE_STATS 0x3013ebf4
#define TL_KPHP_STOP_LEASE 0x183bf49d

#define SPOLL_SEND_STATS 0x32d20000
#define SPOLL_SEND_IMMEDIATE_STATS 1
#define SPOLL_SEND_FULL_STATS 2

#define SIGTERM_MAX_TIMEOUT 10
#define SIGTERM_WAIT_TIMEOUT 0.1

#define SIGSTAT (SIGRTMIN)
#define MAX_WORKERS 1000

/***
  save of stdout/stderr fd
 ***/
extern int kstdout;
extern int kstderr;
extern int kproffd;

#ifdef __cplusplus
extern "C" {
#endif


void reopen_logs(void);

#ifdef __cplusplus
}
#endif
