// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <functional>
#include <ranges>
#include <span>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/core/globals/php-script-globals.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/cli/cli-instance-state.h"
#include "runtime-light/stdlib/output/output-state.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/utils/logs.h"

namespace kphp::cli {

inline void init_cli_server(kphp::component::stream output_stream) noexcept {
  auto& superglobals{PhpScriptMutableGlobals::current().get_superglobals()};
  using namespace PhpServerSuperGlobalIndices;
  superglobals.v$argc = static_cast<int64_t>(0);
  superglobals.v$argv = array<mixed>{};
  superglobals.v$_SERVER.set_value(string{ARGC.data(), ARGC.size()}, superglobals.v$argc);
  superglobals.v$_SERVER.set_value(string{ARGV.data(), ARGV.size()}, superglobals.v$argv);
  superglobals.v$_SERVER.set_value(string{PHP_SELF.data(), PHP_SELF.size()}, string{});
  superglobals.v$_SERVER.set_value(string{SCRIPT_NAME.data(), SCRIPT_NAME.size()}, string{});
  CLIInstanceInstance::get().output_stream = std::move(output_stream);
}

inline kphp::coro::task<> finalize_cli_server() noexcept {
  const auto& cli_instance_state{CLIInstanceInstance::get()};
  if (!cli_instance_state.output_stream) [[unlikely]] {
    kphp::log::error("tried to write output to unset CLI output stream");
  }

  static constexpr auto transformer{
      [](const string_buffer& buffer) noexcept -> std::span<const std::byte> { return {reinterpret_cast<const std::byte*>(buffer.buffer()), buffer.size()}; }};

  auto& output_instance_state{OutputInstanceState::get()};
  const auto system_buffer{std::invoke(transformer, output_instance_state.output_buffers.system_buffer().get())};

  const auto& output_stream{*cli_instance_state.output_stream};
  if (auto expected{co_await output_stream.write(system_buffer)}; !expected) [[unlikely]] {
    kphp::log::error("can't write system buffer to output: stream -> {}, error code -> {}", output_stream.descriptor(), expected.error());
  }

  auto user_buffers{output_instance_state.output_buffers.user_buffers() |
                    std::views::filter([](const string_buffer& buffer) noexcept { return buffer.size() > 0; }) | std::views::transform(transformer)};
  for (const auto& buffer : user_buffers) {
    if (auto expected{co_await output_stream.write(buffer)}; !expected) [[unlikely]] {
      kphp::log::error("can't write user buffer to output: stream -> {}, error code -> {}", output_stream.descriptor(), expected.error());
    }
  }
}

} // namespace kphp::cli
