#pragma once


#include "common/type_traits/function_traits.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/function-data.h"
#include "compiler/debug.h"
#include "compiler/stage.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/profiler.h"
#include "compiler/vertex.h"

/*** Function Pass ***/
class FunctionPassBase {
protected:
  FunctionPtr current_function;
public:

  using ExecuteType = FunctionPtr;

  virtual ~FunctionPassBase() {
  }

  virtual std::string get_description() {
    return "unknown function pass";
  }

  virtual bool check_function(FunctionPtr /*function*/) {
    return true;
  }


  bool on_start(FunctionPtr function) {
    if (!check_function(function)) {
      return false;
    }
    stage::set_name(get_description());
    stage::set_function(function);
    current_function = function;
    return true;
  }

  std::nullptr_t on_finish() { return nullptr; }

  VertexPtr on_enter_vertex(VertexPtr vertex) {
    return vertex;
  }

  bool user_recursion(VertexPtr vertex __attribute__((unused))) {
    return false;
  }

  VertexPtr on_exit_vertex(VertexPtr vertex) {
    return vertex;
  }
};

template<class FunctionPassT>
void run_function_pass(VertexPtr &vertex, FunctionPassT *pass) {
  stage::set_location(vertex->get_location());

  vertex = pass->on_enter_vertex(vertex);

  if (!pass->user_recursion(vertex)) {
    for (auto &i : *vertex) {
      run_function_pass(i, pass);
    }
  }

  vertex = pass->on_exit_vertex(vertex);
}

template<class FunctionPassT, Operation Op>
void run_function_pass(VertexAdaptor<Op> &vertex, FunctionPassT *pass) {
  VertexPtr v = vertex;
  run_function_pass(v, pass);
  vertex = v.as<Op>();
}

template<typename FunctionPassT>
class FunctionPassTraits {
public:
  using OnFinishReturnT = typename vk::function_traits<decltype(&FunctionPassT::on_finish)>::ResultType;
  using IsNullPtr = std::is_same<OnFinishReturnT, std::nullptr_t>;
  using ExecuteType = typename FunctionPassT::ExecuteType;
  using OutType = typename std::conditional<
    IsNullPtr::value,
    ExecuteType,
    std::pair<ExecuteType, OnFinishReturnT>
    >::type;
  static OutType create_out_type(ExecuteType &&function, OnFinishReturnT &&ret) {
    return create_out_type(std::forward<ExecuteType>(function), std::forward<OnFinishReturnT>(ret), IsNullPtr{});
  }
  static FunctionPtr get_function(const ExecuteType &f) {
    return get_function_impl(f);
  }
  static_assert(IsNullPtr::value || std::is_same<ExecuteType, FunctionPtr>::value,
    "Can't have nontrivial both on_finish return type and execute type");
private:
  static OutType create_out_type(ExecuteType &&function, OnFinishReturnT &&, std::true_type) {
    return std::forward<ExecuteType>(function);
  }
  static OutType create_out_type(ExecuteType &&function, OnFinishReturnT &&ret, std::false_type) {
    return {std::forward<ExecuteType>(function), std::forward<OnFinishReturnT>(ret)};
  }

  template<typename T>
  static FunctionPtr get_function_impl(const T &f) { return f.function; }
  template<typename T>
  static FunctionPtr get_function_impl(const std::pair<FunctionPtr, T> &f) { return f.first; }
  static FunctionPtr get_function_impl(const FunctionPtr &f) { return f; }
};

template<class FunctionPassT>
typename FunctionPassTraits<FunctionPassT>::OnFinishReturnT run_function_pass(FunctionPtr function, FunctionPassT *pass) {
  static CachedProfiler cache(demangle(typeid(FunctionPassT).name()));
  AutoProfiler prof{*cache};
  if (!pass->on_start(function)) {
    return typename FunctionPassTraits<FunctionPassT>::OnFinishReturnT();
  }
  run_function_pass(function->root, pass);
  return pass->on_finish();
}


template<class FunctionPassT>
class FunctionPassF {
public:
  using need_profiler = std::false_type;
  using Traits = FunctionPassTraits<FunctionPassT>;
  using OutType = typename Traits::OutType;
  void execute(typename Traits::ExecuteType function, DataStream<OutType> &os) {
    FunctionPassT pass;
    auto x = run_function_pass(Traits::get_function(function), &pass);
    if (stage::has_error()) {
      return;
    }
    os << Traits::create_out_type(std::move(function), std::move(x));
  }
};

