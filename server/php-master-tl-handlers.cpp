// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <csignal>
#include <unistd.h>

#include "common/server/stats.h"
#include "common/tl/constants/common.h"
#include "common/tl/constants/engine.h"
#include "common/tl/constants/net.h"
#include "common/tl/fetch.h"
#include "common/tl/methods/network.h"
#include "common/tl/methods/rwm.h"
#include "common/tl/methods/tcp-rwm.h"
#include "common/tl/query-header.h"
#include "common/version-string.h"

#include "net/net-msg.h"
#include "server/php-master-tl-handlers.h"

namespace {
bool fetch_function() {
  tl_fetch_end();
  return !tl_fetch_error();
}

template<typename Fetcher>
bool fetch_function(Fetcher &&fetcher) {
  fetcher();
  tl_fetch_end();
  return !tl_fetch_error();
}

// @any engine.nop  = True;
// @read engine.readNop = True; // same as nop, but assumed as read in proxy
// @write engine.writeNop = True; // same as nop, but assumed as write in proxy
void tl_engine_nop_handler() {
  if (!fetch_function()) {
    return;
  }

  tl_store_int(TL_TRUE);
}

// @any @internal engine.sleep time_ms:int = Bool;
void tl_engine_sleep_handler() {
  int time_ms = 0;
  bool ok = fetch_function([&]() { time_ms = tl_fetch_int(); });
  if (!ok) {
    return;
  }

  kprintf("sleep for %d ms\n", time_ms);
  usleep(time_ms * 1000);

  tl_store_int(TL_BOOL_TRUE);
}

// @any engine.pid = net.Pid;
void tl_engine_pid_handler() {
  if (!fetch_function()) {
    return;
  }

  tl_store_int(TL_NET_PID);
  tl_store_raw_data(&PID, sizeof(PID));
}

// @any engine.version = String;
void tl_engine_version_handler() {
  if (!fetch_function()) {
    return;
  }

  tl_store_int(TL_STRING);
  tl_store_string0(get_version_string());
}

// @any engine.count = BoolStat;
void tl_engine_count_handler() {
  if (!fetch_function()) {
    return;
  }

  tl_store_bool_stat(1);
}

// @any @internal engine.sendSignal signal:int = True;
void tl_engine_send_signal_handler() {
  int signal = 0;
  bool ok = fetch_function([&]() { signal = tl_fetch_int_range(1, 64); });
  if (!ok) {
    return;
  }

  static double sigkill_forced = 0;
  struct sigaction act {};
  kprintf("Got signal '%s' through rpc\n", strsignal(signal));
  sigaction(signal, nullptr, &act);
  if (signal == SIGKILL) {
    if (precise_now - sigkill_forced > 300.0) {
      tl_fetch_set_error_format(TL_ERROR_FEATURE_DISABLED, "Sending SIGKILL can lead to repeatedly killing of engine. Repeate query if you are sure.");
      sigkill_forced = precise_now;
      return;
    }
  } else if (signal != SIGTERM && act.sa_handler == SIG_DFL) {
    tl_fetch_set_error_format(TL_ERROR_FEATURE_DISABLED, "Handler for signal '%s' is not set", strsignal(signal));
    return;
  }
  if (signal == SIGSEGV || signal == SIGABRT || signal == SIGFPE || signal == SIGBUS) {
    tl_fetch_set_error_format(TL_ERROR_FEATURE_DISABLED, "You shouldn't send signal '%s' to engine", strsignal(signal));
    return;
  }
  raise(signal);
  tl_store_int(TL_TRUE);
}

// @any @internal engine.setVerbosity verbosity:int = True;
void tl_engine_set_verbosity_handler() {
  int requested_verbosity = 0;
  bool ok = fetch_function([&]() { requested_verbosity = tl_fetch_int(); });
  if (!ok) {
    return;
  }

  verbosity = requested_verbosity;
  vkprintf(0, "TL_ENGINE_SET_VERBOSITY to %d\n", requested_verbosity);
  tl_store_int(TL_TRUE);
}

// @any @internal engine.setVerbosityType type:string verbosity:int = True;
void tl_engine_set_verbosity_type_handler() {
  static constexpr int MAX_LEN = 256;
  char type[MAX_LEN + 1] = {0};
  int requested_verbosity = 0;
  bool ok = fetch_function([&]() {
    tl_fetch_string0(type, MAX_LEN);
    requested_verbosity = tl_fetch_int();
  });
  if (!ok) {
    return;
  }

  if (set_verbosity_by_type(type, requested_verbosity)) {
    vkprintf(0, "TL_ENGINE_SET_VERBOSITY_TYPE (%s) to %d\n", type, requested_verbosity);
    tl_store_int(TL_TRUE);
  } else {
    tl_fetch_set_error_format(TL_ERROR_BAD_VALUE, "No type '%s' found", type);
  }
}

// @any engine.stat = Stat;
void tl_engine_stat_handler() {
  if (!fetch_function()) {
    return;
  }

  if (tl_stat_function) {
    tl_stat_function({});
  } else {
    engine_default_tl_stat_function({});
  }
}

// @any engine.filteredStat stat_names:%(Vector string) = Stat;
void tl_engine_filtered_stat_handler() {
  std::vector<std::string> sorted_filter_keys;
  bool ok = fetch_function([&]() {
    vk::tl::fetch_vector(sorted_filter_keys, 1 << 20);
    std::sort(sorted_filter_keys.begin(), sorted_filter_keys.end());
  });
  if (!ok) {
    return;
  }

  if (tl_stat_function) {
    tl_stat_function(sorted_filter_keys);
  } else {
    engine_default_tl_stat_function(sorted_filter_keys);
  }
}

// @any rpcInvokeReq#2374df3d {X:Type} query_id:long query:!X = RpcReqResult X;
// entrypoint for any TL query
void tl_rpc_invoke_req_handler(connection *connection) {
  tl_query_header_t header{};

  tl_store_init(std::make_unique<tl_out_methods_tcp_raw_msg>(connection), NETWORK_MAX_STORED_SIZE);

  tl_fetch_query_header(&header);
  tl_set_current_query_id(header.qid);

  unsigned int op = tl_fetch_int();

  if (!tl_fetch_error()) {
    tl_setup_result_header(&header);

    if (header.actor_id != 0) {
      tl_fetch_set_error(TL_ERROR_WRONG_ACTOR_ID, "actor_id != 0 is not supported by KPHP");
    }

    if (header.flags & (vk::tl::common::rpc_invoke_req_extra_flags::random_delay | vk::tl::common::rpc_invoke_req_extra_flags::wait_binlog_pos)) {
      tl_fetch_set_error(TL_ERROR_QUERY_INCORRECT, "random_delay or wait_binlog_pos is not supported by KPHP");
    }
  }

  if (!tl_fetch_error()) {
    switch (op) {
      case TL_ENGINE_NOP:
      case TL_ENGINE_READ_NOP:
      case TL_ENGINE_WRITE_NOP:
        tl_engine_nop_handler();
        break;
      case TL_ENGINE_SLEEP:
        tl_engine_sleep_handler();
        break;
      case TL_ENGINE_PID:
        tl_engine_pid_handler();
        break;
      case TL_ENGINE_VERSION:
        tl_engine_version_handler();
        break;
      case TL_ENGINE_COUNT:
        tl_engine_count_handler();
        break;
      case TL_ENGINE_SEND_SIGNAL:
        tl_engine_send_signal_handler();
        break;
      case TL_ENGINE_SET_VERBOSITY:
        tl_engine_set_verbosity_handler();
        break;
      case TL_ENGINE_SET_VERBOSITY_TYPE:
        tl_engine_set_verbosity_type_handler();
        break;
      case TL_ENGINE_STAT:
        tl_engine_stat_handler();
        break;
      case TL_ENGINE_FILTERED_STAT:
        tl_engine_filtered_stat_handler();
        break;
      default:
        tl_fetch_set_error_format(TL_ERROR_UNKNOWN_FUNCTION_ID, "Unexpected op 0x%08x", op);
    }
  }
  tl_store_end();
}
} // namespace

int master_rpc_tl_execute(connection *c, int op, raw_message_t *raw) {
  raw_message_t r;
  rwm_clone(&r, raw);
  tl_fetch_init_raw_message(&r);
  rwm_free(&r);

  if (tl_fetch_error()) {
    return -1;
  }
  if (op == TL_RPC_INVOKE_REQ) {
    tl_rpc_invoke_req_handler(c);
  } else {
    return -1;
  }
  return 0;
}
