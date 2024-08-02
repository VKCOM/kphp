// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/streams/interface.h"

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/header.h"
#include "runtime-light/stdlib/misc.h"
#include "runtime-light/streams/component-stream.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/utils/context.h"

task_t<void> f$component_get_http_query() {
  std::ignore = co_await wait_and_process_incoming_stream(QueryType::HTTP);
}

task_t<class_instance<C$ComponentQuery>> f$component_client_send_query(string name, string message) {
  class_instance<C$ComponentQuery> query;
  const auto stream_d{get_component_context()->open_stream(name)};
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    php_warning("can't send client query");
    co_return query;
  }

  int32_t written{co_await write_all_to_stream(stream_d, message.c_str(), message.size())};
  get_platform_context()->shutdown_write(stream_d);
  query.alloc();
  query.get()->stream_d = stream_d;
  php_debug("send %d bytes from %d to \"%s\" on stream %" PRIu64, written, message.size(), name.c_str(), stream_d);
  co_return query;
}

task_t<string> f$component_client_get_result(class_instance<C$ComponentQuery> query) {
  php_assert(!query.is_null());
  uint64_t stream_d{query.get()->stream_d};
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    php_warning("can't get component client result");
    co_return string{};
  }

  const auto [buffer, size]{co_await read_all_from_stream(stream_d)};
  string result{buffer, static_cast<string::size_type>(size)};
  get_platform_context()->allocator.free(buffer);
  get_component_context()->release_stream(stream_d);
  query.get()->stream_d = INVALID_PLATFORM_DESCRIPTOR;
  php_debug("read %d bytes from stream %" PRIu64, size, stream_d);
  co_return result;
}

task_t<void> f$component_server_send_result(string message) {
  auto &component_ctx{*get_component_context()};
  const auto standard_stream{component_ctx.standard_stream()};
  if (!co_await write_all_to_stream(standard_stream, message.c_str(), message.size())) {
    php_warning("can't send component result");
  } else {
    php_debug("send result \"%s\"", message.c_str());
  }
  component_ctx.release_stream(standard_stream);
}

task_t<string> f$component_server_get_query() {
  const auto incoming_stream_d{co_await wait_and_process_incoming_stream(QueryType::COMPONENT)};
  const auto [buffer, size] = co_await read_all_from_stream(incoming_stream_d);
  string result{buffer, static_cast<string::size_type>(size)};
  get_platform_context()->allocator.free(buffer);
  co_return result;
}

task_t<class_instance<C$ComponentStream>> f$component_accept_stream() {
  const auto incoming_stream_d{co_await wait_and_process_incoming_stream(QueryType::COMPONENT)};
  class_instance<C$ComponentStream> stream;
  stream.alloc();
  stream.get()->stream_d = incoming_stream_d;
  co_return stream;
}

class_instance<C$ComponentStream> f$component_open_stream(const string &name) {
  auto &component_ctx = *get_component_context();

  class_instance<C$ComponentStream> query;
  const auto stream_d{component_ctx.open_stream(name)};
  if (stream_d == INVALID_PLATFORM_DESCRIPTOR) {
    return query;
  }
  query.alloc();
  query.get()->stream_d = stream_d;
  return query;
}

int64_t f$component_stream_write_nonblock(const class_instance<C$ComponentStream> &stream, const string &message) {
  return write_nonblock_to_stream(stream.get()->stream_d, message.c_str(), message.size());
}

string f$component_stream_read_nonblock(const class_instance<C$ComponentStream> &stream) {
  const auto [buffer, size] = read_nonblock_from_stream(stream.get()->stream_d);
  string result{buffer, static_cast<string::size_type>(size)};
  get_platform_context()->allocator.free(buffer); // FIXME: do we need platform memory?
  return result;
}

task_t<int64_t> f$component_stream_write_exact(class_instance<C$ComponentStream> stream, string message) {
  const auto written = co_await write_exact_to_stream(stream->stream_d, message.c_str(), message.size());
  php_debug("wrote exact %d bytes to stream %" PRIu64, written, stream->stream_d);
  co_return written;
}

task_t<string> f$component_stream_read_exact(class_instance<C$ComponentStream> stream, int64_t len) {
  auto *buffer = static_cast<char *>(RuntimeAllocator::current().alloc_script_memory(len));
  const auto read = co_await read_exact_from_stream(stream->stream_d, buffer, len);
  string result{buffer, static_cast<string::size_type>(read)};
  RuntimeAllocator::current().free_script_memory(buffer, len);
  php_debug("read exact %d bytes from stream %" PRIu64, read, stream->stream_d);
  co_return result;
}

void f$component_close_stream(const class_instance<C$ComponentStream> &stream) {
  get_component_context()->release_stream(stream.get()->stream_d);
  stream.get()->stream_d = INVALID_PLATFORM_DESCRIPTOR; // TODO: convert stream object to null?
}

void f$component_finish_stream_processing(const class_instance<C$ComponentStream> &stream) {
  auto &component_ctx = *get_component_context();
  if (stream.get()->stream_d != component_ctx.standard_stream()) {
    php_warning("call server finish query on non server stream %lu", stream.get()->stream_d);
    return;
  }
  component_ctx.release_stream(component_ctx.standard_stream());
  stream.get()->stream_d = INVALID_PLATFORM_DESCRIPTOR;
}
