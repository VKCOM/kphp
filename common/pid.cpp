// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/pid.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "net/net-ifnet.h"

process_id_t PID;

void init_common_PID () {
  if (!PID.pid) {
    PID.pid = getpid ();
  }
  if (!PID.utime) {
    PID.utime = time (0);
  }
}

void init_client_PID (unsigned ip) {
  if (ip && (ip != 0x7f000001)) {
    PID.ip = ip;
  }
  // PID.port = 0;
  init_common_PID ();
};

void init_server_PID (unsigned ip, int port) {
  if (ip && (ip != 0x7f000001)) {
    PID.ip = ip;
  }
  if (!PID.port) {
    PID.port = port;
  }
  init_common_PID ();
};

void reset_PID() {
  memset(&PID, 0, sizeof(PID));
}

pid_match_t matches_pid (const process_id_t *X, const process_id_t *Y) {
  if (!memcmp (X, Y, sizeof (process_id_t))) {
    return full_pid_match;
  } else if ((!Y->ip || X->ip == Y->ip) && (!Y->port || X->port == Y->port) && (!Y->pid || X->pid == Y->pid) && (!Y->utime || X->utime == Y->utime)) {
    return partial_pid_match;
  } else {
    return no_pid_match;
  }
}
