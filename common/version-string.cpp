// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef VKEXT
  #include "common/options.h"
  #include "common/stats/provider.h"
#endif

#ifndef COMMIT
  #define COMMIT "unknown"
#endif

#ifndef BUILD_TIMESTAMP
  #define BUILD_TIMESTAMP 0
#endif

static char FullVersionStr[400];

void init_version_string(const char* version) {
  time_t build_time = BUILD_TIMESTAMP;
  static char date_str[100];
  if (build_time) {
    strftime(date_str, sizeof(date_str), "%b %e %Y %T %Z", localtime(&build_time));
  } else {
    strcpy(date_str, "unknown time");
  }
  snprintf(FullVersionStr, sizeof(FullVersionStr) - 1,
           "%s compiled at %s by gcc " __VERSION__ " "
#ifdef __LP64__
           "64-bit"
#else
           "32-bit"
#endif
           " after commit " COMMIT,
           version, date_str);
}

const char* get_version_string() {
  return FullVersionStr;
}

#ifndef VKEXT

STATS_PROVIDER(version_string, 10000) {
  stats->add_general_stat("version", "%s", FullVersionStr);
  stats->add_histogram_stat("build_timestamp", static_cast<long long>(BUILD_TIMESTAMP));
}

OPTION_PARSER(OPT_GENERIC, "version", no_argument, "prints version and exits") {
  printf("%s\n", get_version_string());
  exit(0);
}

#endif
