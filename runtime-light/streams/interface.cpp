#include "runtime-light/streams/interface.h"


#include "runtime-light/component/component.h"
#include "runtime-light/streams/streams.h"

// in KPHP runtime we have only int64_t. What if value uint64_t > int64_t?
task_t<int64_t> f$component_client_send_query(const string &name, const string & message) {
  uint64_t stream_d;
  OpenStreamResult res = get_platform_context()->open(name.size(), name.c_str(), &stream_d);
  if (res != OpenStreamOk) {
    php_warning("cannot open stream");
    co_return v$COMPONENT_ERROR;
  }
  bool ok = co_await write_all_to_stream(stream_d, message.c_str(), message.size());
  get_platform_context()->shutdown_write(stream_d);
  if (!ok) {
    php_warning("cannot send component client query");
    co_return v$COMPONENT_ERROR;
  }
  php_debug("send \"%s\" to \"%s\"", message.c_str(), name.c_str());
  co_return stream_d;
}

task_t<string> f$component_client_get_result(int64_t qid) {
  if (qid < 0) {
    php_warning("cannot get component client result");
    co_return v$COMPONENT_ERROR;
  }
  auto [buffer, size] = co_await read_all_from_stream(qid);
  string result;
  result.assign(buffer, size);
  co_return result;
}

task_t<void> f$component_server_send_result(const string &message) {
  bool ok = co_await write_all_to_stream(get_component_context()->standard_stream, message.c_str(), message.size());
  if (!ok) {
    php_warning("cannot send component result");
    co_return;
  }
  php_debug("send result \"%s\"", message.c_str());
  get_platform_context()->shutdown_write(get_component_context()->standard_stream);
}

// move read query here. What about init and reading there
string f$component_server_get_query() {
  string query = get_component_context()->superglobals.v$_RAW_QUERY;
  get_component_context()->superglobals.v$_RAW_QUERY = string();
  return query;

}
