#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

constexpr int64_t v$COMPONENT_ERROR = -1;

task_t<int64_t> f$component_client_send_query(const string &name, const string & message);
task_t<string> f$component_client_get_result(int64_t qid);

string f$component_server_get_query();
task_t<void> f$component_server_send_result(const string &message);
