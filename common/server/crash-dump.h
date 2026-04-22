// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <ucontext.h>

struct crash_dump_buffer_t { // NOLINT
  char scratchpad[1024];
  size_t position{0};

  void reset() {
    position = 0;
  }

  std::string_view get_content() const noexcept {
    return std::string_view{scratchpad, position};
  }
};

void crash_dump_write_reg(const char* reg_name, size_t reg_name_size, uint64_t reg_value, crash_dump_buffer_t* buffer);
void crash_dump_prepare_registers(crash_dump_buffer_t* buffer, const ucontext_t* uc);
void crash_dump_write(void* ucontext);
