// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

// Helper class to run PHP scripts N times via self-RPC requests through a pipe
// Avoids blocking on pipe write by writing messages in batches from within the event loop
class PhpScriptRunOnceInvoker : vk::not_copyable {
public:
  static constexpr int DEFAULT_RUNS_BATCH_SIZE = 128;

  // Initialize the pipe and register read end with epoll
  void init(int total_run_once_count);

  // Try to send a batch of run-once trigger messages to the pipe
  // Returns true if there are more messages to send, false when done
  bool invoke_run_once(int runs_count = DEFAULT_RUNS_BATCH_SIZE);

  // Check if we still have messages pending
  bool has_pending() const;

  bool enabled() const;

private:
  int remaining_count_{-1};
  int write_fd_{-1};
  int next_packet_id_{0};

  PhpScriptRunOnceInvoker() = default;

  friend class vk::singleton<PhpScriptRunOnceInvoker>;
};
