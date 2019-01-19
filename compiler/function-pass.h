#pragma once

#include "compiler/common.h"
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
  virtual ~FunctionPassBase() {
  }

  struct LocalT {
  };

  virtual string get_description() {
    return "unknown function pass";
  }

  bool default_check_function(FunctionPtr function) {
    return function && function->root;
  }

  virtual bool check_function(FunctionPtr function) {
    return default_check_function(function);
  }

  void init() {
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

  nullptr_t on_finish() { return nullptr; }

  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local __attribute__((unused))) {
    return vertex;
  }

  void on_enter_edge(VertexPtr vertex __attribute__((unused)),
                     LocalT *local __attribute__((unused)),
                     VertexPtr dest_vertex __attribute__((unused)),
                     LocalT *dest_local __attribute__((unused))) {
  }

  template<class VisitT>
  bool user_recursion(VertexPtr vertex __attribute__((unused)),
                      LocalT *local __attribute__((unused)),
                      VisitT &visit __attribute__((unused))) {
    return false;
  }

  bool need_recursion(VertexPtr vertex __attribute__((unused)),
                      LocalT *local __attribute__((unused))) {
    return true;
  }

  void on_exit_edge(VertexPtr vertex __attribute__((unused)),
                    LocalT *local __attribute__((unused)),
                    VertexPtr from_vertex __attribute__((unused)),
                    LocalT *from_local __attribute__((unused))) {
  }

  VertexPtr on_exit_vertex(VertexPtr vertex, LocalT *local __attribute__((unused))) {
    return vertex;
  }
};

template<class FunctionPassT>
VertexPtr run_function_pass(VertexPtr vertex, FunctionPassT *pass, typename FunctionPassT::LocalT *local);

template<class FunctionPassT>
struct VisitVertex {
  VertexPtr vertex;
  FunctionPassT *pass;
  typename FunctionPassT::LocalT *local;

  VisitVertex(VertexPtr vertex, FunctionPassT *pass, typename FunctionPassT::LocalT *local) :
    vertex(vertex),
    pass(pass),
    local(local) {
  }

  inline void operator()(VertexPtr &child) {
    typename FunctionPassT::LocalT child_local;
    pass->on_enter_edge(vertex, local, child, &child_local);
    child = run_function_pass(child, pass, &child_local);
    pass->on_exit_edge(vertex, local, child, &child_local);
  }
};

template<class FunctionPassT>
VertexPtr run_function_pass(VertexPtr vertex, FunctionPassT *pass, typename FunctionPassT::LocalT *local) {
  stage::set_location(vertex->get_location());

  vertex = pass->on_enter_vertex(vertex, local);

  VisitVertex<FunctionPassT> visit(vertex, pass, local);
  if (!pass->user_recursion(vertex, local, visit) && pass->need_recursion(vertex, local)) {
    for (auto &i : *vertex) {
      visit(i);
    }
  }

  vertex = pass->on_exit_vertex(vertex, local);

  return vertex;
}

template<typename FunctionPassT>
class FunctionPassTraits {
public:
  using OnFinishReturnT = typename vk::function_traits<decltype(&FunctionPassT::on_finish)>::ResultType;
  using IsNullPtr = std::is_same<OnFinishReturnT, nullptr_t>;
  using OutType = typename std::conditional<
    IsNullPtr::value,
    FunctionPtr,
    std::pair<FunctionPtr, OnFinishReturnT>
    >::type;
  static OutType create_out_type(FunctionPtr function, OnFinishReturnT &&ret) {
    return create_out_type(function, std::forward<OnFinishReturnT>(ret), IsNullPtr{});
  }
private:
  static OutType create_out_type(FunctionPtr function, OnFinishReturnT &&, std::true_type) {
    return function;
  }
  static OutType create_out_type(FunctionPtr function, OnFinishReturnT &&ret, std::false_type) {
    return {function, ret};
  }
};

template<class FunctionPassT>
typename FunctionPassTraits<FunctionPassT>::OnFinishReturnT run_function_pass(FunctionPtr function, FunctionPassT *pass) {
  static CachedProfiler cache(demangle(typeid(FunctionPassT).name()));
  AutoProfiler prof{*cache};
  pass->init();
  if (!pass->on_start(function)) {
    return typename FunctionPassTraits<FunctionPassT>::OnFinishReturnT();
  }
  typename FunctionPassT::LocalT local;
  function->root = run_function_pass(function->root, pass, &local);
  return pass->on_finish();
}


template<class FunctionPassT>
class FunctionPassF {
public:
  using need_profiler = std::false_type;
  using OutType = typename FunctionPassTraits<FunctionPassT>::OutType;
  void execute(FunctionPtr function, DataStream<OutType> &os) {
    FunctionPassT pass;
    auto x = run_function_pass(function, &pass);
    if (stage::has_error()) {
      return;
    }
    os << FunctionPassTraits<FunctionPassT>::create_out_type(function, std::move(x));
  }
};

