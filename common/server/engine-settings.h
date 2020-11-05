// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <arpa/inet.h>
#include <stdbool.h>

#include "common/crc32c.h"

struct lev_generic;
struct lev_start;

typedef struct {
  /**
   * This function are called when data is read from binlog. Size is number of bytes ready to process and E is a pointer to data.
   * Function should return the real size of next event or REPLAY_BINLOG_NOT_ENOUGH_DATA.
   */
  int (*replay_logevent)(const struct lev_generic *E, int size);

  /**
   * There is a default LEV_START event handler, which sets global variables log_split_{min, max, mod}.
   * If you want to process LEV_START event in a different way, this callback should be set.
   */
  void (*on_lev_start)(const struct lev_start *E);

  /**
   * This function is called to load index. It is opened as Snapshot global var.
   * Snapshot var will be null if no index found.
   * Should return negative value on failure.
   * You need to setup jump_log_{pos,crc32,ts} global vars inside.
   */
  int (*load_index)();

  /**
   * By default engine tries to connect to statsd at ports {8125, 14880} and send stats there
   * Set this option to true to disable it
   */
  bool do_not_use_default_statsd_ports;

  /**
   * engine name displayed in version in logs and stats
   */
  const char *name;

} engine_settings_t;

void set_engine_settings(engine_settings_t *settings);
engine_settings_t *get_engine_settings();

// normally, this shouldn't be set from engine
// this is a way to avoid linkage errors,
// when some part like kfs is not linked
typedef struct {
  const char* (*char_stats)();
} engine_settings_handlers_t;

extern engine_settings_handlers_t engine_settings_handlers;


