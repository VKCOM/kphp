// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/server/engine-settings.h"

void send_data_to_statsd_with_prefix(const char *stats_prefix, unsigned int tag_mask);

