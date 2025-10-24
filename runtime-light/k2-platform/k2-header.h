#ifndef K2_PLATFORM_HEADER_H
#define K2_PLATFORM_HEADER_H

#pragma once

#ifndef K2_API_HEADER_H
#error "should not be directly included"
#endif // K2_API_HEADER_H

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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#endif

#define K2_PLATFORM_HEADER_H_VERSION 12

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

struct ImageInfo {
  // TODO: null terminated string is OK?
  // TODO: namespaces?
  const char* image_name;

  uint64_t build_timestamp;
  uint64_t header_h_version;
  uint8_t commit_hash[40];
  // TODO: more informative?
  uint8_t compiler_hash[64];
  // bool
  uint8_t is_oneshot;
};

/**
 * Symbols provided by component image
 * Every image should provide these symbols
 */
enum PollStatus k2_poll();

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
 * Semantically equivalent of libc's unlink
 *
 *  Some `errno` examples:
 * `EACCES` => permission denied.
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
int32_t k2_access(size_t name_len, const char* name);

/**
 * If the write or read status is `Blocked` - then the platform ensures that
 * the component receives this `stream_d` via `k2_take_update` when the status is
 * no longer `Blocked` ("edge-triggered epoll"-like behaviour).
 *
 * `status` will be filled with actual descriptor status on of success.
 * `s->libc_errno` used to represent errors.
 *
 * Some `errno` examples:
 * `EBADF` => d is not valid(never was valid or used after free)
 * `EBADR` => d is valid descriptor, but not a stream (probably, timer)
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
