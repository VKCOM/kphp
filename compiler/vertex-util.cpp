// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/vertex-util.h"

#include <set>

#include "common/algorithms/contains.h"
#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include "compiler/name-gen.h"

VertexPtr VertexUtil::get_actual_value(VertexPtr v) {
  if (auto var = v.try_as<op_var>()) {
    if (var->extra_type == op_ex_var_const && var->var_id) {
      return var->var_id->init_val;
    }
  }
  if (auto define_val = v.try_as<op_define_val>()) {
    return get_actual_value(define_val->value());
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

VertexAdaptor<op_func_call> VertexUtil::add_call_arg(VertexPtr to_add, VertexAdaptor<op_func_call> call, bool prepend) {
  std::vector<VertexPtr> new_args;
  new_args.reserve(call->args().size() + 1);
  if (prepend) {
    new_args.emplace_back(to_add);
  }
  for (auto arg : call->args()) {
    new_args.emplace_back(arg);
  }
  if (!prepend) {
    new_args.emplace_back(to_add);
  }

  auto new_call = VertexAdaptor<op_func_call>::create(new_args).set_location(call->location);
  new_call->str_val = call->str_val;
  new_call->func_id = call->func_id;
  return new_call;
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

VertexAdaptor<op_var> VertexUtil::create_superlocal_var(const std::string &name_prefix, FunctionPtr cur_function) {
  auto v = VertexAdaptor<op_var>::create();
  v->str_val = gen_unique_name(name_prefix, cur_function);
  v->extra_type = op_ex_var_superlocal;
  return v;
}

VertexAdaptor<op_switch> VertexUtil::create_switch_vertex(FunctionPtr cur_function, VertexPtr switch_condition, std::vector<VertexPtr> &&cases) {
  auto temp_var_condition_on_switch = create_superlocal_var("condition_on_switch", cur_function);
  auto temp_var_matched_with_one_case = create_superlocal_var("matched_with_one_case", cur_function);
  return VertexAdaptor<op_switch>::create(switch_condition, temp_var_condition_on_switch, temp_var_matched_with_one_case, std::move(cases)).set_location(switch_condition);
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
