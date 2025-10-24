// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::component::inter_component_session::details {

// Script memory allocated function wrapper. Allows the placement of functional objects in containers
// Such 'bicycle' is introduced due to the following reasons:
// * std::function isn't able to work with custom allocators
// * kphp::coro::task cannot be invoked more than once
// * Parametric polymorphism isn't appropriate since different functional objects have to be stored in containers
template<typename R, typename... ARGS>
class function_wrapper final {
public:
  function_wrapper() = delete;

  // No copy semantics
  function_wrapper(function_wrapper& other) = delete;
  function_wrapper& operator=(function_wrapper& other) = delete;

  function_wrapper(function_wrapper&& other) noexcept
      : raw_mem(std::move(other.raw_mem)),
        func_obj(std::exchange(other.func_obj, nullptr)),
        invoker(std::exchange(other.invoker, nullptr)),
        deleter(std::exchange(other.deleter, nullptr)){};

  function_wrapper& operator=(function_wrapper&& other) noexcept {
    if (this != std::addressof(other)) {
      raw_mem = std::move(other.raw_mem);
      func_obj = std::exchange(other.func_obj, nullptr);
      invoker = std::exchange(other.invoker, nullptr);
      deleter = std::exchange(other.deleter, nullptr);
    }
    return *this;
  }

  // Invoker and deleter accumulates typed functional objects as pointers
  // It allows to have no depends on functional or lambda objects layout
  template<typename F>
  requires std::invocable<F, ARGS...> && std::is_same_v<std::invoke_result_t<F, ARGS...>, R>
  explicit function_wrapper(F&& f) noexcept
      : raw_mem(raw_mem_alloc<F>(), std::addressof(raw_mem_free)),
        func_obj(new(raw_mem.get()) F{std::forward<F>(f)}),
        invoker(typed_invoker<F>),
        deleter(typed_deleter<F>) {}

  explicit operator bool() const noexcept {
    return raw_mem != nullptr && func_obj != nullptr && invoker != nullptr && deleter != nullptr;
  }

  R operator()(ARGS... args) noexcept {
    kphp::log::assertion(func_obj != nullptr);
    return invoker(func_obj, std::forward<ARGS...>(args...));
  }

  ~function_wrapper() noexcept {
    // Has been moved
    if (func_obj == nullptr || deleter == nullptr) {
      return;
    }
    deleter(func_obj);
  }

private:
  template<typename F>
  static R typed_invoker(const void* func_obj, ARGS... args) noexcept {
    return std::launder(static_cast<const F*>(func_obj))->operator()(std::forward<ARGS...>(args...));
  }

  template<typename F>
  static void typed_deleter(const void* func_obj) noexcept {
    std::launder(static_cast<const F*>(func_obj))->~F();
  }

  template<typename F>
  static void* raw_mem_alloc() noexcept {
    constexpr size_t alignment{alignof(F)};
    constexpr size_t size{sizeof(F)};
    // Allocate more space to ensure alignment can be met
    const auto mem{kphp::memory::script::alloc(size + alignment - 1)};
    kphp::log::assertion(mem != nullptr);
    return mem;
  }

  static void raw_mem_free(void* raw_mem) noexcept {
    kphp::memory::script::free(raw_mem);
  }

  std::unique_ptr<void, decltype(std::addressof(raw_mem_free))> raw_mem;
  const void* func_obj;
  R (*invoker)(const void*, ARGS...);
  void (*deleter)(const void*);
};

} // namespace kphp::component::inter_component_session::details
