#pragma once

namespace vk {

template<class FunctorT>
struct function_holder : FunctorT {
  function_holder(const FunctorT &fun) :
    FunctorT(fun) {
  }

  function_holder(FunctorT &&fun) :
    FunctorT(std::move(fun)) {
  }
};

template<class ResT, class ...Args>
struct function_holder<ResT(*)(Args...)> {
  ResT(*func)(Args...);

  function_holder(decltype(func) func) :
    func(func) {
  }

  ResT operator()(Args ...args) const {
    return func(args...);
  }
};

} // namespace vk
