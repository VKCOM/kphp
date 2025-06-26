// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/stdlib/output/print-functions.h"
#include "runtime-light/utils/logs.h"

void f$ob_start(const string& callback = {}) noexcept;

inline Optional<int64_t> f$ob_get_length() noexcept {
  const auto& output_buffers{OutputInstanceState::get().output_buffers};
  return output_buffers.level() == 0 ? false : output_buffers.current_buffer().get().size();
}

inline int64_t f$ob_get_level() noexcept {
  const auto& output_instance_st{OutputInstanceState::get()};
  return output_instance_st.output_buffers.level();
}

inline void f$ob_clean() noexcept {
  const auto& output_instance_st{OutputInstanceState::get()};
  output_instance_st.output_buffers.current_buffer().get().clean();
}

inline bool f$ob_end_clean() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  if (!output_instance_st.output_buffers.prev_buffer().has_value()) [[unlikely]] {
    return false;
  }

  auto& http_server_instance_st{HttpServerInstanceState::get()};
  http_server_instance_st.encoding &= ~HttpServerInstanceState::ENCODING_GZIP;
  return true;
}

inline Optional<string> f$ob_get_clean() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  if (output_instance_st.output_buffers.level() == 0) [[unlikely]] {
    return false;
  }

  string result{output_instance_st.output_buffers.current_buffer().get().str()};
  kphp::log::assertion(output_instance_st.output_buffers.prev_buffer().has_value());
  auto& http_server_instance_st{HttpServerInstanceState::get()};
  http_server_instance_st.encoding &= ~HttpServerInstanceState::ENCODING_GZIP;
  return result;
}

inline string f$ob_get_contents() noexcept {
  const auto& output_instance_st{OutputInstanceState::get()};
  return output_instance_st.output_buffers.current_buffer().get().str();
}

inline void f$ob_flush() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  const auto current_buffer{output_instance_st.output_buffers.current_buffer()};
  const auto opt_prev_buffer{output_instance_st.output_buffers.prev_buffer()};
  if (!opt_prev_buffer.has_value()) [[unlikely]] {
    kphp::log::warning("ob_flush called without opened buffers");
    return;
  }
  print(current_buffer);
  kphp::log::assertion(output_instance_st.output_buffers.next_buffer().has_value());
  f$ob_clean();
}

inline bool f$ob_end_flush() noexcept {
  return (f$ob_flush(), f$ob_end_clean());
}

inline Optional<string> f$ob_get_flush() noexcept {
  const auto& output_instance_st{OutputInstanceState::get()};
  if (output_instance_st.output_buffers.level() == 0) [[unlikely]] {
    return false;
  }

  string result{output_instance_st.output_buffers.current_buffer().get().str()};
  f$ob_flush();
  f$ob_end_clean();
  return result;
}
