// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/lease-rpc-client.h"

#include <algorithm>
#include <random>

#include "net/net-connections.h"

#include "server/php-engine.h"

const char *LeaseRpcClient::check() const {
  if (host.empty()) {
    return "warning: host is not defined";
  }
  if (port == -1) {
    return "warning: port is not defined";
  }
  return nullptr;
}

bool LeaseRpcClient::is_alive() const {
  return get_target_connection(&Targets[target_id], 0) != nullptr;
}

const LeaseRpcClient &RpcClients::get_random_alive_client() {
  auto alive_end = std::partition(rpc_clients.begin(), rpc_clients.end(), [](const LeaseRpcClient &rpc_client) { return rpc_client.is_alive(); });
  size_t alive_cnt = alive_end - rpc_clients.begin();
  if (alive_cnt == 0) {
    static LeaseRpcClient default_client;
    return default_client;
  }
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, static_cast<int>(alive_cnt - 1));
  return rpc_clients[dis(gen)];
}
