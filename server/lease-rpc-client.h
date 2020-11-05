// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <utility>
#include <vector>

struct LeaseRpcClient {
  std::string host;
  int port{-1};
  long long actor{-1};
  int target_id{-1};

  LeaseRpcClient() = default;
  const char *check() const;
  bool is_alive() const;
};

struct RpcClients {
  // rpc-proxy list that was specified via -w or -S YAML config
  std::vector<LeaseRpcClient> rpc_clients;

  static RpcClients &get() {
    static RpcClients ctx;
    return ctx;
  }

  const LeaseRpcClient &get_random_alive_client();
};
