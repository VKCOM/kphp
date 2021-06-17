// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "net/net-connections.h"
#include "server/workers-control.h"

WorkerType start_master(int *http_fd, int (*try_get_http_fd)(), int http_fd_port);
