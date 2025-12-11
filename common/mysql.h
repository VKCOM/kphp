// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#define MYSQL_COM_INIT_DB 2
#define MYSQL_COM_QUERY 3
#define MYSQL_COM_PING 14
#define MYSQL_COM_BINLOG_DUMP 18
#define MYSQL_COM_REGISTER_SLAVE 21

#define cp1251_general_ci 51

#pragma pack(push, 1)

struct mysql_auth_packet_end {
  int thread_id;
  char scramble1[8];
  char filler;
  short server_capabilities;
  char server_language;
  short server_status;
  short server_capabilities2;
  unsigned char proto_len;
  char filler2[10];
  char scramble2[13];
  char proto[0];
};

#pragma pack(pop)

/* for sqls_data.auth_state */

enum sql_auth { sql_noauth, sql_auth_sent, sql_auth_initdb, sql_auth_ok };
