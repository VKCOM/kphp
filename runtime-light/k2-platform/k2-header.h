#ifndef K2_PLATFORM_HEADER_H
#define K2_PLATFORM_HEADER_H

#ifndef K2_API_HEADER_H
#error "should not be directly included"
#endif // K2_API_HEADER_H

#pragma once

#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>

#ifdef __cplusplus
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#else
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#endif

#define K2_PLATFORM_HEADER_H_VERSION 16

// Always check that enum value is a valid value!

#ifdef __cplusplus
extern "C" {
#endif

enum IOStatus {
  IOAvailable = 0,
  IOBlocked = 1,
  IOClosed = 2,
};

struct StreamStatus {
  enum IOStatus read_status;
  enum IOStatus write_status;
  int32_t please_shutdown_write;
  int32_t libc_errno;
};

/*
 * This time point is valid only within the component.
 * Similar to c++ `std::chrono::steady_clock::time_point`
 */
struct TimePoint {
  uint64_t time_point_ns;
};

struct SystemTime {
  uint64_t since_epoch_ns;
};

#ifdef __cplusplus
typedef std::atomic_uint32_t atomic_uint32_t;
#else
typedef _Atomic(uint32_t) atomic_uint32_t;
#endif

struct InstanceState;

struct ComponentState;

/*
 * Image state created once on library load.
 * Shared between all component [instances].
 * Designed to prevent heavy `_init` section of dlib
 */
struct ImageState;

enum PollStatus {
  // waitings IO or timer; no cpu work remains
  PollBlocked = 0,
  // there is some cpu work to do; platform will reschedule component
  PollReschedule = 1,
  // component decide to shutdown normally
  PollFinishedOk = 2,
  // component decide to shutdown unexpectedly
  PollFinishedError = 3,
};

enum UpdateStatus {
  // no updates to take, provided descriptor will be assigned to 0
  NoUpdates = 0,
  // platform has update on existed stream
  UpdateExisted = 1,
  // new connection has installed, new descriptor is provided
  NewDescriptor = 2,
};

enum CipherMode : uint32_t { CIPHER_MODE_GCM = 0x6, CIPHER_MODE_CCM = 0x7 };

enum RsaPadding : uint32_t {
  RSA_PADDING_PKCS1 = 1,
  RSA_PADDING_SSLV23 = 2,
  RSA_PADDING_NONE = 3,
  RSA_PADDING_PKCS1_OAEP = 4,
  RSA_PADDING_X931 = 5,
  RSA_PADDING_PKCS1_PSS = 6
};

enum HashAlgorithm : uint32_t {
  HASH_SHA1 = 0,
  HASH_SHA224 = 1,
  HASH_SHA256 = 2,
  HASH_SHA384 = 3,
  HASH_SHA512 = 4,
  HASH_MD5 = 5,
  HASH_RMD160 = 6,
  HASH_MD2 = 7,
  HASH_MD4 = 8,
  COUNT_HASH
};

enum DigestAlgorithm : uint32_t {
  DIGEST_DSS1 = 0,
  DIGEST_SHA1 = 1,
  DIGEST_SHA224 = 2,
  DIGEST_SHA256 = 3,
  DIGEST_SHA384 = 4,
  DIGEST_SHA512 = 5,
  DIGEST_MD5 = 6,
  DIGEST_RMD160 = 7,
  DIGEST_MD2 = 8,
  DIGEST_MD4 = 9,
  COUNT_DIGEST
};

enum RpcKind {
  TL_RPC = 0,
};

// k2-node will attempt to extract `compiler_version` from `extra_info` to add as a tag to the `k2_image_version` metric
struct ImageInfo {
  // Base
  const char* image_name;
  uint8_t is_oneshot;

  // DSO specific
  uint64_t build_timestamp;
  uint64_t header_h_version;
  uint8_t commit_hash[40];
  const char* version;

  // Extra
  struct KeyValuePair {
    const char* key;
    const char* value;
  };

  size_t extra_info_size;
  const struct KeyValuePair* extra_info;
};

/**
 * Symbols provided by component image
 * Every image should provide these symbols
 */
enum PollStatus k2_poll();

/**
 * Performs component-specific warmup work before the instance starts handling
 * requests.
 *
 * The platform calls this function repeatedly during scope warmup until it
 * returns either `PollFinishedOk` or `PollFinishedError`. Between calls the
 * component may use timers, streams, and other async descriptors exactly as
 * during normal `k2_poll` execution.
 *
 * Lifecycle semantics:
 * - For multishot components, `k2_warmup` operates on the same `InstanceState`
 *   that will later handle requests. Any state prepared here is preserved.
 * - For oneshot components, `k2_warmup` runs against a single throwaway
 *   instance that is created, warmed, and discarded before the first real
 *   request. The warmup is performed exactly once.
 *
 * Return values reuse `enum PollStatus` with the same meaning as `k2_poll`:
 * - `PollBlocked` / `PollReschedule` — warmup is not complete; the platform
 *   will call `k2_warmup` again.
 * - `PollFinishedOk` — warmup succeeded and the instance is considered ready.
 * - `PollFinishedError` — warmup failed; the instance will not receive
 *   requests and scope warmup will fail.
 *
 * Components with no warmup work should return `PollFinishedOk` immediately.
 */
enum PollStatus k2_warmup();

// Symbols provided by .so.
struct ImageState* k2_create_image();
void k2_init_image();
struct ComponentState* k2_create_component();
void k2_init_component();
struct InstanceState* k2_create_instance();
void k2_init_instance();

const struct ImageInfo* k2_describe();

struct ControlFlags {
  atomic_uint32_t please_yield;
  atomic_uint32_t please_graceful_shutdown;
};

/**
 *
 * Symbols provided by k2-node and resolved by linker during dlopen.
 * Every image can use these symbols.
 *
 * Semantically analogs for `PlatformCtx` methods sometimes with small
 * differences. Preferable way to communication with k2-node.
 *
 */
struct ControlFlags* k2_control_flags();
struct ImageState* k2_image_state();
struct ComponentState* k2_component_state();
struct InstanceState* k2_instance_state();

/**
 * If component track alignment and allocated memory size -
 * it is preferable to use the `*_checked` versions, which have additional checks enabled.
 * The rest of the functions are allowed to be used in any case.
 */
void* k2_alloc(size_t size, size_t align);
void* k2_realloc(void* ptr, size_t new_size);
void* k2_realloc_checked(void* ptr, size_t old_size, size_t align, size_t new_size);
void k2_free(void* ptr);
void k2_free_checked(void* ptr, size_t size, size_t align);

/**
 * Shared memory provides a mechanism for instances to share data.
 * To use it, first allocate memory with `k2_alloc_shared_memory`, then publish
 * it with a unique name using `k2_publish_shared_memory`. Other instances can
 * then retrieve the memory by name with `k2_get_shared_memory`.
 *
 * Lifecycle:
 * - TTL defines when memory becomes eligible for reclamation
 * - Memory is reclaimed lazily after TTL expires AND no references remain
 *
 * Reference counting:
 * - Calling `k2_publish_shared_memory` sets the reference count to one
 * - Calling `k2_get_shared_memory` increments the reference count by one if
 *   this instance does not already hold a reference; repeated calls for the
 *   same underlying pointer while already held do not increment it further
 * - Calling `k2_release_shared_memory` clears the held reference, if `pointer`
 *   is currently held; unpublished memory from `k2_alloc_shared_memory` is
 *   just freed
 */

/**
 * Allocates shared memory with specified size and alignment.
 *
 * The actual alignment of the returned pointer is `max(align, system_page_size)`.
 *
 * @param `size` Size of the memory region to allocate in bytes.
 * @param `align` Alignment requirement for the memory region in bytes. Must be
 *                a power of two.
 * @param `pointer` Return argument. On success (return code is 0), contains
 *                  pointer to the allocated memory. Set to NULL on error.
 *
 * @return `0` on success. libc-like `errno` on error.
 *
 * Possible `errno`:
 * `EINVAL` => `size` is 0, `align` is not a power of 2, or `pointer` is NULL.
 * `ENOMEM` => Not enough memory to allocate the requested region.
 * `ENOSYS` => Shared memory subsystem is unavailable on this host.
 */
int32_t k2_alloc_shared_memory(size_t size, size_t align, void** pointer);

/**
 * Publishes shared memory with a name and TTL, making it discoverable by other instances.
 *
 * @param `name` Name to associate with the memory region. Must be unique.
 *               Should be valid UTF-8 and not contain null bytes.
 * @param `name_len` Length of the name in bytes. Must be greater than 0.
 * @param `memory` Pointer to memory previously allocated via `k2_alloc_shared_memory`.
 * @param `ttl` Time-to-live in milliseconds. Memory becomes eligible for
 *              reclamation after this duration, but only when the reference
 *              count reaches zero. Zero TTL means infinite life.
 * @param `as_mut` Controls whether write protection is disabled. If true, the memory is made writable.
 * @param `ignore_if_exist` If true, allows re-publishing memory previously allocated via
 *              `k2_alloc_shared_memory` even if a live published memory with the same name already exists.
 *
 * @return `0` on success. libc-like `errno` on error.
 *
 * Possible `errno`:
 * `EINVAL` => `name` is NULL, `name_len` is 0, `memory` is NULL, `name` is
 *             not valid UTF-8, or `ttl` is too small (non-zero and less than
 *             100 ms).
 * `ENOENT` => `memory` was not allocated by `k2_alloc_shared_memory`.
 * `EEXIST` => Memory with this name already exists.
 * `ENOSYS` => Shared memory subsystem is unavailable on this host.
 */
int32_t k2_publish_shared_memory(const char* name, size_t name_len, const void* memory, uint64_t ttl, bool as_mut, bool ignore_if_exist);

/**
 * Retrieves shared memory by name and, unless already held by this
 * instance, acquires a reference to it (see the reference counting notes
 * above).
 *
 * @param `name` Name of the published memory region to retrieve.
 * @param `name_len` Length of the name in bytes. Must be greater than 0.
 * @param `pointer` Return argument. On success (return code is 0), contains
 *                  pointer to the shared memory. Set to NULL on error.
 *
 * @return `0` on success. libc-like `errno` on error.
 *
 * Possible `errno`:
 * `EINVAL` => `name` is NULL, `name_len` is 0, `pointer` is NULL, or `name`
 *             is not valid UTF-8.
 * `ENOENT` => No memory found with the given name (or TTL expired and memory was freed).
 * `ENOSYS` => Shared memory subsystem is unavailable on this host.
 */
int32_t k2_get_shared_memory(const char* name, size_t name_len, const void** pointer, size_t* size);

/**
 * Republishes shared memory, resetting its TTL and restarting the age counter from this call.
 *
 * @param `name` Name of the published memory region.
 * @param `name_len` Length of the name in bytes. Must be greater than 0.
 * @param `ttl` New time-to-live in milliseconds, counted from this call.
 *              Zero TTL means infinite life.
 *
 * @return `0` on success. libc-like `errno` on error.
 *
 * Possible `errno`:
 * `EINVAL` => `name` is NULL, `name_len` is 0, `name` is not valid UTF-8, or
 *             `ttl` is too small (non-zero and less than 100 ms).
 * `ENOENT` => No memory found with the given name (or TTL expired and memory was freed).
 * `ENOSYS` => Shared memory subsystem is unavailable on this host.
 */
int32_t k2_republish_shared_memory(const char* name, size_t name_len, uint64_t ttl);

/**
 * Fast-forwards the expiration of published shared memory ahead of its TTL, by name.
 * Its expiration is recomputed so that, at the moment of this call, the entry's age
 * already amounts to `percentile` of its (recomputed) TTL.
 *
 * Timeline right after the call, where `S` = `stored_at`, `C` = the current instant and `E` = the recomputed `expires_at`:
 *
 *   S                                   C        E
 *   |-----------------------------------|--------|
 *   |<---------- age = C - S ---------->|
 *   |<-------------- ttl = E - S --------------->|
 *
 * i.e. `age / ttl == percentile / 100`. The higher `percentile` is, the shorter the
 * remaining slice `E - C` is (closer to `100` puts `E` right next to `C`, expiring almost immediately);
 * the lower it is, the further away `E` ends up.
 * `remaining_lifetime_limit` additionally clamps `E` to no later than `C + remaining_lifetime_limit`, regardless of `percentile`, when non-zero.
 *
 * @param `name` Name of the published memory region to seek.
 * @param `name_len` Length of the name in bytes. Must be greater than 0.
 * @param `percentile` Target percentile (entry age / TTL * 100) to seek the entry to,
 *              in `[0, 100]`. E.g. `80` makes the entry appear 80% of the way through
 *              its lifetime as of this call.
 * @param `remaining_lifetime_limit` Upper bound, in milliseconds, on how much longer the entry may
 *              live from this call, regardless of `percentile`. Zero means no limit.
 *
 * @return `0` on success. libc-like `errno` on error.
 *
 * Possible `errno`:
 * `EINVAL` => `name` is NULL, `name_len` is 0, `name` is not valid UTF-8, or
 *             `percentile` is outside of `[0, 100]`.
 * `ENOENT` => No memory found with the given name (or TTL expired and memory was freed).
 * `ENOSYS` => Shared memory subsystem is unavailable on this host.
 */
int32_t k2_seek_ttl_to_shared_memory(const char* name, size_t name_len, uint8_t percentile, uint64_t remaining_lifetime_limit);

/**
 * Frees shared memory allocated via `k2_alloc_shared_memory` that was never
 * published, or releases the reference to shared memory held by this
 * instance, acquired via `k2_publish_shared_memory` or
 * `k2_get_shared_memory` (see the reference counting notes above).
 *
 * Releasing a reference does not free the underlying memory immediately: the
 * memory is only physically reclaimed once its TTL has expired AND the
 * reference count has reached zero (see the lifecycle notes above).
 *
 * @param `pointer` Pointer to shared memory, as returned by
 *                  `k2_alloc_shared_memory` or `k2_get_shared_memory`, or
 *                  passed as the `memory` argument to `k2_publish_shared_memory`.
 *
 * @return `0` on success: either the unpublished allocation was freed, or the
 *         reference held by this instance was released. libc-like `errno`
 *         on error.
 *
 * Possible `errno`:
 * `EINVAL` => `pointer` is NULL.
 * `ENOENT` => `pointer` does not correspond to memory allocated by this
 *             instance via `k2_alloc_shared_memory`, nor to a reference
 *             currently held by this instance.
 */
int32_t k2_release_shared_memory(const void* pointer);

/**
 * Immediately abort component execution.
 * Function is `[[noreturn]]`
 * Note: `exit_code` used just as indicator for now.
 * `exit_code` == 0 => `FinishedOk`,
 * `exit_code` != 0 => `FinishedError`,
 */
[[noreturn]] void k2_exit(int32_t exit_code);

/**
 * Semantically equivalent to libc's `sysconf` function.
 *
 * Possible 'errno':
 * `EINVAL` => name is invalid.
 */
int64_t k2_sysconf(int32_t name);

/**
 * `k2_getpid` returns the process ID (PID) of the calling process.
 */
uint32_t k2_getpid();

/**
 *  Semantically equivalent to libc's `getuid` function.
 */
uid_t k2_getuid();

/**
 * Semantically equivalent to libc's `getpwuid_r` function.
 *
 * Possible `errno`:
 * `EIO` => An I/O error has occurred.
 * `EINTR` => A signal was caught during `getpwuid()`.
 * `EMFILE` => All file descriptors available to the process are currently open.
 * `ERANGE` => Insufficient storage was supplied via buffer and bufsize to contain the data to be referenced by the resulting passwd structure.
 */
int32_t k2_getpwuid_r(uid_t uid, struct passwd* pwd, char* buf, size_t buflen, struct passwd** result);

/**
 * Semantically equivalent to libc's `uname` function.
 *
 * Possible `errno`:
 * `EFAULT` => buf is not valid
 */
int32_t k2_uname(struct utsname* buf);

/**
 * @return return `0` on success. libc-like `errno` otherwise
 * `stream_d` will be assigned new descriptor on success.
 * `stream_d` will be assigned `0` on error. however `stream_d=0` itself is not an error marker.
 *
 * Some `errno` examples:
 * `EINVAL` => `name` has invalid format (empty, non ascii, too long, etc..)
 * `ECONNREFUSED` => no component with such name found
 * `ENOMEM` => our component has no enough memory for stream
 * `EACCES` => permission denied
 */
int32_t k2_open(uint64_t* stream_d, size_t name_len, const char* name);

/**
 * Open a file with specified `mode`.
 *
 * @return return `0` on success. libc-like `errno` otherwise.
 * `fd` will be assigned new descriptor on success.
 * `fd` will be assigned `0` on error. However, `fd=0` itself is not an error marker.
 *
 * Some `errno` examples:
 * `EINVAL` => `pathname` or `mode` has invalid format (empty, too long, etc...).
 * `EILSEC` => `mode` is not a valid UTF-8 string.
 * Other => any error returned by libc's `fopen` function.
 */
int32_t k2_fopen(uint64_t* fd, const char* pathname, size_t pathname_len, const char* mode, size_t mode_len);

/**
 * Opens a directory stream corresponding to the directory named by the given path.
 *
 * @param `dd` A pointer to a `uint64_t` where the directory descriptor will be stored upon success.
 * @param `path` A pointer to a character array containing the path of the directory to open.
 * @param `path_len` The length of the path string.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `ENOENT` => The directory specified by `path` does not exist.
 * `EACCES` => Permission denied to open the directory.
 * `EINVAL` => The provided `dd` is `nullptr`, or `path` is `nullptr`.
 * `ENAMETOOLONG` => The length of `path` exceeds the maximum allowed.
 * `ENOSYS` => Internal error.
 */
int32_t k2_opendir(uint64_t* dd, const char* path, size_t path_len);

struct DirEntry {
  char* filename;
  size_t filename_len;
  size_t filename_align;
  char* path;
  size_t path_len;
  size_t path_align;
};

/**
 * Reads the next directory entry from the directory stream associated with the given directory descriptor.
 *
 * Memory management:
 * The `filename` and `path` members of `DirEntry` struct are **not** null-terminated and should be manually
 * freed using `k2_free` or `k2_free_checked`.

 * @param `dd` The directory descriptor.
 * @param `entry` A pointer to a `DirEntry` structure where the details of the directory entry will be stored.
 * @param `result` A pointer to a `DirEntry*` where the address of the `entry` will be stored if a directory entry is read.
 *                 If the end of the directory stream is reached, `*result` will be set to `NULL`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => The `dd` is not a valid descriptor, `entry` or `result` is `NULL`.
 * `EBADF` => The `dd` is not a valid directory descriptor.
 * `EIO` => An I/O error occurred while reading from the directory stream.
 * `ENOMEM` => Not enough memory to create members of the `DirEntry` struct.
 * `ENOSYS` => Internal error.
 */
int32_t k2_readdir(uint64_t dd, struct DirEntry* entry, struct DirEntry** result);

/**
 * Semantically equivalent to libc's `access` function.
 *
 * @return On success (all requested permissions granted), zero is returned. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 */
int32_t k2_access(const char* pathname, size_t pathname_len, int32_t mode);

/**
 * Semantically equivalent of libc's unlink
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 *  Possible errno values:
 * `EACCES` => Permission denied.
 * `EIO` => An I/O error occurred.
 * `ENOENT` A component in path does not exist or is a dangling symbolic link, or path is empty.
 */
int32_t k2_unlink(const char* path, size_t path_len);

/**
 * @return return `0` on success. libc-like `errno` otherwise
 *
 *  Some `errno` examples:
 * `EINVAL` => `name` has invalid format (empty, non ascii, too long, etc..)
 * `EACCES` => permission denied
 */
int32_t k2_component_access(size_t name_len, const char* name);

/**
 * Platform binding of OpenSSL's `int RAND_bytes(unsigned char *buf, int num)`.
 * Fills `buf` with `num` cryptographically secure random bytes.
 *
 * @param `buf` A pointer to at least `num` writable bytes.
 * @param `num` The number of bytes to generate.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `RAND_bytes`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `buf` is `NULL` or `num` is negative.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_rand_bytes(void* buf, int32_t num, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `void RAND_add(const void *buf, int num, double randomness)`.
 * Adds `num` bytes from `buf` to the random generator state.
 * `randomness` tells how many of these bytes are really random (must be 0 <= and <= num).
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `buf` is `NULL` or `num` is negative or randomness is invalid.
 */
int32_t k2_openssl_rand_add(const void* buf, int32_t num, double randomness);

/**
 * Platform binding of OpenSSL's `X509 *PEM_read_bio_X509(BIO *bp, X509 **x, pem_password_cb *cb, void *u)` (`x`, `cb` and `u` are `NULL`).
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `pem` A pointer to the PEM data.
 * @param `pem_len` The length of `pem`.
 * @param `x509_d` A pointer where the descriptor of the certificate will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `pem` or `x509_d` is `NULL`, or `pem_len` is negative.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_pem_read_bio_x509(const void* pem, int32_t pem_len, uint64_t* x509_d);

/**
 * Platform binding of OpenSSL's `X509_NAME *X509_get_subject_name(const X509 *a)`.
 * The platform keeps the certificate alive while this descriptor exists, so the descriptors may be
 * freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `name_d` A pointer where the descriptor of the name will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `name_d` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_get_subject_name(uint64_t x509_d, uint64_t* name_d);

/**
 * Platform binding of OpenSSL's `X509_NAME *X509_get_issuer_name(const X509 *a)`.
 * The platform keeps the certificate alive while this descriptor exists, so the descriptors may be
 * freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `name_d` A pointer where the descriptor of the name will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `name_d` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_get_issuer_name(uint64_t x509_d, uint64_t* name_d);

/**
 * Platform binding of OpenSSL's `char *X509_NAME_oneline(const X509_NAME *a, char *buf, int size)`.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `name_d` The descriptor of a name.
 * @param `str` A pointer where the return value of `X509_NAME_oneline` (a `NUL`-terminated string) will be stored. It is valid until `str_d` is freed.
 * @param `str_d` A pointer where the descriptor of the string will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `str` or `str_d` is `NULL`, or `name_d` is not a descriptor of a name.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_name_oneline(uint64_t name_d, const char** str, uint64_t* str_d);

/**
 * Platform binding of OpenSSL's `int X509_NAME_entry_count(const X509_NAME *name)`.
 *
 * @param `name_d` The descriptor of a name.
 * @param `count` A pointer where the return value of `X509_NAME_entry_count` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `count` is `NULL`, or `name_d` is not a descriptor of a name.
 */
int32_t k2_openssl_x509_name_entry_count(uint64_t name_d, int32_t* count);

/**
 * Platform binding of OpenSSL's `X509_NAME_ENTRY *X509_NAME_get_entry(const X509_NAME *name, int loc)`.
 * The platform keeps the certificate alive while this descriptor
 * exists, so the descriptors may be freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `name_d` The descriptor of a name.
 * @param `loc` The index of the entry.
 * @param `entry_d` A pointer where the descriptor of the entry will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `entry_d` is `NULL`, `name_d` is not a descriptor of a name, or `loc` is negative or not less than `k2_openssl_x509_name_entry_count(name_d)`.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_name_get_entry(uint64_t name_d, int32_t loc, uint64_t* entry_d);

/**
 * Platform binding of OpenSSL's `ASN1_OBJECT *X509_NAME_ENTRY_get_object(const X509_NAME_ENTRY *ne)`.
 * The platform keeps the certificate alive while this descriptor
 * exists, so the descriptors may be freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `entry_d` The descriptor of a name entry.
 * @param `obj_d` A pointer where the descriptor of the object will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `obj_d` is `NULL`, or `entry_d` is not a descriptor of a name entry.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_name_entry_get_object(uint64_t entry_d, uint64_t* obj_d);

/**
 * Platform binding of OpenSSL's `int OBJ_obj2nid(const ASN1_OBJECT *o)`.
 *
 * @param `obj_d` The descriptor of an object.
 * @param `nid` A pointer where the return value of `OBJ_obj2nid` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `nid` is `NULL`, or `obj_d` is not a descriptor of an object.
 */
int32_t k2_openssl_obj_obj2nid(uint64_t obj_d, int32_t* nid);

/**
 * Platform binding of OpenSSL's `ASN1_STRING *X509_NAME_ENTRY_get_data(const X509_NAME_ENTRY *ne)`.
 * The platform keeps the certificate alive while this descriptor
 * exists, so the descriptors may be freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `entry_d` The descriptor of a name entry.
 * @param `str_d` A pointer where the descriptor of the string will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `str_d` is `NULL`, or `entry_d` is not a descriptor of a name entry.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_name_entry_get_data(uint64_t entry_d, uint64_t* str_d);

/**
 * Platform binding of OpenSSL's `const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *x)`.
 *
 * @param `str_d` The descriptor of an ASN.1 string.
 * @param `data` A pointer where the return value of `ASN1_STRING_get0_data` will be stored. It is valid only until `str_d` is freed, and it may be `NULL`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `data` is `NULL`, or `str_d` is not a descriptor of an ASN.1 string.
 */
int32_t k2_openssl_asn1_string_get0_data(uint64_t str_d, const void** data);

/**
 * Platform binding of OpenSSL's `int ASN1_STRING_length(const ASN1_STRING *x)`.
 *
 * @param `str_d` The descriptor of an ASN.1 string.
 * @param `len` A pointer where the return value of `ASN1_STRING_length` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `len` is `NULL`, or `str_d` is not a descriptor of an ASN.1 string.
 */
int32_t k2_openssl_asn1_string_length(uint64_t str_d, int32_t* len);

/**
 * Platform binding of OpenSSL's `const char *OBJ_nid2sn(int n)`.
 *
 * @param `nid` A NID.
 * @param `str` A pointer where the return value of `OBJ_nid2sn` (a `NUL`-terminated string) will be stored. The string is static and valid forever.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `str` is `NULL`, or `nid` is unknown.
 */
int32_t k2_openssl_obj_nid2sn(int32_t nid, const char** str);

/**
 * Platform binding of OpenSSL's `const char *OBJ_nid2ln(int n)`.
 *
 * @param `nid` A NID.
 * @param `str` A pointer where the return value of `OBJ_nid2ln` (a `NUL`-terminated string) will be stored. The string is static and valid forever.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `str` is `NULL`, or `nid` is unknown.
 */
int32_t k2_openssl_obj_nid2ln(int32_t nid, const char** str);

/**
 * Platform binding of OpenSSL's `unsigned long X509_subject_name_hash(X509 *x)`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `hash` A pointer where the return value of `X509_subject_name_hash` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `hash` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 */
int32_t k2_openssl_x509_subject_name_hash(uint64_t x509_d, uint64_t* hash);

/**
 * Platform binding of OpenSSL's `long X509_get_version(const X509 *x)`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `version` A pointer where the return value of `X509_get_version` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `version` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 */
int32_t k2_openssl_x509_get_version(uint64_t x509_d, int64_t* version);

/**
 * Platform binding of OpenSSL's `ASN1_TIME *X509_getm_notBefore(const X509 *x)`.
 * The platform keeps the certificate alive while this descriptor exists, so the descriptors may be
 * freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `time_d` A pointer where the descriptor of the time will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `time_d` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_getm_not_before(uint64_t x509_d, uint64_t* time_d);

/**
 * Platform binding of OpenSSL's `ASN1_TIME *X509_getm_notAfter(const X509 *x)`.
 * The platform keeps the certificate alive while this descriptor exists, so the descriptors may be
 * freed in any order. The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `time_d` A pointer where the descriptor of the time will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `time_d` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_getm_not_after(uint64_t x509_d, uint64_t* time_d);

/**
 * Platform binding of OpenSSL's `ASN1_TIME *ASN1_TIME_new(void)`.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `time_d` A pointer where the descriptor of the time will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `time_d` is `NULL`.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_asn1_time_new(uint64_t* time_d);

/**
 * Platform binding of OpenSSL's `ASN1_TIME *ASN1_TIME_set(ASN1_TIME *s, time_t t)`.
 *
 * @param `time_d` The descriptor of a time made by `k2_openssl_asn1_time_new`.
 * @param `t` The number of seconds since the Unix epoch.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `time_d` is not a descriptor of a time made by `k2_openssl_asn1_time_new`, or the year of `t` has more than 8 digits.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_asn1_time_set(uint64_t time_d, int64_t t);

/**
 * Platform binding of OpenSSL's `int ASN1_TIME_diff(int *pday, int *psec, const ASN1_TIME *from, const ASN1_TIME *to)`.
 *
 * @param `from_d` The descriptor of a time.
 * @param `to_d` The descriptor of a time.
 * @param `days` A pointer where `pday` will be stored.
 * @param `seconds` A pointer where `psec` will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `ASN1_TIME_diff`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `days` or `seconds` is `NULL`, or `from_d` or `to_d` is not a descriptor of a time.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_asn1_time_diff(uint64_t from_d, uint64_t to_d, int32_t* days, int32_t* seconds, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int ASN1_TIME_diff(int *pday, int *psec, const ASN1_TIME *from, const ASN1_TIME *to)` with `from == NULL`.
 * OpenSSL treats a `NULL` time as the current time, so this is the difference between the current time and `to_d`.
 *
 * @param `to_d` The descriptor of a time.
 * @param `days` A pointer where `pday` will be stored.
 * @param `seconds` A pointer where `psec` will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `ASN1_TIME_diff`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `days` or `seconds` is `NULL`, or `to_d` is not a descriptor of a time.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_asn1_time_diff_from_now(uint64_t to_d, int32_t* days, int32_t* seconds, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int ASN1_TIME_diff(int *pday, int *psec, const ASN1_TIME *from, const ASN1_TIME *to)` with `to == NULL`.
 * OpenSSL treats a `NULL` time as the current time, so this is the difference between `from_d` and the current time.
 *
 * @param `from_d` The descriptor of a time.
 * @param `days` A pointer where `pday` will be stored.
 * @param `seconds` A pointer where `psec` will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `ASN1_TIME_diff`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `days` or `seconds` is `NULL`, or `from_d` is not a descriptor of a time.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_asn1_time_diff_to_now(uint64_t from_d, int32_t* days, int32_t* seconds, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int X509_PURPOSE_get_count(void)`.
 *
 * @param `count` A pointer where the return value of `X509_PURPOSE_get_count` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `count` is `NULL`.
 */
int32_t k2_openssl_x509_purpose_get_count(int32_t* count);

/**
 * Platform binding of OpenSSL's `X509_PURPOSE *X509_PURPOSE_get0(int idx)`.
 * The purpose is static, but its descriptor must still be freed with `k2_free_descriptor`.
 *
 * @param `idx` The index of the purpose.
 * @param `purpose_d` A pointer where the descriptor of the purpose will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `purpose_d` is `NULL`, or `idx` is negative or not less than `k2_openssl_x509_purpose_get_count()`.
 * `ENOMEM` => The descriptor can not be created.
 */
int32_t k2_openssl_x509_purpose_get0(int32_t idx, uint64_t* purpose_d);

/**
 * Platform binding of OpenSSL's `int X509_PURPOSE_get_id(const X509_PURPOSE *xp)`.
 *
 * @param `purpose_d` The descriptor of a purpose.
 * @param `id` A pointer where the return value of `X509_PURPOSE_get_id` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `id` is `NULL`, or `purpose_d` is not a descriptor of a purpose.
 */
int32_t k2_openssl_x509_purpose_get_id(uint64_t purpose_d, int32_t* id);

/**
 * Platform binding of OpenSSL's `char *X509_PURPOSE_get0_name(const X509_PURPOSE *xp)`.
 *
 * @param `purpose_d` The descriptor of a purpose.
 * @param `str` A pointer where the return value of `X509_PURPOSE_get0_name` (a `NUL`-terminated string) will be stored. The string is static and valid forever.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `str` is `NULL`, or `purpose_d` is not a descriptor of a purpose.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_purpose_get0_name(uint64_t purpose_d, const char** str);

/**
 * Platform binding of OpenSSL's `char *X509_PURPOSE_get0_sname(const X509_PURPOSE *xp)`.
 *
 * @param `purpose_d` The descriptor of a purpose.
 * @param `str` A pointer where the return value of `X509_PURPOSE_get0_sname` (a `NUL`-terminated string) will be stored. The string is static and valid
 * forever.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `str` is `NULL`, or `purpose_d` is not a descriptor of a purpose.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_purpose_get0_sname(uint64_t purpose_d, const char** str);

/**
 * Platform binding of OpenSSL's `int X509_check_purpose(X509 *x, int id, int ca)`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `id` The ID of the purpose.
 * @param `ca` The `ca` argument of `X509_check_purpose`.
 * @param `result` A pointer where the return value of `X509_check_purpose` will be stored: `1` if the certificate has the purpose,
 *                 `0` if it has not, and `-1` if `id` is invalid.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `result` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 */
int32_t k2_openssl_x509_check_purpose(uint64_t x509_d, int32_t id, int32_t ca, int32_t* result);

/**
 * Platform binding of OpenSSL's `EVP_PKEY *X509_get_pubkey(X509 *x)`.
 * The key does not depend on the certificate: it stays valid after the certificate descriptor is freed.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `x509_d` The descriptor of a certificate.
 * @param `pkey_d` A pointer where the descriptor of the public key will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `pkey_d` is `NULL`, or `x509_d` is not a descriptor of a certificate.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_x509_get_pubkey(uint64_t x509_d, uint64_t* pkey_d);

/**
 * Platform binding of OpenSSL's `EVP_PKEY *PEM_read_bio_PUBKEY(BIO *bp, EVP_PKEY **x, pem_password_cb *cb, void *u)` (`x`, `cb` and `u` are `NULL`).
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `pem` A pointer to the PEM data.
 * @param `pem_len` The length of `pem`.
 * @param `pkey_d` A pointer where the descriptor of the public key will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `pem` or `pkey_d` is `NULL`, or `pem_len` is negative.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_pem_read_bio_pubkey(const void* pem, int32_t pem_len, uint64_t* pkey_d);

/**
 * Platform binding of OpenSSL's `EVP_PKEY *PEM_read_bio_PrivateKey(BIO *bp, EVP_PKEY **x, pem_password_cb *cb, void *u)`
 * (`x` and `cb` are `NULL`, `u` is the passphrase: the default callback of OpenSSL reads it as a `NUL`-terminated string).
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `pem` A pointer to the PEM data.
 * @param `pem_len` The length of `pem`.
 * @param `pass` A pointer to the passphrase, may be `NULL` (no passphrase, then `pass_len` must be `0`). The passphrase does not need to be `NUL`-terminated:
 * exactly `pass_len` bytes are read, and the part starting from the first `NUL` byte (if any) is ignored.
 * @param `pass_len` The length of `pass`.
 * @param `pkey_d` A pointer where the descriptor of the private key will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `pem` or `pkey_d` is `NULL`, `pem_len` or `pass_len` is negative, or `pass` is `NULL` while `pass_len` is not `0`.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_pem_read_bio_private_key(const void* pem, int32_t pem_len, const void* pass, int32_t pass_len, uint64_t* pkey_d);

/**
 * Platform binding of OpenSSL's `int EVP_PKEY_id(const EVP_PKEY *pkey)`.
 *
 * @param `pkey_d` The descriptor of a key.
 * @param `id` A pointer where the return value of `EVP_PKEY_id` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `id` is `NULL`, or `pkey_d` is not a descriptor of a key.
 */
int32_t k2_openssl_evp_pkey_id(uint64_t pkey_d, int32_t* id);

/**
 * Checks if the type of a key is rsa: `EVP_PKEY_id(pkey)` is `EVP_PKEY_RSA` or `EVP_PKEY_RSA2`.
 *
 * @param `pkey_d` The descriptor of a key.
 * @param `is_rsa` A pointer where `1` will be stored if the key is an RSA key, and `0` otherwise.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `is_rsa` is `NULL`, or `pkey_d` is not a descriptor of a key.
 */
int32_t k2_openssl_evp_pkey_is_rsa(uint64_t pkey_d, int32_t* is_rsa);

/**
 * Platform binding of OpenSSL's `int EVP_PKEY_size(const EVP_PKEY *pkey)`.
 *
 * @param `pkey_d` The descriptor of a key.
 * @param `size` A pointer where the return value of `EVP_PKEY_size` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `size` is `NULL`, or `pkey_d` is not a descriptor of a key.
 */
int32_t k2_openssl_evp_pkey_size(uint64_t pkey_d, int32_t* size);

/**
 * Platform binding of OpenSSL's `RSA *EVP_PKEY_get1_RSA(EVP_PKEY *pkey)`.
 * The RSA key does not depend on the key descriptor: it stays valid after `pkey_d` is freed.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `pkey_d` The descriptor of a key.
 * @param `rsa_d` A pointer where the descriptor of the RSA key will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `rsa_d` is `NULL`, or `pkey_d` is not a descriptor of a key.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_pkey_get1_rsa(uint64_t pkey_d, uint64_t* rsa_d);

/**
 * Platform binding of OpenSSL's
 * `int RSA_public_encrypt(int flen, const unsigned char *from, unsigned char *to, RSA *rsa, int padding)`.
 *
 * @param `from` A pointer to the data to encrypt.
 * @param `from_len` The length of `from`.
 * @param `to` A pointer to the output buffer.
 * @param `to_len` The size of the output buffer, it must be at least `k2_openssl_evp_pkey_size(rsa_d)`.
 * @param `rsa_d` The descriptor of an RSA key.
 * @param `padding` The RSA padding.
 * @param `written` A pointer where the number of bytes written to `to` will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `RSA_public_encrypt`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `from`, `to` or `written` is `NULL`, `from_len` is negative, `padding` is not a `RsaPadding` value, `rsa_d` is not a descriptor of an RSA key,
 *             or the `from` and `to` buffers overlap.
 * `ERANGE` => `to_len` is less than `k2_openssl_evp_pkey_size(rsa_d)`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_rsa_public_encrypt(const void* from, int32_t from_len, void* to, int32_t to_len, uint64_t rsa_d, enum RsaPadding padding, int32_t* written,
                                      int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's
 * `int RSA_private_decrypt(int flen, const unsigned char *from, unsigned char *to, RSA *rsa, int padding)`.
 *
 * @param `from` A pointer to the data to decrypt.
 * @param `from_len` The length of `from`.
 * @param `to` A pointer to the output buffer.
 * @param `to_len` The size of the output buffer, it must be at least `k2_openssl_evp_pkey_size(rsa_d)`.
 * @param `rsa_d` The descriptor of an RSA key.
 * @param `padding` The RSA padding.
 * @param `written` A pointer where the number of bytes written to `to` (the return value of `RSA_private_decrypt`) will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `RSA_private_decrypt`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `from`, `to` or `written` is `NULL`, `from_len` is negative, `padding` is not a `RsaPadding` value, `rsa_d` is not a descriptor of an RSA key,
 *             or the `from` and `to` buffers overlap.
 * `ERANGE` => `to_len` is less than `k2_openssl_evp_pkey_size(rsa_d)`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_rsa_private_decrypt(const void* from, int32_t from_len, void* to, int32_t to_len, uint64_t rsa_d, enum RsaPadding padding, int32_t* written,
                                       int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `EVP_MD_CTX *EVP_MD_CTX_new(void)`.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `md_ctx_d` A pointer where the descriptor of the context will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md_ctx_d` is `NULL`.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_md_ctx_new(uint64_t* md_ctx_d);

/**
 * Platform binding of OpenSSL's `int EVP_SignInit(EVP_MD_CTX *ctx, const EVP_MD *type)`.
 * `type` is selected by `algorithm`: `DIGEST_SHA1`, ..., `DIGEST_MD4` are `EVP_sha1()`, ..., `EVP_md4()`.
 *
 * @param `md_ctx_d` The descriptor of a context.
 * @param `algorithm` The message digest.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_SignInit`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md_ctx_d` is not a descriptor of a context, `algorithm` is unknown, or the digest is not available in this OpenSSL build.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_sign_init(uint64_t md_ctx_d, enum DigestAlgorithm algorithm, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int EVP_SignUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt)`.
 *
 * @param `md_ctx_d` The descriptor of a context initialized by `k2_openssl_evp_sign_init`.
 * @param `data` A pointer to the data to sign. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_SignUpdate`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md_ctx_d` is not a descriptor of a context initialized by `k2_openssl_evp_sign_init`, or `data` is `NULL` while `data_len` is not `0`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_sign_update(uint64_t md_ctx_d, const void* data, size_t data_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's
 * `int EVP_SignFinal(EVP_MD_CTX *ctx, unsigned char *sig, unsigned int *s, EVP_PKEY *pkey)`.
 *
 * @param `md_ctx_d` The descriptor of a context.
 * @param `sig` A pointer to the output buffer.
 * @param `sig_cap` The size of the output buffer, it must be at least `EVP_PKEY_size(pkey)`.
 * @param `sig_len` A pointer where the length of the signature (the `s` argument of `EVP_SignFinal`) will be stored.
 * @param `pkey_d` The descriptor of a private key.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_SignFinal`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `sig` or `sig_len` is `NULL`, `md_ctx_d` or `pkey_d` is not a descriptor of the right kind, or the context was not initialized by
 *             `k2_openssl_evp_sign_init`.
 * `ERANGE` => `sig_cap` is less than `EVP_PKEY_size(pkey)`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_sign_final(uint64_t md_ctx_d, void* sig, size_t sig_cap, uint32_t* sig_len, uint64_t pkey_d, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int EVP_VerifyInit(EVP_MD_CTX *ctx, const EVP_MD *type)`.
 * `type` is selected by `algorithm`: `DIGEST_SHA1`, ..., `DIGEST_MD4` are `EVP_sha1()`, ..., `EVP_md4()`.
 *
 * @param `md_ctx_d` The descriptor of a context.
 * @param `algorithm` The message digest.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_VerifyInit`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md_ctx_d` is not a descriptor of a context, `algorithm` is unknown, or the digest is not available in this OpenSSL build.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_verify_init(uint64_t md_ctx_d, enum DigestAlgorithm algorithm, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int EVP_VerifyUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt)`.
 *
 * @param `md_ctx_d` The descriptor of a context initialized by `k2_openssl_evp_verify_init`.
 * @param `data` A pointer to the signed data. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_VerifyUpdate`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md_ctx_d` is not a descriptor of a context initialized by `k2_openssl_evp_verify_init`, or `data` is `NULL` while `data_len` is not `0`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_verify_update(uint64_t md_ctx_d, const void* data, size_t data_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's
 * `int EVP_VerifyFinal(EVP_MD_CTX *ctx, const unsigned char *sigbuf, unsigned int siglen, EVP_PKEY *pkey)`.
 *
 * @param `md_ctx_d` The descriptor of a context.
 * @param `sig` A pointer to the signature.
 * @param `sig_len` The length of `sig`.
 * @param `pkey_d` The descriptor of a public key.
 * @param `verified` A pointer where `1` will be stored if the signature is correct and `0` if it is not.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_VerifyFinal`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `verified` or `sig` is `NULL`, `sig_len` is `0`, `md_ctx_d` or `pkey_d` is not a descriptor of the right kind,
 *              or the context was not initialized by `k2_openssl_evp_verify_init`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_verify_final(uint64_t md_ctx_d, const void* sig, uint32_t sig_len, uint64_t pkey_d, int32_t* verified, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `const EVP_CIPHER *EVP_get_cipherbyname(const char *name)`.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `name` A pointer to the name of the cipher. It does not need to be `NUL`-terminated: exactly `name_len` bytes are read.
 * @param `name_len` The length of `name`.
 * @param `cipher_d` A pointer where the descriptor of the cipher will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `name` or `cipher_d` is `NULL`, `name_len` is negative or too long for a cipher name, or `name` contains a `NUL` byte.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_get_cipherbyname(const void* name, int32_t name_len, uint64_t* cipher_d);

/**
 * Platform binding of OpenSSL's `int EVP_CIPHER_key_length(const EVP_CIPHER *cipher)`.
 *
 * @param `cipher_d` The descriptor of a cipher.
 * @param `key_len` A pointer where the return value of `EVP_CIPHER_key_length` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `key_len` is `NULL`, or `cipher_d` is not a descriptor of a cipher.
 */
int32_t k2_openssl_evp_cipher_key_length(uint64_t cipher_d, int32_t* key_len);

/**
 * Platform binding of OpenSSL's `int EVP_CIPHER_iv_length(const EVP_CIPHER *cipher)`.
 *
 * @param `cipher_d` The descriptor of a cipher.
 * @param `iv_len` A pointer where the return value of `EVP_CIPHER_iv_length` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `iv_len` is `NULL`, or `cipher_d` is not a descriptor of a cipher.
 */
int32_t k2_openssl_evp_cipher_iv_length(uint64_t cipher_d, int32_t* iv_len);

/**
 * Platform binding of OpenSSL's `int EVP_CIPHER_block_size(const EVP_CIPHER *cipher)`.
 *
 * @param `cipher_d` The descriptor of a cipher.
 * @param `block_size` A pointer where the return value of `EVP_CIPHER_block_size` will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `block_size` is `NULL`, or `cipher_d` is not a descriptor of a cipher.
 */
int32_t k2_openssl_evp_cipher_block_size(uint64_t cipher_d, int32_t* block_size);

/**
 * Platform binding of OpenSSL's `int EVP_CIPHER_mode(const EVP_CIPHER *cipher)`.
 *
 * @param `cipher_d` The descriptor of a cipher.
 * @param `mode` A pointer where the return value of `EVP_CIPHER_mode` will be stored. Compare it with `enum CipherMode`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `mode` is `NULL`, or `cipher_d` is not a descriptor of a cipher.
 */
int32_t k2_openssl_evp_cipher_mode(uint64_t cipher_d, uint32_t* mode);

/**
 * Platform binding of OpenSSL's `EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void)`.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `cipher_ctx_d` A pointer where the descriptor of the context will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is `NULL`.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_new(uint64_t* cipher_ctx_d);

/**
 * Platform binding of OpenSSL's
 * `int EVP_CipherInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, ENGINE *impl, const unsigned char *key, const unsigned char *iv, int enc)`
 * (`impl` is `NULL`).
 *
 * OpenSSL reads the key and the IV without knowing their sizes, so the platform checks them: if `key` is not `NULL`, `key_len` must be at least
 * the key length of the cipher (`EVP_CIPHER_CTX_key_length`) and if `iv` is not `NULL`, `iv_len` must be at least the IV length
 * (`EVP_CIPHER_CTX_iv_length`, or the length given by a successful `k2_openssl_evp_cipher_ctx_set_iv_len`). The lengths are taken from `cipher_d` if it
 * is not `0`, otherwise from the cipher set in the context by an earlier call.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `cipher_d` The descriptor of a cipher, or `0` for `type == NULL` (keep the cipher set by an earlier call).
 * @param `key` A pointer to the key, may be `NULL`.
 * @param `key_len` The length of `key`.
 * @param `iv` A pointer to the IV, may be `NULL`.
 * @param `iv_len` The length of `iv`.
 * @param `enc` `1` to encrypt, `0` to decrypt, `-1` to keep the value set by an earlier call.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CipherInit_ex`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context, `cipher_d` is neither `0` nor a descriptor of a cipher, `enc` is not `-1`, `0` or `1`,
 *             or `cipher_d` is `0` while the context has no cipher and `key` or `iv` is not `NULL`.
 * `ERANGE` => `key_len` or `iv_len` is less than described above.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_init_ex(uint64_t cipher_ctx_d, uint64_t cipher_d, const void* key, int32_t key_len, const void* iv, int32_t iv_len, int32_t enc,
                                      int32_t* openssl_err);

/**
 * Platform binding of `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, iv_len, NULL)`.
 * Sets the IV length of an AEAD cipher. Must be called before the IV is set.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `iv_len` The IV length, must be positive.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CIPHER_CTX_ctrl`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context with an AEAD cipher set, or `iv_len` is not positive.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_set_iv_len(uint64_t cipher_ctx_d, int32_t iv_len, int32_t* openssl_err);

/**
 * Platform binding of `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tag_len, tag)`.
 * Only when encrypting, after all the data is processed (`k2_openssl_evp_cipher_final_ex`).
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `tag` A pointer to the buffer that receives the tag.
 * @param `tag_len` The size of `tag` and the length of the tag to get, must be in `1..=16`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CIPHER_CTX_ctrl`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context with an AEAD cipher set, the context is not encrypting, `tag` is `NULL`, or `tag_len` is not
 *             in `1..=16`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_get_tag(uint64_t cipher_ctx_d, void* tag, int32_t tag_len, int32_t* openssl_err);

/**
 * Platform binding of `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, tag_len, tag)`.
 * Sets the expected tag, only when decrypting. Must be called before `k2_openssl_evp_cipher_final_ex`.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `tag` A pointer to the expected tag.
 * @param `tag_len` The length of `tag`, must be in `1..=16`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CIPHER_CTX_ctrl`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context with an AEAD cipher set, the context is not decrypting, `tag` is `NULL`, or `tag_len` is not
 *             in `1..=16`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_set_tag(uint64_t cipher_ctx_d, const void* tag, int32_t tag_len, int32_t* openssl_err);

/**
 * Platform binding of `EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, tag_len, NULL)`.
 * Sets the length of the tag that will be produced, only when encrypting (CCM and OCB; GCM reports an error). Must be called before the data is processed.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `tag_len` The tag length, must be in `1..=16`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CIPHER_CTX_ctrl`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context with an AEAD cipher set, the context is not encrypting, or `tag_len` is not in `1..=16`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_set_tag_len(uint64_t cipher_ctx_d, int32_t tag_len, int32_t* openssl_err);

/**
 * Platform binding of the CCM data length call `EVP_CipherUpdate(ctx, NULL, &outl, NULL, data_len)`.
 * CCM needs the total length of the data (it may be `0`) before the AAD is given with `k2_openssl_evp_cipher_update`.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `data_len` The length of the data that will be processed, must not be negative.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CipherUpdate`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context with a CCM cipher and a key set (see `k2_openssl_evp_cipher_init_ex`), or `data_len` is negative.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_set_ccm_data_len(uint64_t cipher_ctx_d, int32_t data_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int EVP_CIPHER_CTX_set_key_length(EVP_CIPHER_CTX *x, int keylen)`.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `key_len` The new key length.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CIPHER_CTX_set_key_length`. Otherwise it is not
 * touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context with a cipher set, or `key_len` is negative.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_ctx_set_key_length(uint64_t cipher_ctx_d, int32_t key_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX *c, int pad)`.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `pad` Non-zero to enable padding, `0` to disable it.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `cipher_ctx_d` is not a descriptor of a context.
 */
int32_t k2_openssl_evp_cipher_ctx_set_padding(uint64_t cipher_ctx_d, int32_t pad);

/**
 * Platform binding of OpenSSL's
 * `int EVP_CipherUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl)`.
 *
 * `out` may be `NULL` only for an AEAD cipher: this is how OpenSSL is given the additional authenticated data (`in` is the data). In that case nothing
 * is written, and `out_cap` is ignored. For CCM the data length must be set before that with `k2_openssl_evp_cipher_ctx_set_ccm_data_len`.
 * For GCM the AAD must be given before the data.
 * Otherwise `out_cap` must be at least `in_len + EVP_CIPHER_CTX_block_size(ctx)`, which is the most OpenSSL may write.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `out` A pointer to the output buffer, may be `NULL`.
 * @param `out_cap` The size of the output buffer.
 * @param `out_len` A pointer where the number of bytes written to `out` (the `outl` argument of `EVP_CipherUpdate`) will be stored.
 * @param `in` A pointer to the input data. May be `NULL` only if `in_len` is `0`.
 * @param `in_len` The length of `in`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CipherUpdate`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `out_len` is `NULL`, `cipher_ctx_d` is not a descriptor of a context with a cipher and a key set (see `k2_openssl_evp_cipher_init_ex`), `in_len`
 * is negative, `in` is `NULL` while `in_len` is not `0`, `out` is `NULL` while the cipher is not an AEAD cipher (GCM, CCM, OCB or ChaCha20-Poly1305), or the
 * `in` and `out` buffers overlap.
 * `ERANGE` => `out` is not `NULL` and `out_cap` is less than described above.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_update(uint64_t cipher_ctx_d, void* out, int32_t out_cap, int32_t* out_len, const void* in, int32_t in_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int EVP_CipherFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl)`.
 * The context stays valid: for an AEAD cipher the tag can be received after this call with `k2_openssl_evp_cipher_ctx_get_tag`.
 *
 * @param `cipher_ctx_d` The descriptor of a context.
 * @param `out` A pointer to the output buffer.
 * @param `out_cap` The size of the output buffer, it must be at least `EVP_CIPHER_CTX_block_size(ctx)`.
 * @param `out_len` A pointer where the number of bytes written to `out` (the `outl` argument of `EVP_CipherFinal_ex`) will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `EVP_CipherFinal_ex`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `out` or `out_len` is `NULL`, or `cipher_ctx_d` is not a descriptor of a context with a cipher and a key set (see
 * `k2_openssl_evp_cipher_init_ex`).
 * `ERANGE` => `out_cap` is less than described above.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_evp_cipher_final_ex(uint64_t cipher_ctx_d, void* out, int32_t out_cap, int32_t* out_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `SHA_DIGEST_LENGTH` (20) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `SHA_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`.
 * `ERANGE` => `md_cap` is less than `SHA_DIGEST_LENGTH`.
 */
int32_t k2_openssl_sha1(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *SHA224(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `SHA224_DIGEST_LENGTH` (28) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `SHA224_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`.
 * `ERANGE` => `md_cap` is less than `SHA224_DIGEST_LENGTH`.
 */
int32_t k2_openssl_sha224(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `SHA256_DIGEST_LENGTH` (32) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `SHA256_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`.
 * `ERANGE` => `md_cap` is less than `SHA256_DIGEST_LENGTH`.
 */
int32_t k2_openssl_sha256(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *SHA384(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `SHA384_DIGEST_LENGTH` (48) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `SHA384_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`.
 * `ERANGE` => `md_cap` is less than `SHA384_DIGEST_LENGTH`.
 */
int32_t k2_openssl_sha384(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *SHA512(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `SHA512_DIGEST_LENGTH` (64) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `SHA512_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`.
 * `ERANGE` => `md_cap` is less than `SHA512_DIGEST_LENGTH`.
 */
int32_t k2_openssl_sha512(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *MD5(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `MD5_DIGEST_LENGTH` (16) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `MD5_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`.
 * `ERANGE` => `md_cap` is less than `MD5_DIGEST_LENGTH`.
 */
int32_t k2_openssl_md5(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *RIPEMD160(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `RIPEMD160_DIGEST_LENGTH` (20) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `RIPEMD160_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`, or the function is not available in this OpenSSL build.
 * `ERANGE` => `md_cap` is less than `RIPEMD160_DIGEST_LENGTH`.
 */
int32_t k2_openssl_ripemd160(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *MD4(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `MD4_DIGEST_LENGTH` (16) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `MD4_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`, or the function is not available in this OpenSSL build.
 * `ERANGE` => `md_cap` is less than `MD4_DIGEST_LENGTH`.
 */
int32_t k2_openssl_md4(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `unsigned char *MD2(const unsigned char *d, size_t n, unsigned char *md)`.
 *
 * @param `data` A pointer to the data to hash. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `md` A pointer to the output buffer, it receives `MD2_DIGEST_LENGTH` (16) bytes.
 * @param `md_cap` The size of the output buffer, it must be at least `MD2_DIGEST_LENGTH`.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` is `NULL`, `data` is `NULL` while `data_len` is not `0`, or the function is not available in this OpenSSL build.
 * `ERANGE` => `md_cap` is less than `MD2_DIGEST_LENGTH`.
 */
int32_t k2_openssl_md2(const void* data, size_t data_len, void* md, size_t md_cap);

/**
 * Platform binding of OpenSSL's `HMAC_CTX *HMAC_CTX_new(void)`.
 * The descriptor must be freed with `k2_free_descriptor`.
 *
 * @param `hmac_ctx_d` A pointer where the descriptor of the context will be stored.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `hmac_ctx_d` is `NULL`.
 * `ENOMEM` => The descriptor can not be created.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_hmac_ctx_new(uint64_t* hmac_ctx_d);

/**
 * Platform binding of OpenSSL's `int HMAC_Init_ex(HMAC_CTX *ctx, const void *key, int len, const EVP_MD *md, ENGINE *impl)` (`impl` is `NULL`).
 * `md` is selected by `algorithm`: `HASH_SHA1`, ..., `HASH_MD4` are `EVP_sha1()`, ..., `EVP_md4()`.
 * The key is always set (unlike OpenSSL, there is no way to keep the key of an earlier call), so a context can be reused with any `algorithm`.
 *
 * @param `hmac_ctx_d` The descriptor of a context.
 * @param `key` A pointer to the HMAC key. May be `NULL` if `key_len` is `0`, that is an empty key.
 * @param `key_len` The length of `key`.
 * @param `algorithm` The hash algorithm.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `HMAC_Init_ex`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `hmac_ctx_d` is not a descriptor of an HMAC context, `key` is `NULL` while `key_len` is not `0`, `key_len` is negative,
 *             `algorithm` is unknown, or the digest is not available in this OpenSSL build.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_hmac_init_ex(uint64_t hmac_ctx_d, const void* key, int32_t key_len, enum HashAlgorithm algorithm, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int HMAC_Update(HMAC_CTX *ctx, const unsigned char *data, size_t len)`.
 *
 * @param `hmac_ctx_d` The descriptor of a context initialized by `k2_openssl_hmac_init_ex`.
 * @param `data` A pointer to the data to authenticate. May be `NULL` if `data_len` is `0`.
 * @param `data_len` The length of `data`.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `HMAC_Update`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `hmac_ctx_d` is not a descriptor of an HMAC context initialized by `k2_openssl_hmac_init_ex`, or `data` is `NULL` while `data_len` is not `0`.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_hmac_update(uint64_t hmac_ctx_d, const void* data, size_t data_len, int32_t* openssl_err);

/**
 * Platform binding of OpenSSL's `int HMAC_Final(HMAC_CTX *ctx, unsigned char *md, unsigned int *len)`.
 *
 * @param `hmac_ctx_d` The descriptor of a context initialized by `k2_openssl_hmac_init_ex`.
 * @param `md` A pointer to the output buffer.
 * @param `md_cap` The size of the output buffer, it must be at least `EVP_MD_size` of the digest of the context.
 * @param `md_len` A pointer where the length of the result (the `len` argument of `HMAC_Final`) will be stored.
 * @param `openssl_err` Optional (may be `NULL`). If `EIO` is returned, receives the return value of `HMAC_Final`. Otherwise it is not touched.
 *
 * @return Returns `0` on success. On failure, returns a non-zero value corresponding to a libc-like `errno`.
 *
 * Possible `errno` values:
 * `EINVAL` => `md` or `md_len` is `NULL`, or `hmac_ctx_d` is not a descriptor of an HMAC context initialized by
 *             `k2_openssl_hmac_init_ex`.
 * `ERANGE` => `md_cap` is less than `EVP_MD_size` of the digest of the context.
 * `EIO` => OpenSSL failed.
 */
int32_t k2_openssl_hmac_final(uint64_t hmac_ctx_d, void* md, size_t md_cap, uint32_t* md_len, int32_t* openssl_err);

/**
 * Try to send rpc request to actor. On success, write descriptor of the corresponding rpc query to `rpc_d`.
 * This descriptor should be later used to call `k2_rpc_get_response_size` and `k2_rpc_fetch_response`.
 * On failure return positive libc-like errno.
 *
 * The request is copied synchronously, so the caller may reuse the buffer after return.
 *
 * The returned descriptor is pollable like a stream - `k2_take_update` may return update on it when the response is ready
 * and `k2_stream_status` can be called on it, returning read status is `Blocked` when response is not ready yet
 * and `Available` when it is ready.
 *
 * @return return `0` on success. libc-like `errno` otherwise.
 *
 * Possible `errno` values:
 * `EMFILE` => max descriptors count achieved.
 * `ENODEV` => rpc client module is not available.
 * `EINVAL` => invalid `actor_name` or request.
 */
int32_t k2_rpc_send_request(const char* actor_name, size_t actor_name_len, const void* request_ptr, size_t request_size, enum RpcKind rpc_kind,
                            uint64_t* rpc_d);

/**
 * Try to get response size for the corresponding query of this `rpc_d`.
 * Write 0 to `response_size` and return `EAGAIN` if response is not ready yet.
 * Write positive response size value to `response_size` if response is ready and return 0.
 *
 * @return return `0` on success. libc-like `errno` otherwise
 *
 * Possible `errno` values:
 * `EINVAL` => invalid `rpc_d` descriptor, for example, it is unknown descriptor, or not rpc descriptor.
 * `EAGAIN` => response is not ready yet.
 */
int32_t k2_rpc_get_response_size(uint64_t rpc_d, size_t* response_size);

/**
 * Try to fetch response for the corresponding query of this `rpc_d`. If response is ready, write it to `buf` and return 0.
 * Return `EAGAIN` if response is not ready yet and write nothing. Return `ENOBUFS` if `buf_size` < response length.
 * User should get response size by calling `k2_rpc_get_response_size` first.
 *
 * @return return `0` on success. libc-like `errno` otherwise
 *
 * Possible `errno` values:
 * `EINVAL`  => invalid `rpc_d` descriptor, for example, it is unknown descriptor, or not rpc descriptor.
 * `EAGAIN`  => response is not ready yet.
 * `ENOBUFS` => provided response buffer is not big enough, (`buf_size` < `response_size` value written by `k2_rpc_get_response_size`).
 */
int32_t k2_rpc_fetch_response(uint64_t rpc_d, void* buf, size_t buf_size);

/**
 * If the write or read status is `Blocked` - then the platform ensures that
 * the component receives this `stream_d` via `k2_take_update` when the status is
 * no longer `Blocked` ("edge-triggered epoll"-like behaviour).
 * Can be called on stream or rpc query descriptors.
 *
 * `status` will be filled with actual descriptor status on of success.
 * `s->libc_errno` used to represent errors.
 *
 * Some `errno` examples:
 * `EBADF` => d is not valid(never was valid or used after free)
 * `EBADR` => d is valid descriptor, but not a stream or rpc query (probably, timer)
 */
void k2_stream_status(uint64_t stream_d, struct StreamStatus* status);

/**
 * Guaranteed to return `0` if  the stream is `Closed`, `Blocked` or `stream_d` is invalid.
 * May return `0` even if the stream is `Available`.
 *
 * The bytes processed may be less than `data_len`,
 * even if the stream is in `Available` state after the operation.
 * Therefore, it does not indicate `Blocked` or `Closed` status.
 * Use `k2_stream_status` to check how to perform co-scheduling.
 *
 * @return number of written bytes.
 */
size_t k2_write(uint64_t stream_d, size_t data_len, const void* data);

/**
 * Guaranteed to return `0` if  the stream is `Closed`, `Blocked` or `stream_d` is invalid.
 * May return `0` even if the stream is `Available`.
 *
 * The bytes processed may be less than `buf_len`,
 * even if the stream is in `Available` state after the operation.
 * Therefore, it does not indicate `Blocked` or `Closed` status.
 * Use `k2_stream_status` to check how to perform co-scheduling.
 *
 * @return number of read bytes.
 */
size_t k2_read(uint64_t stream_d, size_t buf_len, void* buf);

/**
 * reads at most `buf_len` bytes to `buf` from `offset` position without modifying stream state.
 *
 * @return number of read bytes.
 */
size_t k2_pread(uint64_t stream_d, size_t buf_len, void* buf, uint64_t offset);

/**
 * behavior is similar to libc's `fgets` function, except that EOF and empty string return the same result.
 *
 * @return `0` if EOF is reached or an error occurs. total number of bytes read otherwise
 */
size_t k2_readline(uint64_t stream_d, size_t buf_len, void* buf);

/**
 * Semantically equivalent to libc's `mmap` function.
 *
 * @param `md` A pointer to a `uint64_t` where the mmap descriptor will be stored upon success.
 * @param `addr` Must always be nullptr.
 */
void* k2_mmap(uint64_t* md, void* addr, size_t len, int32_t prot, int32_t flags, uint64_t fd, uint64_t offset);

/**
 * Semantically equivalent to libc's `madvise` function.
 */
int32_t k2_madvise(void* addr, size_t length, int32_t advise);

/**
 * Writes a pre-serialized metrics to the specified monitoring system.
 *
 * The buffer must contain a metric serialized according to the following format
 * (TL serialization, native byte order):
 *   <timestamp:u64><metric name:tl string><value format><tags count:u32><tag1><tag2>...
 *   tag := <name:tl string><value:tl string>
 *
 * value format:
 *   <`VALUE_MAGIC`:u32><f64>                              - single double value
 *   <`VALUES_ARRAY_MAGIC`:u32><len:u32><f64><f64>...      - array of double values
 *   <`COUNT_MAGIC`:u32><u32>                              - count value
 *   <`INC_MAGIC`:u32>                                     - counter increment
 *
 * tl string is the standard TL string encoding.
 *
 * Multiple metrics can be sent in a single call by concatenating them sequentially:
 *   <metric1><metric2>...
 * Each metric is serialized independently using the format described above.
 *
 * @param `buf` A pointer to the serialized metric(s) data.
 * @param `buf_len` The length of the serialized metric(s) data in bytes.
 * @return returns 0 if everything is fine, otherwise error code
 */
int32_t k2_write_metrics(const void* buf, size_t buf_len);

/**
 * Sets `StreamStatus.please_whutdown_write=true` for the component on the
 * opposite side (does not affect `StreamStatus` on your side).
 * Does not disable the ability to read from the stream.
 * To perform a graceful shutdown, you still need to read data from the stream
 * as long as `read_status != IOClosed`.
 */
void k2_please_shutdown(uint64_t stream_d);

/**
 * Disables the ability to write to a stream.
 * Data written to the stream buffer is still available for reading on the
 * opposite side.
 * Does not affect the ability to read from the stream.
 *
 * TODO: design information errors.
 */
void k2_shutdown_write(uint64_t stream_d);

// Coordinated with timers. Monotonical, for timeouts, measurements, etc..
void k2_instant(struct TimePoint* time_point);

/**
 * One-shot timer.
 * Use `free_descriptor` to cancel.
 *
 * @return `0` on success. libc-like `errno` on error.
 * `descriptor` will be assigned to new descriptor on success.
 * `descriptor` will be assigned `0` on error.
 *
 * Probably this function never fails.
 * Looks like `ENOMEM` is only reasonable error.
 * I think component should prefer abortion in this case.
 */
int32_t k2_new_timer(uint64_t* descriptor, uint64_t duration_ns);

/**
 * @return `0` on success. libc-like `errno` on error.
 * `deadline` will be assigned to actual timer deadline on success.
 * `deadline` will be assigned `0` on error.
 *
 * Some `error` examples:
 * `EBADF` => `d` is not valid(never was valid or used after free)
 * `EBADR` => `d` is valid descriptor, but not a timer (probably stream)
 */
int32_t k2_timer_deadline(uint64_t d, struct TimePoint* deadline);

/**
 * "Free" associated descriptor.
 * All future uses of this `descriptor` will be invalid.
 *
 * In case were `descriptor` is stream:
 * 1) If you are the owner of the stream (performed an `open`) -
 * the read and write buffers will be immediately deleted. The opposite side
 * will receive an `IOClosed` notification in both directions.
 * 2) If you are not the owner, the opposite side will receive a notification
 * `write_status = IOClosed`; Data sent from the opposite side and remaining
 * in buffer will be deleted. The opposite side will be able to read the data
 * sent from you and remaining in the buffer stream.
 *
 * In case where `descriptor` is timer:
 * Ensures that no notification from this timer will be received.
 *
 * Perform `noop` if `descriptor` is invalid descriptor
 */
void k2_free_descriptor(uint64_t descriptor);

/**
 * Get any updates from platform.
 *
 * If a component has not take all updates during a `poll` iteration, the
 * platform is guaranteed to reschedule it.
 */
enum UpdateStatus k2_take_update(uint64_t* update_d);

/**
 * Represents a key-value pair that can be added to a log.
 * Each instance of this struct must contain valid, non-null pointers for both the key and the value.
 * The lengths of the key and value are specified by `key_len` and `value_len`, respectively.
 *
 * If an instance is intended to represent only a key without an associated value,
 * the `value` pointer should point to an empty string (""), and `value_len` should be set to zero.
 */
struct LogKeyValuePair {
  const char* key;
  const char* value;
  size_t key_len;
  size_t value_len;
};

/**
 * Writes a structured log message. Only UTF-8 encoded strings are supported.
 *
 * @param `level` The severity level of the log message.
 * Possible `level` values:
 *    1 => Error
 *    2 => Warn
 *    3 => Info
 *    4 => Debug
 *    5 => Trace
 * Any other value will cause the log to be skipped
 * if `level` > `log_level_enabled()` log will be skipped
 * @param `len` length of msg in bytes
 * @param `msg` A pointer to the log message
 * @param `kv_count` The number of key-value pairs.
 * @param `kv_pairs` An array of key-value pairs.
 */

void k2_log(size_t level, size_t len, const char* msg, size_t kv_count, const struct LogKeyValuePair* kv_pairs);

// Use for optimization, see `k2_log`
size_t k2_log_level_enabled();

// Note: prefer to use only as seed generator for pseudo-random
void k2_os_rnd(size_t len, void* bytes);

// Time since epoch, non-monotonical
void k2_system_time(struct SystemTime* system_time);

/**
 * libc-like socket api.
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_socket(uint64_t* socket_d, int32_t domain, int32_t type, int32_t protocol);

/**
 * libc-like socket api.
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_connect(uint64_t socket_d, const struct sockaddr_storage* addr, size_t addrlen);

/**
 * Perform sequentially `k2_lookup_host` -> `k2_socket` -> `k2_connect`
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_udp_connect(uint64_t* socket_d, const char* hostport, size_t hostport_len);

/**
 * Perform sequentially `k2_lookup_host` -> `k2_socket` -> `k2_connect`
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_tcp_connect(uint64_t* socket_d, const char* hostport, size_t hostport_len);

struct SockAddr {
  uint16_t is_v6;
  uint16_t port;
  uint8_t addr[16];  // used first 4 bytes in case of ipv4
  uint32_t flowinfo; // ipv6 only
  uint32_t scope_id; // ipv6 only
};

/**
 * Optimistically tries to parse `hostport` as `ip` + `port`.
 * Examples:
 * `IpV4` `hostport` format: `"123.123.123.123:8080"`.
 * `IpV6` `hostport` format: `"[2001:db8::1]:8080"`.
 *
 * Then performs a DNS resolution with `man 3 getaddrinfo` (implementation depends on the host machine settings).
 * Examples:
 * `hostport = "localhost:8080"`
 * `hostport = "vk.com:443"`
 *
 *
 * Although the function signature is synchronous DNS resolution may perform file reading and networking.
 * k2-node will attempt to do this work in the background, but the component execution and associated work will be suspended.
 * Thus, this is an expensive operation that should have an asynchronous signature, but it does not.
 *
 * @param `hostport` should be valid utf-8.
 * @return: `0` on success, `errno != 0` otherwise
 * on success:
 * `result_buf_len` is set to the number of resolved addresses. `0 <= result_buf_len <= min(result_buf_len, 128)`.
 * the first `result_buf_len` items of `result_buf` will be filled with resolved addresses
 */
int32_t k2_lookup_host(const char* hostport, size_t hostport_len, struct SockAddr* result_buf, size_t* result_buf_len);

/**
 * Only available during `k2_create_component` call. Returns `0` in other context.
 * Arguments are constants and do not change during the component lifetime.
 * @return number of arguments available for fetching.
 */
uint32_t k2_args_count();

/**
 * @param `arg_num` must satisfy `0 <= arg_num < k2_args_count()`
 * @return length of argument key with number `arg_num` in bytes
 */
uint32_t k2_args_key_len(uint32_t arg_num);

/**
 * @param `arg_num` must satisfy `0 <= arg_num < k2_args_count()`
 * @return length of argument value with number `arg_num` in bytes
 */
uint32_t k2_args_value_len(uint32_t arg_num);

/**
 * Note: key and value are **not** null-terminated.
 * @param `arg_num` must satisfy `0 <= arg_num < k2_args_count()`
 * @param `key` buffer where key will be written, buffer len must staisfy `len >= k2_args_key_len(arg_num)`
 * @param `value` buffer where value will be written, buffer len must staisfy `len >= k2_args_value_len(arg_num)`
 */
void k2_args_fetch(uint32_t arg_num, char* key, char* value);

/**
 * Even if env is available at any point, try to cache and parse it as soon as possible (during` k2_create_component` or even `k2_create_image`).
 * Environment are constant and do not change during the component lifetime.
 * @return number of env key-value pairs available for fetching.
 */
uint32_t k2_env_count();

/**
 * @param `env_num` must satisfy `0 <= env_num < k2_env_count()`
 * @return length of env key with number `env_num` in bytes
 */
uint32_t k2_env_key_len(uint32_t env_num);

/**
 * @param `env_num` must satisfy `0 <= env_num < k2_env_count()`
 * @return length of env value with number `env_num` in bytes
 */
uint32_t k2_env_value_len(uint32_t env_num);

/**
 * Note: key and value are **not** null-terminated.
 * @param `env_num` must satisfy `0 <= env_num < k2_env_count()`
 * @param `key` buffer where key will be written, buffer len must staisfy `len >= k2_env_key_len(env_num)`
 * @param `value` buffer where value will be written, buffer len must staisfy `len >= k2_env_value_len(env_num)`
 */
void k2_env_fetch(uint32_t env_num, char* key, char* value);

// diagnostic functions for instance debugging

/**
 * Get code segment offset of instance in virtual memory.
 * Virtual address minus offset will be equal to insturction offset in dso file
 * @return: `0` on success, `errno != 0` otherwise
 * `errno` options:
 * `ENODATA` => there is no code segment offset for the image
 * `EFAULT`  => attempt to dereference a nullptr
 */
int32_t k2_code_segment_offset(uint64_t* offset);

/**
 * Return symbol name's len that overlaps address
 * @param `addr` pointer to code instruction
 * @param `name_len` buffer where symbol name's len will be written
 * @return: `0` on success, `errno != 0` otherwise
 * `errno` options:
 * `EINVAL`  => `addr` symbol can't be resolved
 * `ENODATA` => there is no debug information for the image
 * `EFAULT`  => attempt to dereference a nullptr
 */
int32_t k2_symbol_name_len(void* addr, size_t* name_len);

/**
 * Return symbol filename's len
 * @param `addr` pointer to code instruction
 * @param `filename_len` buffer where symbol filename's will be written
 * @return: `0` on success, `errno != 0` otherwise
 * `errno` options:
 * `EINVAL`  => `addr` symbol can't be resolved
 * `ENODATA` => there is no debug information for the image
 * `EFAULT`  => attempt to dereference a nullptr
 */
int32_t k2_symbol_filename_len(void* addr, size_t* filename_len);

struct SymbolInfo {
  char* name;
  char* filename;
  uint32_t lineno;
};

/**
 * Returns information about the symbol that overlaps addr and it's position in source code.
 * Note: name and filename in `symbol_info` are **not** null-terminated.
 * @param `addr` pointer to code instruction
 * @param `symbol_info` structure stores buffers where the name and file name will be written. The buffer
 *        lens must satisfy `name_len >= k2_symbol_name_len and filename_len >= k2_symbol_filename_len`
 * `errno` options:
 * `EINVAL`  => `addr` symbol can't be resolved
 * `ENODATA` => there is no debug information for the image
 * `EFAULT`  => attempt to dereference a nullptr
 */
int32_t k2_resolve_symbol(void* addr, struct SymbolInfo* symbol_info);

// ---- libc analogues, designed to work instance-local ----

/**
 * Works instance-local.
 * Analogue of `tzset()`, but accept locale explicitly.
 * @return: `0` on success, non-zero otherwise
 */
int32_t k2_set_timezone(const char* timezone);

/**
 * Works instance-local.
 * Analogue of `localtime_r()`.
 * @return: `result` on success, `NULL` otherwise
 */
struct tm* k2_localtime_r(const time_t* timer, struct tm* result);

/**
 * Works instance-local.
 * Does not return previous locale.
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_uselocale(int32_t category, const char* locale);

/**
 * Works instance-local.
 * Synonym for libc `nl_langinfo(NL_LOCALE_NAME(category))`
 *
 * The returned string is read-only and will be valid until `k2_uselocale` is called;
 *
 * Note: `LC_ALL` is not supported for now :(
 *
 * @return: current locale name.
 * if `category` is invalid, as empty string returned.
 */
char* k2_current_locale_name(int32_t category);

/**
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_iconv_open(void** iconv_cd, const char* tocode, const char* fromcode);

void k2_iconv_close(void* iconv_cd);

/**
 * @return: `0` on success, `errno != 0` otherwise
 */
int32_t k2_iconv(size_t* result, void* iconv_cd, char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft);

/**
 * Writes `data` into stderr. On success, it's guaranteed that
 * the whole input will be written in stderr without interleaving with other data.
 *
 * @return `data_len` on success, `0` otherwise
 */
size_t k2_stderr_write(size_t data_len, const void* data);

/**
 * Returns a backtrace of the calling component
 *
 * @return backtrace size on success, `0` otherwise
 */
size_t k2_backtrace(void** buffer, size_t size);

/**
 * Resolves a path to its canonical, absolute form with:
 * - All intermediate components normalized
 * - All symbolic links resolved
 *
 * Memory management:
 * The resolved path is **not** null-terminated and should be manually freed using `k2_free` or `k2_free_checked`.
 *
 * Possible `errno`:
 * `EINVAL` => any of passed pointers is `NULL`.
 * `ENOMEM` => Out of memory.
 * `ENOSYS` => Internal error.
 * Other => Any `errno` from libc's `realpath`.
 */
int32_t k2_canonicalize(const char* path, size_t pathlen, char* const* resolved_path, size_t* resolved_pathlen, size_t* resolved_path_align);

/**
 * Semantically equivalent to libc's `fstat`.
 */
int32_t k2_fstat(uint64_t fd, struct stat* statbuf);

/**
 * Semantically equivalent to libc's `stat`.
 *
 * Possible `errno`:
 * `EACCES` => Search permission is denied for one of the directories in the path prefix of `pathname`.
 * `EINVAL` => `pathname` or `statbuf` is `NULL`.
 * `EFAULT` => Bad address.
 * `ELOOP` => Too many symbolic links encountered while traversing the path.
 * `ENAMETOOLONG` => `pathname` is too long.
 * `ENOENT` => A component of `pathname` does not exist or is a dangling symbolic link.
 * `ENOMEM` => Out of memory (i.e., kernel memory).
 * `ENOTDIR` => A component of the path prefix of `pathname` is not a directory.
 * `EOVERFLOW` => `pathname` refers to a file whose size, inode number,
 *                or number of blocks cannot be represented in, respectively,
 *                the types `off_t`, `ino_t`, or `blkcnt_t`.  This error can occur
 *                when, for example, an application compiled on a 32-bit
 *                platform without `-D_FILE_OFFSET_BITS=64` calls `stat()` on a
 *                file whose size exceeds `(1<<31)-1` bytes.
 * `ERANGE` => Failed to convert `st_size`, `st_blksize`, or `st_blocks` to `int64_t`.
 * `ENOSYS` => Internal error.
 */
int32_t k2_stat(const char* pathname, size_t pathname_len, struct stat* statbuf);

/**
 * Semantically equivalent to libc's `lstat`.
 *
 * Possible `errno`:
 * `EACCES` => Search permission is denied for one of the directories in the path prefix of `pathname`.
 * `EINVAL` => `pathname` or `statbuf` is `NULL`.
 * `EFAULT` => Bad address.
 * `ELOOP` => Too many symbolic links encountered while traversing the path.
 * `ENAMETOOLONG` => `pathname` is too long.
 * `ENOENT` => A component of `pathname` does not exist or is a dangling symbolic link.
 * `ENOMEM` => Out of memory (i.e., kernel memory).
 * `ENOTDIR` => A component of the path prefix of `pathname` is not a directory.
 * `EOVERFLOW` => `pathname` refers to a file whose size, inode number,
 *                or number of blocks cannot be represented in, respectively,
 *                the types `off_t`, `ino_t`, or `blkcnt_t`.  This error can occur
 *                when, for example, an application compiled on a 32-bit
 *                platform without `-D_FILE_OFFSET_BITS=64` calls `lstat()` on a
 *                file whose size exceeds `(1<<31)-1` bytes.
 * `ERANGE` => Failed to convert `st_size`, `st_blksize`, or `st_blocks` to `int64_t`.
 * `ENOSYS` => Internal error.
 */
int32_t k2_lstat(const char* pathname, size_t pathname_len, struct stat* statbuf);

struct CommandArg {
  const char* arg;
  size_t arg_len;
};

/**
 * Executes a specified command as a child process, waits for it to complete, and optionally captures
 * its exit code and output.
 *
 * This function allocates memory for the output buffer and returns a pointer to this memory, the size and
 * alignment of the memory.The caller is responsible for freeing the allocated memory when it is no longer needed.
 *
 * @param `cmd` The command to be executed.
 * @param `cmd_len` The length of the command.
 * @param `args` A pointer to an array of `CommandArg` structures representing the command-line
 *               arguments to be passed to the command.
 * @param `args_len` The number of elements in the `args` array.
 * @param `exit_code` A pointer to a memory where the exit code of the command will be stored.
 *                    If you do not need the exit code, you can pass `nullptr`.
 * @param `output` A pointer to a memory where the address of the allocated output
 *                 buffer will be stored. The buffer will contain the command's output. If you
 *                 pass `nullptr`, the function will skip capturing the command's output.
 * @param `output_len` A pointer to a memory where the total length of the command's output will be
 *                     stored. If you pass `nullptr`, the function will skip capturing the command's
 *                     output.
 * @param `output_align` A pointer to a memory where the alignment of the output will be stored. If
 *                       you pass `nullptr`, the function will skip capturing the command's output.
 *
 * @return The status of the command execution. A return value of 0 indicates success, while a non-zero
 * value indicates an error.
 *
 * @note This function is more likely to be temporary and may be replaced by a more generic one
 *       in the future. Consider this when integrating it into your application.
 *
 * @note The function blocks until the command has finished executing.
 *
 * @note The caller is responsible for freeing the allocated output buffer using `k2_free` or `k2_free_checked`
 * to prevent memory leaks.
 *
 * Possible `errno`:
 * `EINVAL` => `cmd` or `args.arg` is `NULL`.
 * `ENOMEM` => Out of memory (i.e., kernel memory).
 * `ENOSYS` => Internal error.
 *
 */
int32_t k2_command(const char* cmd, size_t cmd_len, const struct CommandArg* args, size_t args_len, int32_t* exit_code, char* const* output, size_t* output_len,
                   size_t* output_align);

#ifdef __cplusplus
}
#endif
#endif
