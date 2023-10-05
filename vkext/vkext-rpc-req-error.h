// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once


#include <map>
#include <optional>
#include <string>

#include "vkext/vkext-tl-parse.h"

namespace vkext_rpc {
namespace tl {
struct NetPid {
  int ip{0};
  int port_pid{0};
  int utime{0};

  void tl_fetch();
};

struct RpcReqResultExtra {
  std::optional<long long> binlog_pos;
  std::optional<long long> binlog_time;
  std::optional<NetPid> engine_pid;
  std::optional<int> request_size;
  std::optional<int> response_size;
  std::optional<int> failed_subqueries;
  std::optional<int> compression_version;
  std::optional<std::map<std::string, std::string>> stats;
  std::optional<long long> epoch_number;
  std::optional<long long> view_number;

  void tl_fetch(int flags);
};

struct RpcReqError {
  long long query_id{0};
  int error_code{0};
  std::string error_msg;

  void tl_fetch();
};
} // namespace tl

struct RpcError {
  int flags{0};
  std::optional<tl::RpcReqResultExtra> header;
  std::optional<tl::RpcReqError> error;

  void try_fetch();
};
} // namespace vkext_rpc
