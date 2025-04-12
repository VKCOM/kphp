// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/cdefs.h>

#include "net/net-tcp-rpc-common.h"

#include "common/tl/methods/network-rwm.h"
#include "common/tl/parse.h"

struct tl_out_methods_tcp_raw_msg final : tl_out_methods_network_rwm_base<tl_out_methods_tcp_raw_msg> {
  connection_t* conn{};
  raw_message_t rwm{};

  explicit tl_out_methods_tcp_raw_msg(connection_t* conn)
      : conn(conn) {
    rwm_init(&rwm, 0);
  }

  void store_flush(const char* prefix, int prefix_len) noexcept override {
    assert(rwm.magic == RM_INIT_MAGIC);
    assert(conn);
    rwm_push_data_front(&rwm, prefix, prefix_len);
    tcp_rpc_conn_send(conn, &rwm, 0);
    rwm_clean(&rwm);
    flush_later(conn);
  }

  const process_id_t* get_pid() noexcept override {
    return &TCP_RPC_DATA(conn)->remote_pid;
  }
  ~tl_out_methods_tcp_raw_msg() {
    if (rwm.magic == RM_INIT_MAGIC) {
      rwm_free(&rwm);
    }
  }
  void* get_connection() noexcept override {
    return conn;
  }
};
