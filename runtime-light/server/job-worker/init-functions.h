// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/component/component.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/server/job-worker/job-worker-server-context.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/utils/context.h"

inline void init_job_server(tl::K2InvokeJobWorker &&invoke_jw) noexcept {
  auto &jw_server_ctx{JobWorkerServerInstanceState::get()};
  jw_server_ctx.kind = invoke_jw.ignore_answer ? JobWorkerServerInstanceState::Kind::NoReply : JobWorkerServerInstanceState::Kind::Regular;
  jw_server_ctx.state = JobWorkerServerInstanceState::State::Working;
  jw_server_ctx.job_id = invoke_jw.job_id;
  jw_server_ctx.body = std::move(invoke_jw.body);

  {
    using namespace PhpServerSuperGlobalIndices;
    auto &server{get_component_context()->php_script_mutable_globals_singleton.get_superglobals().v$_SERVER};
    server.set_value(string{JOB_ID, std::char_traits<char>::length(JOB_ID)}, invoke_jw.job_id);
  }
}
