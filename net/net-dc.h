// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/cdefs.h>

#include "net/net-connections.h"

int is_same_data_center(struct connection *c, int is_client);
bool add_dc_by_ipv4_config(const char *index_ipv4_subnet);
void print_not_same_dc_log(const char *prefix, connection *c);
