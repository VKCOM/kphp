// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
enum class ExitCode {
  KPHP_TO_CPP_STAGE = 100,
  CPP_TO_OBJS_STAGE = 101,
  OBJS_TO_BINARY_STAGE = 102,
  SIGNAL_OFFSET = 128
};
