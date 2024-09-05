// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"

struct FileStreamComponentState {
  mixed error_number_dummy;
  mixed error_description_dummy;

  static FileStreamComponentState &get() noexcept;
};

inline mixed f$stream_socket_client(const string &url, mixed &error_number = FileStreamComponentState::get().error_number_dummy,
                                    mixed &error_description = FileStreamComponentState::get().error_description_dummy, double timeout = -1, int64_t flags = 1,
                                    const mixed &context = mixed()) {
  php_critical_error("call to unsupported function");
}
