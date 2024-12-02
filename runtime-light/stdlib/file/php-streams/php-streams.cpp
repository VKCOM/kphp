// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/php-streams/php-streams.h"

#include <cstring>

#include "runtime-light/state/instance-state.h"

namespace {

constexpr std::string_view STDERR_NAME{"stderr"};
constexpr std::string_view STDOUT_NAME{"stdout"};
constexpr std::string_view STDIN_NAME{"stdin"};

} // namespace

uint64_t open_php_stream(const string &stream_name) noexcept {
  if (strcmp(stream_name.c_str(), STDERR_NAME.data()) == 0) {
    return INVALID_PLATFORM_DESCRIPTOR;
  } else if (strcmp(stream_name.c_str(), STDOUT_NAME.data()) == 0) {
    return InstanceState::get().standard_stream();
  } else if (strcmp(stream_name.c_str(), STDIN_NAME.data()) == 0) {
    return InstanceState::get().standard_stream();
  } else {
    php_warning("Unknown name %s for php stream", stream_name.c_str());
    return INVALID_PLATFORM_DESCRIPTOR;
  }
}
