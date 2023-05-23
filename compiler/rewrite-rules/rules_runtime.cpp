// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/rewrite-rules/rules_runtime.h"

#include "compiler/inferring/public.h"
#include "compiler/type-hint.h"
#include "compiler/data/function-data.h"
#include "compiler/vertex-util.h"

namespace rewrite_rules {

Context &get_context() {
  static thread_local Context ctx;
  return ctx;
}

bool add_to_cache(Context &ctx, VertexPtr v) {
  switch (v->type()) {
    case op_int_const:
      ctx.int_const_cache_.push(v.as<op_int_const>());
      return true;

    case op_string:
      ctx.string_cache_.push(v.as<op_string>());
      return true;

    case op_func_call: {
      auto func_call = v.as<op_func_call>();
      kphp_assert(func_call->extra_type == op_ex_none);
      switch (func_call->args().size()) {
        case 0:
          ctx.func_call0_cache_.push(func_call);
          return true;
        case 1:
          ctx.func_call1_cache_.push(func_call);
          return true;
        case 2:
          ctx.func_call2_cache_.push(func_call);
          return true;
        case 3:
          ctx.func_call3_cache_.push(func_call);
          return true;
      }
      return false;
    }

    default:
      return false;
  }
}

void retire_vertex(Context &ctx, VertexPtr v) {
  add_to_cache(ctx, v);
}

// is_pure reports whether the evaluation of v is free of side effects.
bool is_pure(VertexPtr v) {
  switch (v->type()) {
    case op_var:
    case op_int_const:
    case op_float_const:
    case op_string:
    case op_false:
    case op_true:
    case op_null:
      return true;

    case op_conv_array:
    case op_conv_string:
    case op_conv_int:
    case op_conv_float:
    case op_conv_bool:
    case op_conv_mixed:
      return is_pure(v.as<meta_op_unary>()->expr());

    case op_instance_prop:
      return is_pure(v.as<op_instance_prop>()->instance());

    case op_index: {
      auto as_index = v.as<op_index>();
      if (as_index->has_key()) {
        return is_pure(as_index->array()) && is_pure(as_index->key());
      }
      return false;
    }

    default:
      return false;
  }
}

// is_same reports whether x and y represent identical vertices.
//
// for most complex vertices it returns false all the time
// you should avoid using !is_same(x, y) conditions as false could be an indication of "don't know"
// instead of "these two vertices are different"
//
// using this function without negation is the only way to go
bool is_same(VertexPtr x, VertexPtr y) {
  if (x->type() != y->type()) {
    return false;
  }
  if (!x || !y) {
    return x == y;
  }

  switch (x->type()) {
    case op_var:
    case op_string:
    case op_int_const:
    case op_float_const:
      return x->get_string() == y->get_string();

    case op_index: {
      auto x_index = x.as<op_index>();
      auto y_index = y.as<op_index>();
      VertexPtr x_key;
      VertexPtr y_key;
      if (x_index->has_key()) {
        if (y_index->has_key() != x_index->has_key()) {
          return false;
        }
        x_key = x_index->key();
        y_key = y_index->key();
      }
      return is_same(x_index->array(), y_index->array()) &&
             is_same(x_key, y_key);
    }

    case op_instance_prop: {
      auto x_instance_prop = x.as<op_instance_prop>();
      auto y_instance_prop = y.as<op_instance_prop>();
      return is_same(x_instance_prop->instance(), y_instance_prop->instance()) &&
             x_instance_prop->get_string() == y_instance_prop->get_string();
    }

    case op_false:
    case op_true:
    case op_null:
      return true;

    default:
      return false;
  }
}

// is_safe_simple_expr reports whether v is "safe" from the point of the optimizations
// that may depend on the v result lifetimes
//
// it's stricter than is_pure(): the pure expression is allowed to allocate, but
// safe simple expressions are not allowed to do that
static bool is_safe_simple_expr(VertexPtr v) {
  // some notes on why certain vertex types are not included
  // * mixed/array/string conversions could allocate
  switch (v->type()) {
    case op_var:
    case op_int_const:
    case op_float_const:
    case op_string:
    case op_false:
    case op_true:
    case op_null:
      return true;

    case op_conv_int:
    case op_conv_float:
    case op_conv_bool:
      // these conversions never allocate
      return is_safe_simple_expr(v.as<meta_op_unary>()->expr());

    case op_instance_prop:
      // even though $foo->x could allocate $foo if it was null,
      // it will never affect the lifetimes: $foo variable existed before
      // this expression was evaluated
      return is_safe_simple_expr(v.as<op_instance_prop>()->instance());

    case op_index: {
      auto as_index = v.as<op_index>();
      if (as_index->has_key()) {
        return is_safe_simple_expr(as_index->array()) && is_safe_simple_expr(as_index->key());
      }
      return false;
    }
    case op_define_val: {
      return is_safe_simple_expr(v.as<op_define_val>()->value());
    }
    default:
      return false;
  }
}

static VertexAdaptor<op_var> extract_var(VertexPtr v) {
  switch (v->type()) {
    case op_var:
      return v.as<op_var>();

    case op_index:
      return extract_var(v.as<op_index>()->array());

    case op_instance_prop:
      return extract_var(v.as<op_instance_prop>()->instance());

    case op_conv_array_l:
    case op_conv_int_l:
    case op_conv_string_l:
      return extract_var(v.as<meta_op_unary>()->expr());

    default:
      return {};
  }
}

bool contains(VertexPtr tree, VertexPtr x) {
  if (is_same(tree, x)) {
    return true;
  }
  return std::any_of(tree->begin(), tree->end(), [x](VertexPtr e) {
    return contains(e, x);
  });
}

bool contains_var(VertexPtr tree, VertexPtr var) {
  auto x = extract_var(var);
  if (!x) {
    return false;
  }
  return contains(tree, x);
}

VertexPtr to_tmp_string_expr(VertexPtr v, bool safe) {
  v = VertexUtil::unwrap_string_value(v);
  if (v->type() != op_func_call) {
    return {};
  }
  auto as_call = v.as<op_func_call>();
  if (as_call->args().empty()) {
    return {};
  }
  vk::string_view specialization_func = as_call->func_id->get_tmp_string_specialization();
  if (specialization_func.empty()) {
    return {};
  }
  if (safe) {
    // the $subject string is always the first argument
    auto subject_arg = VertexUtil::unwrap_string_value(as_call->args()[0]);
    if (!is_safe_simple_expr(subject_arg)) {
      return {};
    }
  }
  as_call->str_val = specialization_func;
  as_call->func_id = G->get_function(as_call->str_val);
  as_call->auto_inserted = true;
  return as_call;
}

VertexPtr to_tmp_string_expr(VertexPtr v) {
  return to_tmp_string_expr(v, false);
}

VertexPtr to_safe_tmp_string_expr(VertexPtr v) {
  return to_tmp_string_expr(v, true);
}

} // namespace rewrite_rules
