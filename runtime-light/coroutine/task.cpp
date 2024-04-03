#include "runtime-light/coroutine/task.h"

#include "runtime-light/component/component.h"

void * push(std::size_t n) {
  return get_component_context()->coroutine_stack.push(n);
}

void pop(void * ptr, std::size_t n) {
  php_assert(get_component_context() != nullptr);
  get_component_context()->coroutine_stack.pop(ptr, n);
}