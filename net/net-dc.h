#pragma once

#include <sys/cdefs.h>

#include "net/net-connections.h"

int is_same_data_center(struct connection *c, int is_client);
bool add_dc_by_ipv4_config(const char *index_ipv4_subnet);
