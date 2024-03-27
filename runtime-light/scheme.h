#ifndef K2_PLATFORM_HEADER_H
#define K2_PLATFORM_HEADER_H


#include <stdint.h>
#include <string.h>
#include <atomic>

#define K2_PLATFORM_HEADER_H_VERSION 1

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
};

enum GetStatusResult {
  GetStatusOk = 0,
  GetStatusErrorNotStream = 1,
  GetStatusErrorUseAfterFree = 2,
  GetStatusErrorNoSuchStream = 3,
};

enum SetTimerResult {
  SetTimerSuccess = 0,
  SetTimerLimitExceeded = 1,
};

enum TimerStatus {
  TimerStatusScheduled = 0,
  TimerStatusElapsed = 1,
  TimerStatusInvalid = 2,
};

enum OpenStreamResult {
  OpenStreamOk = 0,
  // TODO: really need error? MB it's better to open and immediately close
  // channel with corresponding error
  OpenStreamErrorInvalidName = 1,
  OpenStreamErrorUnknownComponent = 3,
  OpenStreamErrorComponentUnavailable = 4,
  OpenStreamErrorLimitExceeded = 5,
};

// This time point is valid only within the component.
// Similar to c++ `std::chrono::steady_clock::time_point`
struct TimePoint {
  uint64_t time_point_ns;
};

struct SystemTime {
  uint64_t since_epoch_ns;
};

struct TimerElapsed {
  uint64_t timer_d;
  uint64_t overtime_ns;
};

struct PlatformCtx {
  std::atomic<uint32_t> please_yield;

  struct Allocator allocator;

  /*
   * In case of `result != Ok`:
   * `stream_d` will be asigined `0`.
   * however `stream_d=0` itself is not an error marker
   */
  enum OpenStreamResult (*open)(size_t name_len, const char *name,
                                uint64_t *stream_d);
  /*
   * In case of `result != Ok`:
   * `new_status` will be asigined as
   * `{.read_status = 0, .write_status = 0, .please_shutdown = 0}`.
   */
  enum GetStatusResult (*get_stream_status)(uint64_t stream_d,
                                            struct StreamStatus *new_status);
  /*
   * return processed bytes(write or read).
   * return `0` if stream `Closed`, `Blocked` or `stream_d` is invalid.
   * may return `0` even if stream is `Available`
   *
   * The bytes processed may be less than `data_len`,
   * even if the stream in `Available` state after the operation.
   * Therefore, it does not indicate `Blocked` or `Closed` status.
   * Use `get_stream_status` to check how to start co-scheduling.
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
   * Returns Stream descriptor into platform. All future uses of this `stream_d`
   * will be invalid.
   * 1) If you are the owner of the stream (performed an `open`) -
   * the read and write buffers will be immediately deleted. The opposite side
   * will receive an `IOClosed` notification in both directions.
   * 2) If you are not the owner, the opposite side will receive a notification
   * `write_status = IOClosed`; Data sent from the opposite side and remaining
   * in buffer will be deleted. The opposite side will be able to read the data
   * sent from you and remaining in the buffer stream.
   */
  void (*free_stream)(uint64_t stream_d);

  // time since epoch, non-monotonical
  void (*get_system_time)(struct SystemTime *system_time);

  // Monotonical, for timeouts, measurements, etc..
  void (*get_time)(struct TimePoint *time_point);
  enum SetTimerResult (*set_timer)(uint64_t *timer_d, uint64_t duration_ns);
  enum TimerStatus (*get_timer_status)(uint64_t timer_d,
                                       struct TimePoint *deadline);

  /*
   * Ensures that no notification from this timer will be received. All futures
   * uses of this `timer_d` will be invalid.
   */
  void (*cancel_timer)(uint64_t timer_d);

  /*
   * Return: `bool`.
   * If `True`: the update was successfully received.
   * If `False`: no updates, `update_d` will be set to `0`.
   *
   * `update_d` might be either a stream or a timer.
   * `update_d` can be a new stream installed for this component.
   * Therefore, component should keep track of all active descriptors to
   * distinguish new stream from existing one.
   */
  char (*take_update)(uint64_t *update_d);
};

struct ComponentState;
/*
 * image state, created once on library load
 * shared between all component [instances].
 * designed to prevent heavy `_init` section of dlib
 */
struct ImageState;

enum PollStatus {
  // waitings IO or timer; no cpu work remains
  PollBlocked = 0,
  // there is some cpu work to do; platform will reschedule component
  PollReschedule = 1,
  // component decide to shutdown
  PollFinished = 2,
};

struct ImageInfo {
  // TODO: null terminated string is OK?
  // TODO: namespaces?
  const char *image_name;

  uint64_t build_timestamp;
  uint64_t header_h_version;
  uint8_t commit_hash[40];
};

// Every image should provide these symbols
enum PollStatus vk_k2_poll(const struct ImageState *image_state,
                           const struct PlatformCtx *pt_ctx,
                           struct ComponentState *component_ctx);
struct ComponentState *
vk_k2_create_component_state(const struct ImageState *image_state,
                             const struct Allocator *alloc);
struct ImageState *vk_k2_create_image_state(const struct Allocator *alloc);

const struct ImageInfo *vk_k2_describe();

typedef enum PollStatus (*PollSig)(const struct ImageState *image_state,
                                   const struct PlatformCtx *pt_ctx,
                                   struct ComponentState *component_state);
typedef struct ComponentState *(*CreateComponentStateSig)(
  const struct ImageState *image_state, const struct Allocator *alloc);
typedef struct ImageState *(*CreateImageStateSig)(
  const struct Allocator *alloc);
typedef const struct ImageInfo *(*DescribeSig)();

#ifdef __cplusplus
}
#endif
#endif
