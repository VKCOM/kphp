// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

using on_oom_callback_t = std::function<void()>;

class OomHandler : vk::not_copyable {
public:
  void reset() noexcept;

  void set_callback(const on_oom_callback_t &callback) noexcept;
  void invoke() noexcept;
private:
  on_oom_callback_t callback_;
  bool callback_running_ = false;

  OomHandler() = default;

  friend class vk::singleton<OomHandler>;
};

bool f$register_kphp_on_oom_callback(const on_oom_callback_t &callback);
