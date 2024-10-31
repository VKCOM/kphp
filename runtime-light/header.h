// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef K2_PLATFORM_HEADER_H
#define K2_PLATFORM_HEADER_H

#pragma once

#ifdef __cplusplus
#include <atomic>
#include <cstdint>
#include <cstring>
#else
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#endif

#define K2_PLATFORM_HEADER_H_VERSION 8

// Always check that enum value is a valid value!

#ifdef __cplusplus
extern "C" {
#endif

struct Allocator {
  // TODO zeroing or not?
  void *(*alloc)(size_t);
  void (*free)(void *);
  void *(*realloc)(void *ptr, size_t);
};

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

enum GetStatusResult {
  GetStatusOk = 0,
  GetStatusErrorNotStream = 1,
  GetStatusErrorUseAfterFree = 2,
  GetStatusErrorNoSuchStream = 3,
};

enum SetTimerResult {
  SetTimerOk = 0,
  SetTimerErrorLimitExceeded = 1,
};

enum TimerStatus {
  TimerStatusScheduled = 0,
  TimerStatusElapsed = 1,
  TimerStatusInvalid = 2,
};

enum OpenStreamResult {
  OpenStreamOk = 0,
  /*
   * TODO: really need error? MB it's better to open and immediately close
   * channel with corresponding error
   */
  OpenStreamErrorInvalidName = 1,
  OpenStreamErrorUnknownComponent = 3,
  OpenStreamErrorComponentUnavailable = 4,
  OpenStreamErrorLimitExceeded = 5,
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

// DEPRECATED:
struct PlatformCtx {
  atomic_uint32_t please_yield;
  atomic_uint32_t please_graceful_shutdown;

  /*
   * Immediately abort component execution.
   * Function is `[[noreturn]]`
   * Note: `exit_code` used just as indicator for now.
   * `exit_code` == 0 => FinishedOk,
   * `exit_code` != 0 => FinishedError,
   */
  void (*exit)(int32_t exit_code);

  // Deprecated; Synonym for `exit(255)`;
  void (*abort)();

  struct Allocator allocator;

  /*
   * In case of `result == Ok` stream_d will be NonZero
   *
   * In case of `result != Ok`:
   * `stream_d` will be assigned `0`.
   * however `stream_d=0` itself is not an error marker
   */
  enum OpenStreamResult (*open)(size_t name_len, const char *name, uint64_t *stream_d);
  /*
   * If the write or read status is `Blocked` - then the platform ensures that
   * the component receives this `stream_d` via `take_update` when the status is
   * no longer `Blocked`
   *
   * In case of `result != Ok`:
   * `new_status` will be assigned as
   * `{.read_status = 0, .write_status = 0, .please_shutdown = 0}`.
   */
  enum GetStatusResult (*get_stream_status)(uint64_t stream_d, struct StreamStatus *new_status);
  /*
   * Return processed bytes (written or read).
   * Guaranteed to return `0` if  the stream is `Closed`, `Blocked` or
   * `stream_d` is invalid.
   * May return `0` even if the stream is `Available`
   *
   * The bytes processed may be less than `data_len`,
   * even if the stream is in `Available` state after the operation.
   * Therefore, it does not indicate `Blocked` or `Closed` status.
   * Use `get_stream_status` to check how to perform co-scheduling.
   */
  size_t (*write)(uint64_t stream_d, size_t data_len, const void *data);
  size_t (*read)(uint64_t stream_d, size_t data_len, void *data);

  /*
   * Sets `StreamStatus.please_whutdown_write=true` for the component on the
   * opposite side (does not affect `StreamStatus` on your side).
   * Does not disable the ability to read from the stream.
   * To perform a graceful shutdown, you still need to read data from the stream
   * as long as `read_status != IOClosed`.
   */
  void (*please_shutdown_write)(uint64_t stream_d);

  /*
   * Disables the ability to write to a stream.
   * Data written to the stream buffer is still available for reading on the
   * opposite side.
   * Does not affect the ability to read from the stream.
   *
   * TODO: design information errors.
   */
  void (*shutdown_write)(uint64_t stream_d);

  /*
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
   * Noop in case if `descriptor` is invalid
   */
  void (*free_descriptor)(uint64_t descriptor);

  // Time since epoch, non-monotonical
  void (*get_system_time)(struct SystemTime *system_time);

  // Coordinated with timers. Monotonical, for timeouts, measurements, etc..
  void (*get_time)(struct TimePoint *time_point);

  /*
   * In case of `result == Ok` timer_d will be NonZero
   * In case of `result != Ok` timer_d will be `0`
   *
   * One-shot timer
   * Use `free_descriptor` to cancel
   */
  enum SetTimerResult (*set_timer)(uint64_t *timer_d, uint64_t duration_ns);

  /*
   * It is guaranteed that if `TimerStatusElapsed` is returned
   * then `deadline <= get_time()`
   * There is no symmetric guarantee for `TimerStatusScheduled`.
   *
   * `deadline` will be assigned `0` if `timer_d` invalid
   */
  enum TimerStatus (*get_timer_status)(uint64_t timer_d, struct TimePoint *deadline);
  /*
   * Return: `bool`.
   * If `True`: the update was successfully received.
   * If `False`: no updates, `update_d` will be set to `0`.
   *
   * `update_d` might be either a stream or a timer.
   * `update_d` can be a new stream installed for this component.
   * Therefore, component should keep track of all active descriptors to
   * distinguish new stream from existing one.
   *
   * `update_d` may be an elapsed timer.
   * `get_timer_status` for this timer will be a valid call and will allow you
   * to get its deadline (which is guaranteed to be in the past).
   * But you must call `free_descriptor` to release the associated descriptor.
   *
   * If a component has not read all updates during a `poll` iteration, the
   * platform is guaranteed to reschedule it.
   */
  uint8_t (*take_update)(uint64_t *update_d);

  /*
   * Only utf-8 string supported.
   * Possible `level` values:
   *    1 => Error
   *    2 => Warn
   *    3 => Info
   *    4 => Debug
   *    5 => Trace
   * Any other value will cause the log to be skipped
   * if `level` > `log_level_enabled()` log will be skipped
   */
  void (*log)(size_t level, size_t len, const char *str);

  // Use for optimization, see `log`
  size_t (*log_level_enabled)();

  // Note: prefer to use only as seed generator for pseudo-random
  void (*os_rnd)(size_t len, void *bytes);
};

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

struct ImageInfo {
  // TODO: null terminated string is OK?
  // TODO: namespaces?
  const char *image_name;

  uint64_t build_timestamp;
  uint64_t header_h_version;
  uint8_t commit_hash[40];
  // TODO: more informative?
  uint8_t compiler_hash[64];
  // bool
  uint8_t is_oneshot;
};

///
// Symbols provided by component image
// Every image should provide these symbols
///
enum PollStatus vk_k2_poll(const struct ImageState *image_state, const struct PlatformCtx *pt_ctx, struct ComponentState *component_ctx);

/*
 * platform_ctx without IO stuff (nullptr instead io-functions)
 * for now, returning nullptr will indicate error
 */
struct ComponentState *vk_k2_create_component_state(const struct ImageState *image_state, const struct PlatformCtx *pt_ctx);
struct ImageState *vk_k2_create_image_state(const struct PlatformCtx *pt_ctx);

const struct ImageInfo *vk_k2_describe();

struct ControlFlags {
  atomic_uint32_t please_yield;
  atomic_uint32_t please_graceful_shutdown;
};

/*
 *
 * Symbols provided by k2-node and resolved by linker during dlopen.
 * Every image can use these symbols.
 *
 * Semantically analogs for `PlatformCtx` methods sometimes with small
 * differences. Preferable way to communication with k2-node.
 *
 */
const struct ControlFlags *k2_control_flags();
const struct ImageState *k2_image_state();
struct ComponentState *k2_component_state();

void *k2_alloc(size_t size, size_t align);
void *k2_realloc(void *ptr, size_t new_size);
void *k2_realloc_checked(void *ptr, size_t old_size, size_t align, size_t new_size);
void k2_free(void *ptr);
void k2_free_checked(void *ptr, size_t size, size_t align);

void k2_exit(int32_t exit_code);

// Difference with PlatformCtx:
// This function return `libc errno` (`0` on success)
//
// Some examples:
// `EINVAL` => `name` has invalid format (empty, non ascii, too long, etc..)
// `ECONNREFUSED` => no component with such name found
// `ENOMEM` => our compoonent has no enough memory for stream
// `EACCES` => permission denied
//
int32_t k2_open(uint64_t *d, size_t name_len, const char *name);

// Difference with PlatformCtx:
// This function does not return status.
// But it will set `s->libc_errno` to corresponding errors:
// `EBADF` => d is not valid(never was valid or used after free)
// `EBADR` => d is valid descriptor, but not a stream (probably, timer)
//
void k2_stream_status(uint64_t d, struct StreamStatus *s);

size_t k2_write(uint64_t d, size_t data_len, const void *data);
size_t k2_read(uint64_t d, size_t buf_len, void *buf);

void k2_please_shutdown(uint64_t d);
void k2_shutdown_write(uint64_t d);

void k2_instant(struct TimePoint *time_point);

// Difference with PlatformCtx:
// This function return `libc errno` (`0` on success)
//
// Probably this function newer fails.
// Looks like `ENOMEM` is only reasonable error.
// I think component prefer abortion in this case.
//
int32_t k2_new_timer(uint64_t *d, uint64_t duration_ns);

// Difference with PlatformCtx:
// This function return `libc errno` (`0` on success)
//
// `EBADF` => d is not valid(never was valid or used after free)
// `EBADR` => d is valid descriptor, but not a timer (probably stream)
//
int32_t k2_timer_deadline(uint64_t d, struct TimePoint *deadline);

void k2_free_descriptor(uint64_t d);

uint8_t k2_take_update(uint64_t *d);

void k2_log(size_t level, size_t len, const char *str);
size_t k2_log_level_enabled();

void k2_os_rnd(size_t len, void *bytes);

void k2_system_time(struct SystemTime *system_time);

#ifdef __cplusplus
}
#endif
#endif
