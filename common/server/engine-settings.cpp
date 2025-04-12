// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/engine-settings.h"

#include <assert.h>

#include "common/options.h"
#include "common/version-string.h"

static engine_settings_t* engine_settings;
engine_settings_handlers_t engine_settings_handlers;

#define DEFAULT_EPOLL_WAIT_TIMEOUT 37

void set_engine_settings(engine_settings_t* settings) {
  assert(settings->name);
  init_version_string(settings->name);
  engine_settings = settings;
}

engine_settings_t* get_engine_settings() {
  return engine_settings;
}
