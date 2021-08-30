// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "net/net-connections.h"

#include "server/php-queries.h"
#include "server/php-worker.h"

void php_worker_run_sql_query_packet(php_worker *worker, php_net_query_packet_t *query);
bool set_mysql_db_name(const char *db_name);
bool set_mysql_user(const char *user);
bool set_mysql_password(const char *password);
void disable_mysql_same_datacenter_check();
extern conn_target_t db_ct;
