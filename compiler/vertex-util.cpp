// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/vertex-util.h"

#include "common/algorithms/contains.h"
#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include <set>

VertexPtr VertexUtil::get_actual_value(VertexPtr v) {
  if (auto var = v.try_as<op_var>()) {
    if (var->extra_type == op_ex_var_const && var->var_id) {
      return var->var_id->init_val;
    }
  }
  return v;
}

const std::string *VertexUtil::get_constexpr_string(VertexPtr v) {
  v = VertexUtil::get_actual_value(v);
  if (auto conv_vertex = v.try_as<op_conv_string>()) {
    return get_constexpr_string(conv_vertex->expr());
  }
  if (auto str_vertex = v.try_as<op_string>()) {
    return &str_vertex->get_string();
  }
  return nullptr;
}

VertexPtr VertexUtil::get_call_arg_ref(int arg_num, VertexPtr v_func_call) {
  if (arg_num > 0) {
    auto call_args = v_func_call.as<op_func_call>()->args();
    return arg_num <= call_args.size() ? call_args[arg_num - 1] : VertexPtr{};
  }
  return {};
}

VertexPtr VertexUtil::create_int_const(int64_t number) {
  auto int_v = VertexAdaptor<op_int_const>::create();
  int_v->str_val = std::to_string(number);
  return int_v;
}

VertexAdaptor<op_string> VertexUtil::create_string_const(const std::string &s) {
  auto string_v = VertexAdaptor<op_string>::create();
  string_v->set_string(s);
  return string_v;
}

VertexAdaptor<op_seq> VertexUtil::embrace(VertexPtr v) {
  if (auto seq = v.try_as<op_seq>()) {
    return seq;
  }
  return VertexAdaptor<op_seq>::create(v).set_location(v);
}

void VertexUtil::func_force_return(VertexAdaptor<op_function> func, VertexPtr val) {
  VertexPtr cmd = func->cmd();
  assert (cmd->type() == op_seq);

  VertexAdaptor<op_return> return_node;
  if (val) {
    return_node = VertexAdaptor<op_return>::create(val);
  } else {
    return_node = VertexAdaptor<op_return>::create();
  }

  std::vector<VertexPtr> next = cmd->get_next();
  next.push_back(return_node);
  func->cmd_ref() = VertexAdaptor<op_seq>::create(next);
}

bool VertexUtil::is_superglobal(const std::string &s) {
  static std::set<std::string> names = {
    "_SERVER",
    "_GET",
    "_POST",
    "_FILES",
    "_COOKIE",
    "_REQUEST",
    "_ENV"
  };
  return vk::contains(names, s);
}

bool VertexUtil::is_constructor_call(VertexAdaptor<op_func_call> call) {
  return !call->args().empty() && call->str_val == ClassData::NAME_OF_CONSTRUCT;
}

bool VertexUtil::is_positive_constexpr_int(VertexPtr v) {
  auto actual_value = get_actual_value(v).try_as<op_int_const>();
  return actual_value && parse_int_from_string(actual_value) >= 0;
}

bool VertexUtil::is_const_int(VertexPtr root) {
  switch (root->type()) {
    case op_int_const:
      return true;
    case op_minus:
    case op_plus:
    case op_not:
      return is_const_int(root.as<meta_op_unary>()->expr());
    case op_add:
    case op_mul:
    case op_sub:
    case op_div:
    case op_and:
    case op_or:
    case op_xor:
    case op_shl:
    case op_shr:
    case op_mod:
    case op_pow:
      return is_const_int(root.as<meta_op_binary>()->lhs()) && is_const_int(root.as<meta_op_binary>()->rhs());
    default:
      break;
  }
  return false;
}

VertexPtr VertexUtil::create_conv_to(PrimitiveType targetType, VertexPtr x) {
  switch (targetType) {
    case tp_int:
      return VertexAdaptor<op_conv_int>::create(x).set_location(x);

    case tp_bool:
      return VertexAdaptor<op_conv_bool>::create(x).set_location(x);

    case tp_string:
      return VertexAdaptor<op_conv_string>::create(x).set_location(x);

    case tp_float:
      return VertexAdaptor<op_conv_float>::create(x).set_location(x);

    case tp_array:
      return VertexAdaptor<op_conv_array>::create(x).set_location(x);

    case tp_regexp:
      return VertexAdaptor<op_conv_regexp>::create(x).set_location(x);

    case tp_mixed:
      return VertexAdaptor<op_conv_mixed>::create(x).set_location(x);

    default:
      return x;
  }
}

VertexAdaptor<meta_op_unary> VertexUtil::create_conv_to_lval(PrimitiveType targetType, VertexPtr x) {
  switch (targetType) {
    case tp_array : return VertexAdaptor<op_conv_array_l>::create(x).set_location(x);
    case tp_int   : return VertexAdaptor<op_conv_int_l>::create(x).set_location(x);
    case tp_string: return VertexAdaptor<op_conv_string_l>::create(x).set_location(x);
    default:
      return {};
  }
}

VertexPtr VertexUtil::unwrap_array_value(VertexPtr v) {
  while (v->type() == op_conv_array) {
    v = v.as<op_conv_array>()->expr();
  }
  return get_actual_value(v);
}

VertexPtr VertexUtil::unwrap_int_value(VertexPtr v) {
  while (v->type() == op_conv_int) {
    v = v.as<op_conv_int>()->expr();
  }
  return get_actual_value(v);
}

VertexPtr VertexUtil::unwrap_float_value(VertexPtr v) {
  while (v->type() == op_conv_float) {
    v = v.as<op_conv_float>()->expr();
  }
  return get_actual_value(v);
}

VertexPtr VertexUtil::unwrap_string_value(VertexPtr v) {
  while (v->type() == op_conv_string) {
    v = v.as<op_conv_string>()->expr();
  }
  return get_actual_value(v);
}

VertexPtr VertexUtil::unwrap_bool_value(VertexPtr v) {
  while (v->type() == op_conv_bool) {
    v = v.as<op_conv_bool>()->expr();
  }
  return get_actual_value(v);
}
