// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/output-control.h"

#include "runtime-light/component/component.h"
#include "runtime-light/stdlib/string-functions.h"

static constexpr int system_level_buffer = 0;

void f$ob_clean() {
  Response &httpResponse = get_component_context()->response;
  httpResponse.output_buffers[httpResponse.current_buffer].clean();
}

bool f$ob_end_clean() {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer == system_level_buffer) {
    return false;
  }

  --httpResponse.current_buffer;
  return true;
}

Optional<string> f$ob_get_clean() {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer == system_level_buffer) {
    return false;
  }

  string result = httpResponse.output_buffers[httpResponse.current_buffer].str();
  return result;
}

string f$ob_get_content() {
  Response &httpResponse = get_component_context()->response;
  return httpResponse.output_buffers[httpResponse.current_buffer].str();
}

void f$ob_start(const string&callback) {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer + 1 == httpResponse.ob_max_buffers) {
    php_warning("Maximum nested level of output buffering reached. Can't do ob_start(%s)", callback.c_str());
    return;
  }

  if (!callback.empty()) {
    php_critical_error ("unsupported callback %s at buffering level %d", callback.c_str(), httpResponse.current_buffer + 1);
  }

  ++httpResponse.current_buffer;
}

void f$ob_flush() {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer == 0) {
    php_warning("ob_flush with no buffer opented");
    return;
  }
  --httpResponse.current_buffer;
  print(httpResponse.output_buffers[httpResponse.current_buffer + 1]);
  ++httpResponse.current_buffer;
  f$ob_clean();
}

bool f$ob_end_flush() {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer == 0) {
    return false;
  }
  f$ob_flush();
  return f$ob_end_clean();
}

Optional<string> f$ob_get_flush() {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer == 0) {
    return false;
  }
  string result = httpResponse.output_buffers[httpResponse.current_buffer].str();
  f$ob_flush();
  f$ob_end_clean();
  return result;
}

Optional<int64_t> f$ob_get_length() {
  Response &httpResponse = get_component_context()->response;
  if (httpResponse.current_buffer == 0) {
    return false;
  }
  return httpResponse.output_buffers[httpResponse.current_buffer].size();
}

int64_t f$ob_get_level() {
  Response &httpResponse = get_component_context()->response;
  return httpResponse.current_buffer;
}
