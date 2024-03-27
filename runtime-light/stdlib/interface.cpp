#include "runtime-light/stdlib/interface.h"

#include <cassert>
#include <cstdint>
#include <random>


#include "runtime-light/core/kphp_core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/awaitable.h"

int64_t f$rand() {
  std::random_device rd;
  int64_t dice_roll = rd();
  return dice_roll;
}

task_t<std::optional<char>> try_read(uint64_t stream_d) {
  const PlatformCtx &ptx = *get_platform_context();
  char letter;
  size_t processed = 0;
  StreamStatus status;
  while (processed != 1) {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      printf("res read not OK\n");
      co_return std::nullopt;
    }
    if (status.read_status == IOBlocked) {
      printf("blocked read\n");
      get_component_context()->awaited_stream = stream_d;
      co_await platform_switch_t{};
      continue;
    }
    if (status.read_status == IOClosed) {
      printf("read status closed\n");
      co_return std::nullopt;
    }
    processed = ptx.read(stream_d, 1, &letter);
  }
  co_return letter;
}

task_t<std::optional<char>> try_write(uint64_t stream_d, char letter) {
  const PlatformCtx &ptx = *get_platform_context();
  size_t processed = 0;
  StreamStatus status;
  while (processed != 1) {
    GetStatusResult res = ptx.get_stream_status(stream_d, &status);
    if (res != GetStatusOk) {
      printf("res write not OK\n");
      co_return std::nullopt;
    }
    if (status.write_status == IOBlocked) {
      printf("blocked write\n");
      get_component_context()->awaited_stream = stream_d;
      co_await platform_switch_t{};
      continue;
    }
    if (status.write_status == IOClosed) {
      printf("read write closed\n");
      co_return std::nullopt;
    }
    processed = ptx.write(stream_d, 1, &letter);
  }
  co_return letter;
}

task_t<void> f$dummy_echo() {
    ComponentState &rtx = *get_component_context();
    char letter;
    uint64_t stream_d = rtx.standard_stream;
    while (true) {
      auto read_res = co_await try_read(stream_d);
      if (!read_res.has_value()) {
        co_return;
      } else {
        letter = read_res.value();
      }
      auto write_res = co_await try_write(stream_d, letter);
      if (!write_res.has_value()) {
        co_return;
      }
    }
}


//task_t<void> stream_write(uint64_t sd, int len, const char * data) {
//  ComponentState &rtx = *get_component_context();
//  const PlatformCtx &ptx = *get_platform_context();
//  int left = len;
//  int processed = 0;
//  while (left != 0) {
//    auto res = ptx.write(sd, left, data + processed);
//    processed += res.processed_bytes;
//    left -= res.processed_bytes;
//    if (left == 0) {
//      break;
//    }
//
//    if (res.new_status == OperationStatus::Available) {
//      continue;
//    } else if (res.new_status == OperationStatus::Blocked) {
//      do {
//        rtx.awaited_stream = Stream{sd, OperationStatus::Blocked, OperationStatus::Blocked};
//        co_await platform_switch_t{};
//      } while (rtx.awaited_stream.write_status != OperationStatus::Available);
//    } else {
//      assert(false);
//    }
//  }
//}
//
//task_t<void> stream_read(uint64_t sd, int len, char * data) {
//  ComponentState &rtx = *get_runtime_context();
//  const PlatformCtx &ptx = *get_platform_context();
//  int left = len;
//  int processed = 0;
//  while (left != 0) {
//    auto res = ptx.read(sd, left, data + processed);
//    processed += res.processed_bytes;
//    left -= res.processed_bytes;
//    if (left == 0) {
//      break;
//    }
//
//    if (res.new_status == OperationStatus::Available) {
//      continue;
//    } else if (res.new_status == OperationStatus::Blocked) {
//      do {
//        rtx.awaited_stream = Stream{sd, OperationStatus::Blocked, OperationStatus::Blocked};
//        co_await platform_switch_t{};
//      } while (rtx.awaited_stream.read_status != OperationStatus::Available);
//    } else {
//      assert(false);
//    }
//  }
//}
//
//task_t<string> f$query_echo(const string &str) {
//  ComponentState &rtx = *get_runtime_context();
//  const PlatformCtx &ptx = *get_platform_context();
//
//  OpenResponse response = ptx.open("QueryEcho");
//  if (response.error != OpenError::NoError) {
//    assert(false);
//  }
//  if (response.stream.write_status != OperationStatus::Available) {
//    assert(false);
//  }
//
//  int64_t size = str.size();
//  co_await stream_write(response.stream.sd, sizeof(int64_t), reinterpret_cast<const char *>(&size));
//  co_await stream_write(response.stream.sd, str.size(), str.c_str());
//
//  do {
//    rtx.awaited_stream = Stream{response.stream.sd, OperationStatus::Blocked, OperationStatus::Blocked};
//    co_await platform_switch_t{};
//  } while (rtx.awaited_stream.read_status != OperationStatus::Available);
//
//  int64_t new_size = 0;
//  co_await stream_read(response.stream.sd, sizeof(int64_t), reinterpret_cast<char *>(&new_size));
//  char answer[1024]; // ATTENTION - This is not standard
//  co_await stream_read(response.stream.sd, new_size, answer);
//
//  ptx.free_stream(response.stream.sd);
//  co_return string(answer, new_size);
//}