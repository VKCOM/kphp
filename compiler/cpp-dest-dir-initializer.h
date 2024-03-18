// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <thread>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

class CppDestDirInitializer : vk::not_copyable {
public:
  friend class vk::singleton<CppDestDirInitializer>;

  static void initialize_sync() noexcept;

  void initialize_async(int32_t thread_id) noexcept;
  void wait() noexcept;

  ~CppDestDirInitializer() noexcept;

private:
  CppDestDirInitializer() = default;

  std::unique_ptr<std::thread> initialization_thread_;
};
