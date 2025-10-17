// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/allocator/script-malloc-interface.h"

namespace kphp::component::inter_component_session {

class BufferProvider final {
public:
  template<typename R, typename... ARGS>
  class FunctionWrapper final {
  public:
    // No copy semantics
    FunctionWrapper(FunctionWrapper& other) = delete;
    FunctionWrapper& operator=(FunctionWrapper& other) = delete;

    FunctionWrapper(FunctionWrapper&& other) noexcept
        : func_obj(std::exchange(other.func_obj, nullptr)),
          invoker(std::exchange(other.invoker, nullptr)),
          deleter(std::exchange(other.deleter, nullptr)){};

    FunctionWrapper& operator=(FunctionWrapper&& other) noexcept {
      if (this != std::addressof(other)) {
        func_obj = std::exchange(other.func_obj);
        invoker = std::exchange(other.invoker);
        deleter = std::exchange(other.deleter);
      }
      return *this;
    }

    // Invoker and deleter accumulates typed functional objects as pointers
    // It allows to have no depends on functional or lambda objects layout
    template<typename F>
    explicit FunctionWrapper(F&& f) noexcept
        : invoker(typed_invoker<F>),
          deleter(typed_deleter<F>) {
      auto* raw_mem = kphp::memory::script::alloc(sizeof(F));
      new (raw_mem) F{std::forward<F>(f)};
      func_obj = raw_mem;
    }

    explicit operator bool() const noexcept {
      return func_obj != nullptr && invoker != nullptr && deleter != nullptr;
    }

    R operator()(ARGS... args) noexcept {
      if constexpr (std::is_void_v<R>) {
        invoker(func_obj, args...);
      } else {
        return invoker(func_obj, args...);
      }
    }

    ~FunctionWrapper() noexcept {
      // Has been moved
      if (func_obj == nullptr || deleter == nullptr) {
        return;
      }
      deleter(func_obj);
    }

  private:
    template<typename F>
    static R typed_invoker(void* func, ARGS... args) noexcept {
      if constexpr (std::is_void_v<R>) {
        static_cast<F*>(func)->operator()(args...);
      } else {
        return static_cast<F*>(func)->operator()(args...);
      }
    }

    template<typename F>
    static void typed_deleter(void* func) noexcept {
      static_cast<F*>(func)->~F();
      kphp::memory::script::free(func);
    }

  private:
    void* func_obj{};
    R (*invoker)(void*, ARGS...);
    void (*deleter)(void*){};
  };

  using SliceMaker = FunctionWrapper<std::span<std::byte>, size_t>;

  BufferProvider() = delete;
  // No copy semantics
  BufferProvider(BufferProvider& other) = delete;
  BufferProvider& operator=(BufferProvider& other) = delete;

  BufferProvider(BufferProvider&& other) = default;
  BufferProvider& operator=(BufferProvider&& other) = default;

  template<class F>
  explicit BufferProvider(F&& f) noexcept
      : provider(std::forward<F>(f)){};

  auto inline provide() noexcept {
    return provider();
  }

private:
  FunctionWrapper<SliceMaker> provider;
};

} // namespace kphp::component::inter_component_session
