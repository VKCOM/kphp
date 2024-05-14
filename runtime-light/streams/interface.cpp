#include "runtime-light/streams/interface.h"


#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/streams/streams.h"
#include "runtime-light/stdlib/misc.h"

task_t<void> f$parse_http_query() {
  co_await parse_input_query(HTTP);
}

task_t<class_instance<C$ComponentQuery>> f$component_client_send_query(const string &name, const string & message) {
  class_instance<C$ComponentQuery> query;
  const PlatformCtx & ptx = *get_platform_context();
  uint64_t stream_d;
  OpenStreamResult res = ptx.open(name.size(), name.c_str(), &stream_d);
  if (res != OpenStreamOk) {
    php_warning("cannot open stream");
    co_return query;
  }
  int writed = co_await write_all_to_stream(stream_d, message.c_str(), message.size());
  ptx.shutdown_write(stream_d);
  php_debug("send %d bytes from %d to \"%s\" on stream %lu", writed, message.size(), name.c_str(), stream_d);
  query.alloc();
  query.get()->stream_d = stream_d;
  co_return query;
}

task_t<string> f$component_client_get_result(class_instance<C$ComponentQuery> query) {
  php_assert(!query.is_null());
  uint64_t stream_d = query.get()->stream_d;
  if (stream_d == 0) {
    php_warning("cannot get component client result");
    co_return string();
  }

  auto [buffer, size] = co_await read_all_from_stream(stream_d);
  string result;
  result.assign(buffer, size);
  free_descriptor(stream_d);
  query.get()->stream_d = 0;
  php_debug("read %d bytes from stream %lu", size, stream_d);
  co_return result;
}

task_t<void> f$component_server_send_result(const string &message) {
  ComponentState & ctx = *get_component_context();
  bool ok = co_await write_all_to_stream(ctx.standard_stream, message.c_str(), message.size());
  if (!ok) {
    php_warning("cannot send component result");
  } else {
    php_debug("send result \"%s\"", message.c_str());
  }
  free_descriptor(ctx.standard_stream);
  ctx.standard_stream = 0;
}

task_t<string> f$component_server_get_query() {
  ComponentState & ctx = *get_component_context();
  if (ctx.standard_stream == 0) {
    co_await parse_input_query(COMPONENT);
  }
  auto [buffer, size] = co_await read_all_from_stream(ctx.standard_stream);
  string query = string(buffer, size);
  get_platform_context()->allocator.free(buffer);
  co_return query;
}

task_t<class_instance<C$ComponentStream>> f$component_accept_stream() {
  ComponentState & ctx = *get_component_context();
  if (ctx.standard_stream != 0) {
    php_warning("previous stream does not closed");
    free_descriptor(ctx.standard_stream);
    ctx.standard_stream = 0;
  }
  co_await parse_input_query(COMPONENT);
  class_instance<C$ComponentStream> stream;
  stream.alloc();
  stream.get()->stream_d = ctx.standard_stream;
  co_return stream;
}

class_instance<C$ComponentStream> f$component_open_stream(const string &name) {
  class_instance<C$ComponentStream> query;
  const PlatformCtx & ptx = *get_platform_context();
  ComponentState & ctx = *get_component_context();
  uint64_t stream_d;
  OpenStreamResult res = ptx.open(name.size(), name.c_str(), &stream_d);
  if (res != OpenStreamOk) {
    php_warning("cannot open stream");
    return query;
  }
  ctx.opened_streams[stream_d] = NotBlocked;
  query.alloc();
  query.get()->stream_d = stream_d;
  php_debug("open stream %lu to %s", stream_d, name.c_str());
  return query;
}

int64_t f$component_stream_write_nonblock(const class_instance<C$ComponentStream> & stream, const string & message) {
  return write_nonblock_to_stream(stream.get()->stream_d, message.c_str(), message.size());
}

string f$component_stream_read_nonblock(const class_instance<C$ComponentStream> & stream) {
  auto [ptr, size] = read_nonblock_from_stream(stream.get()->stream_d);
  string result(ptr, size);
  get_platform_context()->allocator.free(ptr);
  return result;
}

task_t<int64_t> f$component_stream_write_exact(const class_instance<C$ComponentStream> & stream, const string & message) {
  int write = co_await write_exact_to_stream(stream->stream_d, message.c_str(), message.size());
  php_debug("write exact %d bytes to stream %lu", write, stream->stream_d);
  co_return write;
}

task_t<string> f$component_stream_read_exact(const class_instance<C$ComponentStream> & stream, int64_t len) {
  char * buffer = static_cast<char *>(dl::allocate(len));
  int read = co_await read_exact_from_stream(stream->stream_d, buffer, len);
  string result(buffer, read);
  dl::deallocate(buffer, len);
  php_debug("read exact %d bytes from stream %lu", read, stream->stream_d);
  co_return result;
}

void f$component_close_stream(const class_instance<C$ComponentStream> & stream) {
  free_descriptor(stream->stream_d);
}

void f$component_finish_stream_processing(const class_instance<C$ComponentStream> & stream) {
  ComponentState & ctx = *get_component_context();
  if (stream->stream_d != ctx.standard_stream) {
    php_warning("call server finish query on non server stream %lu", stream->stream_d);
    return;
  }
  free_descriptor(ctx.standard_stream);
  ctx.standard_stream = 0;
}
