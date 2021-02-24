// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __KDB_PID_H__
#define __KDB_PID_H__

#include <sys/cdefs.h>

#pragma pack(push,4)

struct process_id {
  unsigned ip;
  short port;
  short pid;
  int utime;
};
typedef struct process_id process_id_t;

#pragma pack(pop)

extern process_id_t PID;

void init_common_PID ();
void init_client_PID (unsigned ip);
void init_server_PID (unsigned ip, int port);
void reset_PID();

enum pid_match { no_pid_match, partial_pid_match, full_pid_match };
typedef enum pid_match pid_match_t;

pid_match_t matches_pid (const struct process_id *X, const struct process_id *Y);

inline pid_match_t matches_pid_ref (const process_id_t &X, const process_id_t &Y) {
  return matches_pid(&X, &Y);
}

#endif
