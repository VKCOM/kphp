// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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

  void setup(FunctionPtr function) {
    stage::set_name(get_description());
    stage::set_function(function);
    current_function = function;
  }

  virtual ~FunctionPassBase() = default;

  virtual std::string get_description() = 0;

  virtual bool check_function(FunctionPtr /*function*/) const {
    return true;
  }

  virtual void on_start() {  }

  virtual void on_finish() {  }

  virtual VertexPtr on_enter_vertex(VertexPtr vertex) {
    return vertex;
  }

  virtual bool user_recursion(VertexPtr vertex __attribute__((unused))) {
    return false;
  }

  virtual VertexPtr on_exit_vertex(VertexPtr vertex) {
    return vertex;
  }
};

template<class FunctionPassT>
void run_function_pass(VertexPtr &vertex, FunctionPassT *pass) {
  static constexpr size_t STACK_INIT_CAP = 1000;
  constexpr uintptr_t MASK = 1ul << 50;

  std::vector<VertexPtr*> vertex_stack;
  vertex_stack.reserve(STACK_INIT_CAP);

  vertex_stack.push_back(&vertex); // TODO think about insert bit in ptr

  while (!vertex_stack.empty()) {
    VertexPtr *x = vertex_stack.back();

    if (reinterpret_cast<uintptr_t>(x) & MASK) {
      vertex_stack.pop_back();
      x = reinterpret_cast<VertexPtr*>(reinterpret_cast<uintptr_t>(x) ^ MASK);
//      stage::set_location((*x)->get_location());
      *x = pass->on_exit_vertex(*x);
      continue;
    }

    stage::set_location((*x)->get_location());
    *x = pass->on_enter_vertex(*x);
    vertex_stack.back() = reinterpret_cast<VertexPtr*>(reinterpret_cast<uintptr_t>(x) | MASK);
    if (!pass->user_recursion(*x)) {
      for (auto iter = (*x)->rbegin(); iter != (*x)->rend(); ++iter) {
        vertex_stack.push_back(&*iter);
      }
    }
  }
}

template<class FunctionPassT, Operation Op>
void run_function_pass(VertexAdaptor<Op> &vertex, FunctionPassT *pass) {
  VertexPtr v = vertex;
  run_function_pass(v, pass);
  vertex = v.as<Op>();
}

template<typename FunctionPassT>
class FunctionPassTraits {
  struct get_data_helper {
    template<typename T> static decltype(std::declval<T>().get_data()) Test(std::nullptr_t);
    template<typename T> static std::nullptr_t Test(...);
  };
public:
  using GetDataReturnT = decltype(get_data_helper::template Test<FunctionPassT>(nullptr));
  using IsNullPtr = std::is_same<GetDataReturnT, std::nullptr_t>;
  using ExecuteType = typename FunctionPassT::ExecuteType;
  using OutType = typename std::conditional<
    IsNullPtr{},
    ExecuteType,
    std::pair<ExecuteType, GetDataReturnT>
    >::type;
  static OutType create_out_type(ExecuteType &&function, GetDataReturnT &&ret) {
    return create_out_type(std::move(function), std::move(ret), IsNullPtr{});
  }
  static FunctionPtr get_function(const ExecuteType &f) {
    return get_function_impl(f);
  }
  static auto get_data(FunctionPassT &f) {
    return get_data_impl(f, IsNullPtr{});
  }
  static_assert(IsNullPtr::value || std::is_same<ExecuteType, FunctionPtr>::value,
    "Can't have nontrivial both on_finish return type and execute type");
private:
  static OutType create_out_type(ExecuteType &&function, GetDataReturnT &&, std::true_type) {
    return std::move(function);
  }
  static OutType create_out_type(ExecuteType &&function, GetDataReturnT &&ret, std::false_type) {
    return {std::move(function), std::move(ret)};
  }
  static std::nullptr_t get_data_impl(FunctionPassT &, std::true_type) {
    return nullptr;
  }
  static auto get_data_impl(FunctionPassT &f, std::false_type) {
    return f.get_data();
  }

  template<typename T>
  static FunctionPtr get_function_impl(const T &f) { return f.function; }
  template<typename T>
  static FunctionPtr get_function_impl(const std::pair<FunctionPtr, T> &f) { return f.first; }
  static FunctionPtr get_function_impl(const FunctionPtr &f) { return f; }
};

template<class FunctionPassT>
typename FunctionPassTraits<FunctionPassT>::GetDataReturnT run_function_pass(FunctionPtr function, FunctionPassT *pass) {
  if (!pass->check_function(function)) {
    return {};
  }

  static CachedProfiler cache(demangle(typeid(FunctionPassT).name()));
  AutoProfiler prof{*cache};
  Location location_backup_raw = Location{*stage::get_location_ptr()};
  pass->setup(function);
  pass->on_start();
  run_function_pass(function->root, pass);
  pass->on_finish();
  *stage::get_location_ptr() = std::move(location_backup_raw);
  return FunctionPassTraits<FunctionPassT>::get_data(*pass);
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

