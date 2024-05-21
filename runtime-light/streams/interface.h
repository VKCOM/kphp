#pragma once

#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/component-stream.h"

constexpr int64_t v$COMPONENT_ERROR = -1;

task_t<void> f$component_get_http_query();

/**
 * component query client blocked interface
 * */
task_t<class_instance<C$ComponentQuery>> f$component_client_send_query(const string &name, const string & message);
task_t<string> f$component_client_get_result(class_instance<C$ComponentQuery> query);

/**
 * component query server blocked interface
 * */
task_t<string> f$component_server_get_query();
task_t<void> f$component_server_send_result(const string &message);

/**
 * component query non blocked interface
 * */
class_instance<C$ComponentStream> f$component_open_stream(const string &name);
task_t<class_instance<C$ComponentStream>> f$component_accept_stream();

int64_t f$component_stream_write_nonblock(const class_instance<C$ComponentStream> & stream, const string & message);
string f$component_stream_read_nonblock(const class_instance<C$ComponentStream> & stream);

task_t<int64_t> f$component_stream_write_exact(const class_instance<C$ComponentStream> & stream, const string & message);
task_t<string> f$component_stream_read_exact(const class_instance<C$ComponentStream> & stream, int64_t len);

void f$component_close_stream(const class_instance<C$ComponentStream> & stream);
void f$component_finish_stream_processing(const class_instance<C$ComponentStream> & stream);
