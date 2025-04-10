// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __KDB_BINLOG_COMMON_H__
#define __KDB_BINLOG_COMMON_H__

#include <stdbool.h>

#pragma pack(push, 4)
/* if a KFS header is present, first four bytes will contain KFS_FILE_MAGIC; then 4Kbytes have to be skipped */

#ifndef KFS_FILE_MAGIC
  #define KFS_FILE_MAGIC 0x0473664b
#endif

#define DEFAULT_MAX_BINLOG_SIZE ((1LL << 30) - (1LL << 20))

#ifndef hash_t
typedef unsigned long long hash_t;
  #define hash_t hash_t
#endif

typedef unsigned int lev_type_t;

// first entry in any binlog file: sets schema, but doesn't clear data if occurs twice
#define LEV_START 0x044c644b
// for alignment
#define LEV_NOOP 0x04ba3de4
// timestamp (unix time) for all following records
#define LEV_TIMESTAMP 0x04d931a8
// must be immediately after LEV_START; defines a 16-byte `random tag` for this virtual binlog
#define LEV_TAG 0x04476154
// usually the second entry in a binlog file, unless this binlog file is the very first
#define LEV_ROTATE_FROM 0x04724cd2
// almost last entry, output before switching to a new binlog file; LEV_CRC32 and LEV_END might follow
#define LEV_ROTATE_TO 0x04464c72
// stores crc32 up to this point
#define LEV_CRC32 0x04435243
// delayed alteration of extra fields bitmask
#define LEV_CHANGE_FIELDMASK_DELAYED 0x04546c41
// end of data in cbinlog
#define LEV_CBINLOG_END 0x04644e65

#define LEV_ALIGN_FILL 0xfc

#define LEV_SET_PERSISTENT_CONFIG_VALUE 0xe133cd0d
#define LEV_SET_PERSISTENT_CONFIG_ARRAY 0xe375d4a8

struct lev_set_persistent_config_value {
  lev_type_t type;
  int name_len;
  int value;
  char name[0];
};

struct lev_set_persistent_config_array {
  lev_type_t type;
  int array_len;
  int name_len;
  int extra[0];
};

struct lev_generic {
  lev_type_t type;
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
};
typedef struct lev_generic lev_generic_t;

typedef struct lev_start {
  constexpr static lev_type_t MAGIC = LEV_START;

  int get_extra_bytes() const {
    return extra_bytes - static_cast<int>(sizeof(str));
  }

  lev_type_t type;
  int schema_id;
  int extra_bytes;
  int split_mod;
  int split_min;
  int split_max;
  char str[4]; // extra_bytes, contains: [\x01 char field_bitmask] [<table_name> [\t <extra_schema_args>]] \0
} lev_start_t;

struct lev_timestamp {
  lev_type_t type;
  int timestamp;
};

struct lev_crc32 {
  lev_type_t type;
  int timestamp;  // timestamp (serves as a LEV_TIMESTAMP)
  long long pos;  // position of the beginning of this record in the log
  unsigned crc32; // crc32 of all data in file up to the beginning of this record
};

struct lev_rotate_from {
  lev_type_t type;
  int timestamp;
  long long cur_log_pos;
  unsigned crc32;
  hash_t prev_log_hash;
  hash_t cur_log_hash;
};

struct lev_rotate_to {
  lev_type_t type;
  int timestamp;
  long long next_log_pos;
  unsigned crc32;
  hash_t cur_log_hash;
  hash_t next_log_hash;
};

struct lev_tag {
  lev_type_t type;
  unsigned char tag[16];
};

/* binlogs */

extern int log_split_min, log_split_max, log_split_mod;

int lev_start_logrec_size(const lev_start_t* E, int size);
int default_replay_logevent(const struct lev_generic* E, int size);

typedef int (*replay_logevent_vector_t)(const struct lev_generic* E, int size);
extern replay_logevent_vector_t replay_logevent;

extern double binlog_load_time;
extern long long jump_log_pos;
extern int jump_log_ts;
extern unsigned jump_log_crc32;

enum replay_binlog_result {
  REPLAY_BINLOG_OK = 0,
  REPLAY_BINLOG_ERROR = -1,
  REPLAY_BINLOG_NOT_ENOUGH_DATA = -2,
  REPLAY_BINLOG_STOP_READING = -13,
  REPLAY_BINLOG_WAIT_JOB = -14,
};

#pragma pack(pop)

#endif
