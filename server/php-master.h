// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "net/net-connections.h"
#include "server/workers-control.h"

DECLARE_VERBOSITY(master_process);

WorkerType start_master();
