// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

namespace vk {

// Helper class to run PHP scripts N times via self-RPC requests through a pipe
// Avoids blocking on pipe write by writing messages in batches from within the event loop
class PhpScriptRunOnceInvoker {
public:
  PhpScriptRunOnceInvoker(int total_count);

  // Initialize the pipe and register read end with epoll
  void init();

  // Try to send a batch of messages to the pipe
  // Returns true if there are more messages to send, false when done
  bool try_send_batch();

  // Check if we still have messages pending
  bool has_pending() const;

private:
  int write_fd_;
  int remaining_count_;
  int next_packet_id_;

  static constexpr int MAX_MESSAGES_PER_BATCH = 64;  // ~1.5KB, well below typical PIPE_BUF (~4KB-64KB)
};

} // namespace vk
