#ifndef ENGINE_KFS_TYPEDEFS_H
#define ENGINE_KFS_TYPEDEFS_H

typedef struct kfs_replica kfs_replica_t;
typedef struct kfs_replica *kfs_replica_handle_t;
typedef struct kfs_file_info kfs_file_info_t;
typedef struct kfs_file *kfs_file_handle_t;
typedef struct kfs_binlog_zip_header kfs_binlog_zip_header_t;

typedef unsigned long long kfs_hash_t;	// all kfs hashes MUST be non-zero

#endif // ENGINE_KFS_TYPEDEFS_H
