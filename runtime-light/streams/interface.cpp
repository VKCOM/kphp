#include "runtime-light/streams/interface.h"

#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/streams/streams.h"

int64_t f$open(const string & name) {
  uint64_t stream_d;
  OpenStreamResult res = get_platform_context()->open(name.size(), name.c_str(), &stream_d);
  if (res != OpenStreamOk) {
    php_error("cannot open stream");
    panic();
  }
  return stream_d;
}

task_t<void> f$send(int64_t id, const string & message) {
  co_await write_all_to_stream(id, message.c_str(), message.size());
}

task_t<string> f$read(int64_t id) {
  auto [buffer, size] = co_await read_all_from_stream(id);
  string str;
  str.assign(buffer, size);
  get_platform_allocator()->free(buffer);
  co_return str;
}

void f$close(int64_t id) {
  get_platform_context()->shutdown_write(id);
}
