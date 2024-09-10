// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"

// Polymorphic wrapper for lambda expressions and functional objects which are used in php script
// The main goal -- use script's memory manager instead of default language-provided memory manager
template<typename R, typename... ARGS>
class CallbackWrapper {
public:
  CallbackWrapper() = default;

  // No copy semantics
  CallbackWrapper(CallbackWrapper &other) = delete;
  CallbackWrapper &operator=(CallbackWrapper &other) = delete;

  CallbackWrapper(CallbackWrapper &&other) noexcept
    : func_obj(std::exchange(other.func_obj, nullptr))
    , invoker(std::exchange(other.invoker, nullptr))
    , deleter(std::exchange(other.deleter, nullptr)){};

  CallbackWrapper &operator=(CallbackWrapper &&other) noexcept {
    if (this == &other) {
      return *this;
    }
    func_obj = other.func_obj;
    invoker = other.invoker;
    deleter = other.deleter;
    other.func_obj = nullptr;
    other.invoker = nullptr;
    other.deleter = nullptr;
    return *this;
  };

  // Invoker and deleter accumulates typed functional objects as pointers
  // It allows to have no depends on functional or lambda objects layout
  template<typename F>
  explicit CallbackWrapper(F &&f)
    : invoker(typed_invoker<F>)
    , deleter(typed_deleter<F>) {
    auto *raw_mem = RuntimeAllocator::current().alloc_script_memory(sizeof(F));
    new (raw_mem) F{std::forward<F>(f)};
    func_obj = raw_mem;
  };

  explicit operator bool() const {
    return func_obj != nullptr && invoker != nullptr && deleter != nullptr;
  }

  R operator()(ARGS... args) {
    if constexpr (std::is_void_v<R>) {
      invoker(func_obj, args...);
    } else {
      return invoker(func_obj, args...);
    }
  }

  ~CallbackWrapper() {
    // Has been moved
    if (func_obj == nullptr || deleter == nullptr) {
      return;
    }
    deleter(func_obj);
  }

private:
  template<typename F>
  static R typed_invoker(void *func, ARGS... args) {
    if constexpr (std::is_void_v<R>) {
      static_cast<F *>(func)->operator()(args...);
    } else {
      return static_cast<F *>(func)->operator()(args...);
    }
  }

  template<typename F>
  static void typed_deleter(void *func) {
    static_cast<F *>(func)->~F();
    RuntimeAllocator::current().free_script_memory(func, sizeof(F));
  }

private:
  void *func_obj;
  R (*invoker)(void *, ARGS...);
  void (*deleter)(void *);
};
