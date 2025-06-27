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
  auto& output_instance_st{OutputInstanceState::get()};
  const auto opt_user_buffer{output_instance_st.output_buffers.user_buffer()};
  if (!opt_user_buffer.has_value()) {
    return false;
  }
  return (*opt_user_buffer).get().size();
}

inline int64_t f$ob_get_level() noexcept {
  const auto& output_instance_st{OutputInstanceState::get()};
  return output_instance_st.output_buffers.user_level();
}

inline void f$ob_clean() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  output_instance_st.output_buffers.current_buffer().get().clean();
}

inline bool f$ob_end_clean() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  const auto opt_user_buffer{output_instance_st.output_buffers.user_buffer()};
  if (!opt_user_buffer.has_value()) [[unlikely]] {
    return false;
  }

  output_instance_st.output_buffers.prev_user_buffer();
  auto& http_server_instance_st{HttpServerInstanceState::get()};
  http_server_instance_st.encoding &= ~HttpServerInstanceState::ENCODING_GZIP;
  return true;
}

inline Optional<string> f$ob_get_clean() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  const auto opt_user_buffer{output_instance_st.output_buffers.user_buffer()};
  if (!opt_user_buffer.has_value()) [[unlikely]] {
    return false;
  }

  string result{(*opt_user_buffer).get().str()};
  output_instance_st.output_buffers.prev_user_buffer();
  auto& http_server_instance_st{HttpServerInstanceState::get()};
  http_server_instance_st.encoding &= ~HttpServerInstanceState::ENCODING_GZIP;
  return result;
}

inline string f$ob_get_contents() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  return output_instance_st.output_buffers.current_buffer().get().str();
}

inline void f$ob_flush() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  const auto opt_user_buffer{output_instance_st.output_buffers.user_buffer()};
  if (!opt_user_buffer.has_value()) [[unlikely]] {
    kphp::log::warning("ob_flush called without opened buffers");
    return;
  }

  output_instance_st.output_buffers.prev_user_buffer();
  print((*opt_user_buffer).get());
  kphp::log::assertion(output_instance_st.output_buffers.next_user_buffer().has_value());
}

inline bool f$ob_end_flush() noexcept {
  f$ob_flush();
  return f$ob_end_clean();
}

inline Optional<string> f$ob_get_flush() noexcept {
  auto& output_instance_st{OutputInstanceState::get()};
  const auto opt_user_buffer{output_instance_st.output_buffers.user_buffer()};
  if (!opt_user_buffer.has_value()) [[unlikely]] {
    return false;
  }

  string result{(*opt_user_buffer).get().str()};
  f$ob_flush();
  f$ob_end_clean();
  return result;
}
