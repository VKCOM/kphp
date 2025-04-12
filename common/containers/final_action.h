// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <utility>

namespace vk {

template<class FuncT>
struct final_action {
  explicit final_action(FuncT func)
      : final_function{std::move(func)} {}

  final_action(final_action&& other) noexcept
      : final_function(std::move(other.final_function)),
        disabled(other.disabled) {
    other.disabled = true;
  }

  void disable() {
    disabled = true;
  }

  final_action(const final_action&) = delete;
  final_action& operator=(final_action&& other) = delete;
  final_action& operator=(const final_action&) = delete;

  ~final_action() {
    if (!disabled) {
      final_function();
    }
  }

private:
  FuncT final_function;
  bool disabled = false;
};

template<class FuncT>
final_action<FuncT> finally(FuncT&& final_function) {
  return final_action<FuncT>{std::forward<FuncT>(final_function)};
}

} // namespace vk
