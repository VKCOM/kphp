#pragma once

#include "net/net-connections.h"
#include "net/net-msg.h"

int master_rpc_tl_execute(connection *c, int op, raw_message_t *raw);
