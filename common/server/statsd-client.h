#pragma once

#include "common/server/engine-settings.h"

void send_data_to_statsd_with_prefix(const char *stats_prefix, unsigned int tag_mask);

