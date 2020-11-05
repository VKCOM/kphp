// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "net/net-memcache-client.h"

#include "server/php-queries.h"
#include "server/php-worker.h"

void php_worker_run_mc_query_packet(php_worker *worker, php_net_query_packet_t *query);
extern conn_target_t memcache_ct;
