// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/binlog/kdb-binlog-common.h"

#pragma pack(push, 4)

enum {
  LEV_PMEMCACHED_EVENT_ENUMERATORS_START = __LINE__,
  LEV_PMEMCACHED_DELETE = 0x1ace7893,
  LEV_PMEMCACHED_STORE = 0x27827d00,
  LEV_PMEMCACHED_STORE_FOREVER = 0x29aef200,
  LEV_PMEMCACHED_APPEND = 0x4b7ece38,
  LEV_PMEMCACHED_GET = 0x3a789adb,
  LEV_PMEMCACHED_INCR = 0x4fe23098,
  LEV_PMEMCACHED_INCR_TINY = 0x5ac40900,
  LEV_PMEMCACHED_TOUCH = 0x5e69e046,
  LEV_PMEMCACHED_EVENT_ENUMERATORS_COUNT = __LINE__ - LEV_PMEMCACHED_EVENT_ENUMERATORS_START - 1
};

enum {
  PMCT_STORE_EVENT_ENUMERATORS_START = __LINE__,
  pmct_add = 0,
  pmct_set = 1,
  pmct_replace = 2,
  PMCT_STORE_EVENT_ENUMERATORS_COUNT = __LINE__ - PMCT_STORE_EVENT_ENUMERATORS_START - 1
};

// Be careful when add/remove binlog event type: check both pmemcached and KPHP server
static_assert(LEV_PMEMCACHED_EVENT_ENUMERATORS_COUNT == 8, "Got unexpected event, check comment below");
static_assert(PMCT_STORE_EVENT_ENUMERATORS_COUNT == 3, "Got unexpected store event, check comment below");

// binlog structures
struct lev_pmemcached_delete {
  lev_type_t type;
  short key_len;
  char key[1];
};

struct lev_pmemcached_store {
  lev_type_t type;
  short key_len;
  short flags;
  int data_len;
  int delay;
  char data[1]; // the first part contains bytes from key, the second - value for this key
};

struct lev_pmemcached_store_forever {
  lev_type_t type;
  short key_len;
  int data_len;
  char data[1]; // the first part contains bytes from key, the second - value for this key
};

struct lev_pmemcached_append {
  lev_type_t type;
  int key_len;
  int flags;
  int data_len;
  int delay;
  char data[0];
};

struct lev_pmemcached_get {
  lev_type_t type;
  long long hash;
  short key_len;
  char key[1];
};

struct lev_pmemcached_incr {
  lev_type_t type;
  long long arg;
  short key_len;
  char key[1];
};

struct lev_pmemcached_incr_tiny {
  lev_type_t type;
  short key_len;
  char key[1];
};

struct lev_pmemcached_touch {
  lev_type_t type;
  int key_len;
  int delay;
  char key[0];
};

#pragma pack(pop)

constexpr auto PMEMCACHED_OLD_INDEX_MAGIC = 0x53407fa0;
constexpr auto PMEMCACHED_INDEX_RAM_MAGIC_G3 = 0x65049e9e;
constexpr auto BARSIC_SNAPSHOT_HEADER_MAGIC = 0x1d0d1b74;
constexpr auto TL_ENGINE_SNAPSHOT_HEADER_MAGIC = 0x4bf8b614;

// snapshot structures
typedef struct {
/* strange numbers */
  int magic;
  int created_at;
  long long log_pos0;
  long long log_pos1;
  unsigned long long file_hash;
  int log_timestamp;
  unsigned int log_pos0_crc32;
  unsigned int log_pos1_crc32;
  int num_records;
} index_header;

struct index_entry {
  enum {
    PMEMCACHED_FLAG_COMPRESSED = 1 << 1,
    PMEMCACHED_FLAG_ERASED = 1 << 2
  };

  short key_len;
  short flags;
  int data_len;
  int delay;
  char data[0];
};
