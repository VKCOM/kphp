// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/server/crash-dump.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/ucontext/ucontext-portable.h"

struct crash_dump_buffer {
  char scratchpad[1024];
  size_t position;
};
using crash_dump_buffer_t = struct crash_dump_buffer;

static inline char crash_dump_half_byte_char(uint8_t hb) {
  if (hb <= 9) {
    return '0' + hb;
  }

  if (10 <= hb && hb <= 15) {
    return 'A' + hb - 10;
  }

  return '*';
}

static inline void crash_dump_write_uint8(uint8_t value, crash_dump_buffer_t *buffer) {
  assert(sizeof(buffer->scratchpad) - buffer->position >= 2);
  buffer->scratchpad[buffer->position++] = crash_dump_half_byte_char(value >> 4);
  buffer->scratchpad[buffer->position++] = crash_dump_half_byte_char(value & 0x0F);
}

static inline void crash_dump_write_uint32(uint32_t value, crash_dump_buffer_t *buffer) {
  for (int i = 0; i < sizeof(value); ++i) {
    const uint8_t byte = (value >> (8 * (3 - i))) & 0xFF;
    crash_dump_write_uint8(byte, buffer);
  }
}

static inline void crash_dump_write_uint64(uint64_t value, crash_dump_buffer_t *buffer) {
  crash_dump_write_uint32(static_cast<uint32_t>(value >> 32), buffer);
  crash_dump_write_uint32(static_cast<uint32_t>(value & 0xFFFFFFFF), buffer);
}

static inline void crash_dump_write_reg(const char* reg_name, size_t reg_name_size, uint64_t reg_value, crash_dump_buffer_t *buffer) {
  assert(reg_name_size +buffer->position <= sizeof(buffer->scratchpad));
  memcpy(&buffer->scratchpad[buffer->position], reg_name, reg_name_size);
  buffer->position += reg_name_size;
  crash_dump_write_uint64(reg_value, buffer);
  assert(buffer->position < sizeof(buffer->scratchpad));
  buffer->scratchpad[buffer->position++] = ' ';
}

#define LITERAL_WITH_LENGTH(literal) literal, sizeof(literal) - 1

static inline void crash_dump_prepare_registers(crash_dump_buffer_t *buffer, void *ucontext) {
#ifdef __x86_64__
#ifdef __APPLE__
  const auto *uc = static_cast<ucontext_t *>(ucontext);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("RIP=0x"), uc->uc_mcontext->__ss.__rip, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RSP=0x"), uc->uc_mcontext->__ss.__rsp, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RBP=0x"), uc->uc_mcontext->__ss.__rbp, buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("RDI=0x"), uc->uc_mcontext->__ss.__rdi, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RSI=0x"), uc->uc_mcontext->__ss.__rsi, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RDX=0x"), uc->uc_mcontext->__ss.__rdx, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RCX=0x"), uc->uc_mcontext->__ss.__rcx, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R8=0x"), uc->uc_mcontext->__ss.__r8, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R9=0x"), uc->uc_mcontext->__ss.__r9, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R10=0x"), uc->uc_mcontext->__ss.__r10, buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("RBX=0x"), uc->uc_mcontext->__ss.__rbx, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RAX=0x"), uc->uc_mcontext->__ss.__rax, buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("R11=0x"), uc->uc_mcontext->__ss.__r11, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R12=0x"), uc->uc_mcontext->__ss.__r12, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R13=0x"), uc->uc_mcontext->__ss.__r13, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R14=0x"), uc->uc_mcontext->__ss.__r14, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R15=0x"), uc->uc_mcontext->__ss.__r15, buffer);
