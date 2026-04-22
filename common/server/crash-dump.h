// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <cstddef>
#include <cstdint>

struct crash_dump_buffer_t {
  char scratchpad[1024]{};
  size_t position{0};

  void reset() {
    position = 0;
  }
};

void crash_dump_write_reg(const char* reg_name, size_t reg_name_size, uint64_t reg_value, crash_dump_buffer_t* buffer);
void crash_dump_prepare_registers(crash_dump_buffer_t* buffer, void* ucontext);
void crash_dump_write(void* ucontext);
