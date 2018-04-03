#include "PHP/php-engine-vars.h"

#include <limits.h>
#include <stdlib.h>

/***
 DEFAULT GLOBAL VARIABLES
 ***/

/** engine variables **/
char *logname_pattern = NULL;
int logname_id = 0;
long long max_memory = 1 << 29;
int http_port;

int worker_id = -1;
int pid = -1;

int no_sql = 0;

int master_flag = 0; // 1 -- master, 0 -- single process, -1 -- child
int workers_n = 0;

int run_once = 0;
int run_once_return_code = 0;

int die_on_fail = 0;

/** stats **/
double load_time;
acc_stats_t worker_acc_stats;

/** http **/
int http_port = -1;
int http_sfd = -1;

/** rcp **/
long long rpc_failed, rpc_sent, rpc_received, rpc_received_news_subscr, rpc_received_news_redirect;
int rpc_port = -1;
int rpc_sfd = -1;
int rpc_client_port = -1;
const char *rpc_client_host = NULL;
int rpc_client_target = -1;

/** sigterm **/
double sigterm_time = 0;
int sigterm_on = 0;
int rpc_stopped = 0;

/** master **/
const char *cluster_name = "default";
int master_port = -1;
int master_sfd = -1;
int master_sfd_inited = 0;
int master_pipe_write = -1;
int master_pipe_fast_write = -1;


/***
  GLOBAL VARIABLES
 ***/
int sql_target_id = -1;
int in_ready = 0;
int script_timeout = 0;
int disable_access_log = 0;
int force_clear_sql_connection = 0;
long long static_buffer_length_limit = -1;
int use_madvise_dontneed = 0;
long long memory_used_to_recreate_script = LLONG_MAX;

/***
  save of stdout/stderr fd
 ***/
int kstdout = 1;
int kstderr = 2;