#else
  const auto *uc = static_cast<ucontext_t *>(ucontext);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("RIP=0x"), uc->uc_mcontext.gregs[REG_RIP], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RSP=0x"), uc->uc_mcontext.gregs[REG_RSP], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RBP=0x"), uc->uc_mcontext.gregs[REG_RBP], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("RDI=0x"), uc->uc_mcontext.gregs[REG_RDI], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RSI=0x"), uc->uc_mcontext.gregs[REG_RSI], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RDX=0x"), uc->uc_mcontext.gregs[REG_RDX], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RCX=0x"), uc->uc_mcontext.gregs[REG_RCX], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R8=0x"), uc->uc_mcontext.gregs[REG_R8], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R9=0x"), uc->uc_mcontext.gregs[REG_R9], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R10=0x"), uc->uc_mcontext.gregs[REG_R10], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("RBX=0x"), uc->uc_mcontext.gregs[REG_RBX], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("RAX=0x"), uc->uc_mcontext.gregs[REG_RAX], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("CR2=0x"), uc->uc_mcontext.gregs[REG_CR2], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("R11=0x"), uc->uc_mcontext.gregs[REG_R11], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R12=0x"), uc->uc_mcontext.gregs[REG_R12], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R13=0x"), uc->uc_mcontext.gregs[REG_R13], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R14=0x"), uc->uc_mcontext.gregs[REG_R14], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("R15=0x"), uc->uc_mcontext.gregs[REG_R15], buffer);
#endif
#elif defined(__arm64__) || defined (__aarch64__)
  const auto *uc = static_cast<ucontext_t_portable *>(ucontext);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("SP=0x"), uc->uc_mcontext.sp, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("PC=0x"), uc->uc_mcontext.pc, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("FP=0x"), uc->uc_mcontext.fp, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("LR=0x"), uc->uc_mcontext.lr, buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("PSTATE=0x"), uc->uc_mcontext.pstate, buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("X0=0x"), uc->uc_mcontext.regs[0], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X1=0x"), uc->uc_mcontext.regs[1], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X2=0x"), uc->uc_mcontext.regs[2], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X3=0x"), uc->uc_mcontext.regs[3], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X4=0x"), uc->uc_mcontext.regs[4], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X5=0x"), uc->uc_mcontext.regs[5], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X6=0x"), uc->uc_mcontext.regs[6], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X7=0x"), uc->uc_mcontext.regs[7], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("X8=0x"), uc->uc_mcontext.regs[8], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X9=0x"), uc->uc_mcontext.regs[9], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X10=0x"), uc->uc_mcontext.regs[10], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X11=0x"), uc->uc_mcontext.regs[11], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X12=0x"), uc->uc_mcontext.regs[12], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X13=0x"), uc->uc_mcontext.regs[13], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X14=0x"), uc->uc_mcontext.regs[14], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X15=0x"), uc->uc_mcontext.regs[15], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("X16=0x"), uc->uc_mcontext.regs[16], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X17=0x"), uc->uc_mcontext.regs[17], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X18=0x"), uc->uc_mcontext.regs[18], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X19=0x"), uc->uc_mcontext.regs[19], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X20=0x"), uc->uc_mcontext.regs[20], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X21=0x"), uc->uc_mcontext.regs[21], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X22=0x"), uc->uc_mcontext.regs[22], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X23=0x"), uc->uc_mcontext.regs[23], buffer);

  crash_dump_write_reg(LITERAL_WITH_LENGTH("X24=0x"), uc->uc_mcontext.regs[24], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X25=0x"), uc->uc_mcontext.regs[25], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X26=0x"), uc->uc_mcontext.regs[26], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X27=0x"), uc->uc_mcontext.regs[27], buffer);
  crash_dump_write_reg(LITERAL_WITH_LENGTH("X28=0x"), uc->uc_mcontext.regs[28], buffer);
#else
#error "Unsupported arch"
#endif
}

void crash_dump_write(void *ucontext) {
  static const char header[] = "\n------- Register Values -------\n";
  kwrite(STDERR_FILENO, header, sizeof(header) - 1);

  static crash_dump_buffer_t buffer;
  buffer.position = 0;
  crash_dump_prepare_registers(&buffer, ucontext);

  assert(buffer.position < sizeof(buffer.scratchpad));
  buffer.scratchpad[buffer.position++] = '\n';

  kwrite(STDERR_FILENO, buffer.scratchpad, buffer.position);
}
