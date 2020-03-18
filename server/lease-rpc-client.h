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
  // Список rpc-proxy, переданных через -w или через -S YAML конфиг
  std::vector<LeaseRpcClient> rpc_clients;

  static RpcClients &get() {
    static RpcClients ctx;
    return ctx;
  }

  const LeaseRpcClient &get_random_alive_client();
};
