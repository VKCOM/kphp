// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// This file is intended to be used inside auto-generated rewrite rules.
// Do not include it from somewhere else.

#include <string>
#include "compiler/vertex.h"
#include "compiler/compiler-core.h"

namespace rewrite_rules {

template<Operation Op>
class VertexCache {
private:

  std::vector<VertexAdaptor<Op>> stack_;

public:

  VertexAdaptor<Op> pop() {
    if (!stack_.empty()) {
      auto v = stack_.back();
      stack_.pop_back();
      return v;
    }
    return {};
  }

  void push(VertexAdaptor<Op> v) {
    constexpr int MAX_CACHE_SIZE = 40;
    if (stack_.size() < MAX_CACHE_SIZE) {
      stack_.push_back(v);
    }
  }
};

struct Context {
  VertexCache<op_int_const> int_const_cache_;
  VertexCache<op_string> string_cache_;
  VertexCache<op_func_call> func_call0_cache_; // 0 call args
  VertexCache<op_func_call> func_call1_cache_; // 1 call arg
  VertexCache<op_func_call> func_call2_cache_; // 2 call args
  VertexCache<op_func_call> func_call3_cache_; // 3 call args

  std::vector<VertexPtr> tmp_args_vector_;
};

Context &get_context();

bool is_pure(VertexPtr v);
bool is_same(VertexPtr x, VertexPtr y);
bool contains(VertexPtr tree, VertexPtr x);
bool contains_var(VertexPtr tree, VertexPtr var);

VertexPtr to_tmp_string_expr(VertexPtr v);
VertexPtr to_safe_tmp_string_expr(VertexPtr v);

template<Operation Op, class T>
VertexAdaptor<Op> vertex_cast(T v) {
  if (v->type() != Op) {
    return {};
  }
  return v.template as<Op>();
}

void retire_vertex(Context &ctx, VertexPtr v);

inline void set_func_call_args(VertexAdaptor<op_func_call> call __attribute__((unused))) {}

inline void set_func_call_args(VertexAdaptor<op_func_call> call, const std::vector<VertexPtr> &args) {
  for (int i = 0; i < args.size(); i++) {
    call->args()[i] = args[i];
  }
}

template<class... Args>
void set_func_call_args(VertexAdaptor<op_func_call> call, const Args &...args) {
  int i = 0;
  ((call->args()[i++] = args), ...);
}

template<Operation Op, class... Args>
VertexAdaptor<Op> create_vertex_with_string(Context &ctx, std::string &&str_val, Args &&...args) {
  // allocate a vertex or take it from the cache
  VertexAdaptor<Op> v;

  if constexpr (Op == op_int_const) {
    v = ctx.int_const_cache_.pop();
  } else if constexpr (Op == op_string) {
    v = ctx.string_cache_.pop();
  } else if constexpr (Op == op_func_call) {
    int num_args = vertex_inner<meta_op_base>::get_children_size(std::forward<Args>(args)...);
    if (num_args == 0) {
      v = ctx.func_call0_cache_.pop();
    } else if (num_args == 1) {
      v = ctx.func_call1_cache_.pop();
    } else if (num_args == 2) {
      v = ctx.func_call2_cache_.pop();
    } else if (num_args == 3) {
      v = ctx.func_call3_cache_.pop();
    }
    if (v) {
      set_func_call_args(v, std::forward<Args>(args)...);
    }
  }

  if (!v) {
    v = VertexAdaptor<Op>::create(std::forward<Args>(args)...);
  }

  // assign the op-specific data, if necessary
  if constexpr (Op == op_func_call) {
    v->func_id = G->get_function(str_val);
    v->auto_inserted = true;
  }

  v->set_string(str_val);

  return v;
}

template<Operation Op>
VertexAdaptor<Op> create_vertex_with_string(Context &ctx, std::string &&name, const VertexConstRange &args) {
  ctx.tmp_args_vector_.insert(ctx.tmp_args_vector_.end(), args.begin(), args.end());
  auto result = create_vertex_with_string<Op>(ctx, std::move(name), ctx.tmp_args_vector_);
  ctx.tmp_args_vector_.clear();
  return result;
}

template<Operation Op, class... Args>
VertexAdaptor<Op> create_vertex(Context &ctx, Args &&...args) {
  static_cast<void>(ctx);
  auto v = VertexAdaptor<Op>::create(std::forward<Args>(args)...);
  return v;
}

} // namespace rewrite_rules
