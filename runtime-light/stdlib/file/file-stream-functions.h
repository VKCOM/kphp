// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/stdlib/file/file-stream-context.h"

inline mixed f$stream_socket_client(const string &, mixed & = FileStreamComponentContext::get().error_number_dummy,
                                    mixed & = FileStreamComponentContext::get().error_description_dummy, double = -1, int64_t = 1,
                                    const mixed & = mixed()) {
  php_critical_error("call to unsupported function");
}
