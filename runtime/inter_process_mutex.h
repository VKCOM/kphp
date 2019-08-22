#pragma once
#include <sys/types.h>

#include "common/mixin/not_copyable.h"

class inter_process_mutex : vk::not_copyable {
public:
  void lock() noexcept;
  void unlock() noexcept;

private:
  pid_t lock_{0};
  char cache_line_padding_[128 - sizeof(pid_t)];
};
