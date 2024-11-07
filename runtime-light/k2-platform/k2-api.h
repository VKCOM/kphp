// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#define K2_API_HEADER_H
#include "runtime-light/k2-platform/k2-header.h"
#undef K2_API_HEADER_H

namespace k2 {

namespace k2_impl_ {

inline constexpr size_t DEFAULT_MEMORY_ALIGN = 16;

} // namespace k2_impl_

inline constexpr int32_t errno_ok = 0;

using IOStatus = IOStatus;

using StreamStatus = StreamStatus;

using TimePoint = TimePoint;

using SystemTime = SystemTime;

using PollStatus = PollStatus;

using ImageInfo = ImageInfo;

using ControlFlags = ControlFlags;

inline const ImageInfo *describe() noexcept {
  return k2_describe();
}

inline const ControlFlags *control_flags() noexcept {
  return k2_control_flags();
}

inline const ImageState *image_state() noexcept {
  return k2_image_state();
}

// inline const ComponentState *component_state() noexcept {
//   return k2_component_state();
// }

inline InstanceState *instance_state() noexcept {
  return k2_instance_state();
}

inline void *alloc_align(size_t size, size_t align) noexcept {
  return k2_alloc(size, align);
}

inline void *alloc(size_t size) noexcept {
  return alloc_align(size, k2_impl_::DEFAULT_MEMORY_ALIGN);
}

inline void *realloc(void *ptr, size_t new_size) noexcept {
  return k2_realloc(ptr, new_size);
}

inline void *realloc_checked(void *ptr, size_t old_size, size_t align, size_t new_size) noexcept {
  return k2_realloc_checked(ptr, old_size, align, new_size);
}

inline void free(void *ptr) noexcept {
  k2_free(ptr);
}

inline void free_checked(void *ptr, size_t size, size_t align) noexcept {
  k2_free_checked(ptr, size, align);
}

[[noreturn]] inline void exit(int32_t exit_code) noexcept {
  k2_exit(exit_code);
}

inline int32_t open(uint64_t *stream_d, size_t name_len, const char *name) noexcept {
  return k2_open(stream_d, name_len, name);
}

inline void stream_status(uint64_t stream_d, StreamStatus *status) noexcept {
  k2_stream_status(stream_d, status);
}

inline size_t write(uint64_t stream_d, size_t data_len, const void *data) noexcept {
  return k2_write(stream_d, data_len, data);
}

inline size_t read(uint64_t stream_d, size_t buf_len, void *buf) noexcept {
  return k2_read(stream_d, buf_len, buf);
}

inline void please_shutdown(uint64_t stream_d) noexcept {
  k2_please_shutdown(stream_d);
}

inline void shutdown_write(uint64_t stream_d) noexcept {
  k2_shutdown_write(stream_d);
}

inline void instant(TimePoint *time_point) noexcept {
  k2_instant(time_point);
}

inline int32_t new_timer(uint64_t *descriptor, uint64_t duration_ns) noexcept {
  return k2_new_timer(descriptor, duration_ns);
}

inline int32_t timer_deadline(uint64_t d, TimePoint *deadline) noexcept {
  return k2_timer_deadline(d, deadline);
}

inline void free_descriptor(uint64_t descriptor) noexcept {
  k2_free_descriptor(descriptor);
}

inline uint8_t take_update(uint64_t *update_d) noexcept {
  return k2_take_update(update_d);
}

inline void log(size_t level, size_t len, const char *str) noexcept {
  k2_log(level, len, str);
}

inline size_t log_level_enabled() noexcept {
  return k2_log_level_enabled();
}

inline void os_rnd(size_t len, void *bytes) noexcept {
  k2_os_rnd(len, bytes);
}

inline void system_time(SystemTime *system_time) noexcept {
  k2_system_time(system_time);
}

} // namespace k2
