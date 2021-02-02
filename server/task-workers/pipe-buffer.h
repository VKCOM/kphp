// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/param.h>

struct PipeBuffer {
  int read_buf[PIPE_BUF / sizeof(int)]{};
  int write_buf[PIPE_BUF / sizeof(int)]{};
};
