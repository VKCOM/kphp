#pragma once

#include "net/net-connections.h"

#include "PHP/php-queries.h"
#include "PHP/php-worker.h"

void php_worker_run_sql_query_packet(php_worker *worker, php_net_query_packet_t *query);
extern conn_target_t db_ct;
