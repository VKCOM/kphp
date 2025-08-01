// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/output/output-buffer.h"

#include <string_view>

#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/output/output-state.h"

void f$ob_start(const string& callback) noexcept {
  static constexpr std::string_view ob_gzhandler_name{"ob_gzhandler"};

  auto& output_instance_st{OutputInstanceState::get()};
  const auto current_buffering_level{output_instance_st.output_buffers.user_level()};

  if (!output_instance_st.output_buffers.next_user_buffer().has_value()) [[unlikely]] {
    kphp::log::warning("maximum level of output buffering reached, can't do ob_start({})", callback.c_str());
    return;
  }

  if (!callback.empty()) {
    if (current_buffering_level == 0 && std::string_view{callback.c_str(), callback.size()} == ob_gzhandler_name) {
      auto& http_server_instance_st{HttpServerInstanceState::get()};
      http_server_instance_st.encoding |= HttpServerInstanceState::ENCODING_GZIP;
    } else {
      kphp::log::error("unsupported callback {} at buffering level {}", callback.c_str(), output_instance_st.output_buffers.user_level());
    }
  }
}
