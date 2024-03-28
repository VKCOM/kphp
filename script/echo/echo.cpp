#include "runtime-headers.h"

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

task_t<void> f$src() noexcept  {
  // init_superglobals();
  // auto res = co_await read_all_from_stream(get_component_context()->standard_stream);
  // f$echo(res);
  co_await f$dummy_echo();
  co_await finish(0);
  co_return ;
}

task_t<void>(*k_main)(void) = f$src;
