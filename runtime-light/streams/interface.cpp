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
  bool ok = co_await write_all_to_stream(stream_d, message.c_str(), message.size());
  ptx.shutdown_write(stream_d);
  if (!ok) {
    php_warning("cannot send component client query");
    co_return query;
  }
  php_debug("send \"%s\" to \"%s\" on stream %lu", message.c_str(), name.c_str(), stream_d);
  query.alloc();
  query.get()->stream_d = stream_d;
  co_return query;
}

task_t<string> f$component_client_get_result(class_instance<C$ComponentQuery> query) {
  php_assert(!query.is_null());
  php_debug("f$component_client_get_result");
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
