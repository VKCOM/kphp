// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// The exit codes were selected according to https://tldp.org/LDP/abs/html/exitcodes.html
// 100, 101, and 102 are free for statuses defined by the programmer and they look pretty
// If the app is going to exit because of a signal, then the return code is 128 + %signal_code%,
// which is also a common practice
enum class ExitCode {
  KPHP_TO_CPP_STAGE = 100,
  CPP_TO_OBJS_STAGE = 101,
  OBJS_TO_BINARY_STAGE = 102,
  SIGNAL_OFFSET = 128
};
