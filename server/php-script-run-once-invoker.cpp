// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-script-run-once-invoker.h"
#include "server/php-engine.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <unistd.h>

#include "common/crc32c.h"
#include "common/kprintf.h"
#include "common/pipe-utils.h"
#include "common/tl/constants/common.h"
#include "net/net-connections.h"
#include "net/net-socket.h"
#include "net/net-tcp-rpc-client.h"

// These are defined in php-engine.cpp
extern conn_type_t ct_php_rpc_client;
extern tcp_rpc_client_functions rpc_client_methods;

void PhpScriptRunOnceInvoker::init(int total_run_once_count) {
  remaining_count_ = total_run_once_count;

  int pipe_fd[2];
  if (pipe(pipe_fd) != 0) {
    kprintf("Failed to create pipe for run_once mode: %s\n", strerror(errno));
    exit(1);
  }

  int read_fd = pipe_fd[0];
  write_fd_ = pipe_fd[1];

  bool ok = set_fd_nonblocking(write_fd_);
  if (!ok) {
    kprintf("Failed to set pipe non-blocking: %s\n", strerror(errno));
    exit(1);
  }

  rpc_client_methods.rpc_ready = nullptr;
  epoll_insert_pipe(pipe_for_read, read_fd, &ct_php_rpc_client, &rpc_client_methods);
}

bool PhpScriptRunOnceInvoker::invoke_run_once(int runs_count) {
  if (remaining_count_ <= 0 || write_fd_ == -1) {
    return false;
  }

  int q[6];
  int qsize = 6 * sizeof(int);
  q[2] = TL_RPC_INVOKE_REQ;

  int batch_size = std::min(remaining_count_, runs_count);
  for (int i = 0; i < batch_size; i++) {
    prepare_rpc_query_raw(next_packet_id_, q, qsize, crc32c_partial);
    ssize_t written = write(write_fd_, q, static_cast<size_t>(qsize));
    if (written != qsize) {
      if (written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        break;
      }
      // Other error or partial write - this shouldn't happen
      kprintf("Failed to write to run_once pipe: %s\n", strerror(errno));
      exit(1);
    }
    --remaining_count_;
    ++next_packet_id_;
  }

  return true;
}

bool PhpScriptRunOnceInvoker::has_pending() const {
  return remaining_count_ > 0;
}

bool PhpScriptRunOnceInvoker::enabled() const {
  return remaining_count_ != -1;
}
