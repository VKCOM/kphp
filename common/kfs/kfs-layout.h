// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __KFS_LAYOUT_INCLUDED__
#define	__KFS_LAYOUT_INCLUDED__

#include <assert.h>

#include "common/kfs/kfs-typedefs.h"

#pragma	pack(push,4)

#define	KFS_HEADER_SIZE	4096

#define	KFS_MAGIC	0x0473664b
#define	KFS_V01		1

#define KFS_BINLOG_ZIP_MAGIC 0x047a4c4b

/* 0x10000000000LL = 1 << 40 */
#define KFS_BINLOG_ZIP_MAX_FILESIZE 0x10000000000LL
#define KFS_BINLOG_ZIP_CHUNK_SIZE_EXP 24
#define KFS_BINLOG_ZIP_CHUNK_SIZE (1<<KFS_BINLOG_ZIP_CHUNK_SIZE_EXP)
#define KFS_BINLOG_ZIP_MAX_ENCODED_CHUNK_SIZE ((1<<KFS_BINLOG_ZIP_CHUNK_SIZE_EXP) + (1<<(KFS_BINLOG_ZIP_CHUNK_SIZE_EXP-4)))
#define KFS_BINLOG_ZIP_FORMAT_FLAG_HAS_TAG 0x01000000

enum kfs_file_type {
  kfs_unknown = 0,
  kfs_binlog = 1,
  kfs_snapshot = 2,
  kfs_partial = 3,	// for partial snapshots, usually kept while reindexing
  kfs_temp = 4,		// for temporary files
  kfs_aux = 5,		// auxiliary files: debug info, statistics etc.
  kfs_enc = 6
};

typedef enum {
  kfs_bzf_zlib   = 0,
  kfs_bzf_bz2    = 1,
  kfs_bzf_xz     = 2
} kfs_bz_format_t;

struct kfs_file_header {
  int magic;		// (F) KFS_MAGIC
  int kfs_version;	// (F) KFS_V01 = 1
  int kfs_file_type;	// (F) one of enum kfs_file_type
  int modified;	// (H) unixtime of most recent header modification (if two headers with correct crc32 are present, only most recent is considered)
  int header_seq_num;	// (H) header modification timestamp: starts with 1, then is increased before each header modification (similar to 'modified', but safer)
  int finished;		// (F) unixtime of final modification, 0=being modified, -1=being constructed
  unsigned data_crc32;	// (F) crc32 of data_size bytes (for binlog/snapshot integrity checks; for binlog cumulative starting from start_crc32)
  int slice_status;	// (F) describes slice status when this log/snapshot was created
  kfs_hash_t file_id_hash;	// (F) identifies copies of same file: file_id_hash must differ if files are supposed to be different
  kfs_hash_t replica_id_hash;	// (S) identifies files (logs, snapshots,...) related to same slice/table
  kfs_hash_t slice_id_hash;	// (SR) same for all replicas of this slice (original slice_id_hash)
  kfs_hash_t table_id_hash;	// (T) identifies table (all files of all slices share same table_id_hash)
  long long data_size;	// (F) file size without kfs headers (if file grows, can be obsolete)
  long long raw_size;	// (F) file size with (all) kfs headers

  int created;		// (F) unixtime (for this file)
  int created_by;	// (F) server id or ip
  int replica_created;	// (S) unixtime (for the first file of this replica)
  int replica_created_by; // (S) must coincide with current server id or ip, otherwise new replica_id_hash must be generated
  int slice_created;	// (SR) unixtime (for the first file of this slice or the original master of this slice)
  int slice_created_by; // (SR)

  int checkpoint_id;	// (F=) for snapshots required by a LEV_SNAPSHOT
  int checkpoint_date;	// (F=) for snapshots required by a LEV_SNAPSHOT; else 0

  int total_copies;	// (C) # of known copies of this file in different data dirs
  int copy_id;		// (C) 0 .. total_copies-1; 0 = main copy
  int copy_state;	// (C) 0 = ok, else broken (error codes: 1 = incomplete; 32 = media error)
  int copy_importance;	// (C) usually 0; used to mark a good copy when another copy fails; copy with maximal value of copy_importance will be chosen as 'main', if several, then the copy with minimal copy_id; if several, then with maximal file size and minimal ctime

  int last_log_time;	// (F) last known timestamp (only for snapshots and possibly complete binlogs)
  int last_log_mtime;	// (F) last known modification timestamp (idem)

  kfs_hash_t prev_log_hash;	// file hash of previous binlog
  int prev_log_time;	// (F) binlogs: timestamp of rotate log pos; snapshot: same as first_snapshot_*

  int snapshot_flags;
  kfs_hash_t log_hash;	// for snapshot: log file hash & position
  long long log_pos;	// for snapshot: (absolute) log position; for binlog: (absolute) log position of start of this binlog

  int first_snapshot_time;	// binlog point timestamps!
  int last_snapshot_time;

  kfs_hash_t first_snapshot_hash;  // for binlogs: first associated snapshot; for snapshots: snapshot used to generate this one
  long long first_snapshot_pos;    // absolute log position!
  long long first_snapshot_size;   // optional (may be 0)

  kfs_hash_t last_snapshot_hash;   // for binlogs: last associated snapshot; for snapshots: most recent snapshot before this one
  long long last_snapshot_pos;
  long long last_snapshot_size;

  unsigned log_pos_crc32;		// (SL) accumulated log data crc32 corresponding to log_pos (same as start_crc32 for log files)

  int reserved_ints[31];
  
  char table_name[64];	// (T) string with (printable) table name (e.g. 'table')
  char slice_name[64];	// (S) string with (printable) slice name (e.g. 'table17')
  char filename[32];	// (S) recommended filename; if none, slice_name might be used; seq_num and appropriate suffix might be appended if necessary (e.g. 'table17' -> 'table17.bin.000239')
  char creator[32];	// (F) program which created this file
  char slice_creator[32];	// (S) program which created this slice

  int split_method;	// (S) external split method (if needed; it cannot affect schema itself)
  int schema_id;	// (S) schema id - same as in binlog, determines binlog/snapshot layout
  int extra_bytes;	// (S) additional bytes used in schema_extra to determine schema
  int split_mod;
  int split_min;
  int split_max;
  char schema_extra[512];	// (S) additional schema parameters (e.g. field names)

  char custom[1024];
  char tag[16];
  char reserved[1940];
  unsigned header_crc32;
};
static_assert(sizeof(struct kfs_file_header) == 4096, "mmm");

struct kfs_binlog_zip_header {
  int magic;
  int format;
  long long orig_file_size;
  unsigned char orig_file_md5[16];
  char first36_bytes[36];
  char last36_bytes[36];
  unsigned char first_1m_md5[16];
  unsigned char first_128k_md5[16]; // tag also can be stored here for first slice of binlog
  unsigned char last_128k_md5[16];
  unsigned long long file_hash;
  long long chunk_offset[0];
};
static_assert(sizeof(kfs_binlog_zip_header_t) == 160, "mm");

#pragma	pack(pop)

#endif
