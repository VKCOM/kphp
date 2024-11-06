// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/output/output-buffer.h"

#include <cstdint>

#include "runtime-light/component/component.h"
#include "runtime-light/stdlib/output/print-functions.h"

static constexpr int32_t system_level_buffer = 0;

void f$ob_start(const string &callback) noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer + 1 == Response::ob_max_buffers) {
    php_warning("Maximum nested level of output buffering reached. Can't do ob_start(%s)", callback.c_str());
    return;
  }

  if (!callback.empty()) {
    php_critical_error("unsupported callback %s at buffering level %d", callback.c_str(), httpResponse.current_buffer + 1);
  }
  ++httpResponse.current_buffer;
}

Optional<int64_t> f$ob_get_length() noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer == 0) {
    return false;
  }
  return httpResponse.output_buffers[httpResponse.current_buffer].size();
}

int64_t f$ob_get_level() noexcept {
  return InstanceState::get().response.current_buffer;
}

void f$ob_clean() noexcept {
  Response &httpResponse{InstanceState::get().response};
  httpResponse.output_buffers[httpResponse.current_buffer].clean();
}

bool f$ob_end_clean() noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer == system_level_buffer) {
    return false;
  }

  --httpResponse.current_buffer;
  return true;
}

Optional<string> f$ob_get_clean() noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer == system_level_buffer) {
    return false;
  }
  return httpResponse.output_buffers[httpResponse.current_buffer].str();
}

string f$ob_get_contents() noexcept {
  Response &httpResponse{InstanceState::get().response};
  return httpResponse.output_buffers[httpResponse.current_buffer].str();
}

void f$ob_flush() noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer == 0) {
    php_warning("ob_flush with no buffer opented");
    return;
  }
  --httpResponse.current_buffer;
  print(httpResponse.output_buffers[httpResponse.current_buffer + 1]);
  ++httpResponse.current_buffer;
  f$ob_clean();
}

bool f$ob_end_flush() noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer == 0) {
    return false;
  }
  f$ob_flush();
  return f$ob_end_clean();
}

Optional<string> f$ob_get_flush() noexcept {
  Response &httpResponse{InstanceState::get().response};
  if (httpResponse.current_buffer == 0) {
    return false;
  }
  string result{httpResponse.output_buffers[httpResponse.current_buffer].str()};
  f$ob_flush();
  f$ob_end_clean();
  return result;
}
