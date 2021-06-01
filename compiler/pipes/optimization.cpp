// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/optimization.h"

#include <sstream>

#include "common/algorithms/hashes.h"

#include "compiler/data/class-data.h"
#include "compiler/format-parser.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"

namespace {

bool can_init_value_be_removed(VertexPtr init_value, const VarPtr &variable) {
  const auto *variable_type = tinf::get_type(variable);
  if (variable_type->use_optional() ||
      vk::any_of_equal(variable_type->ptype(), tp_Class, tp_mixed)) {
    return init_value->type() == op_null;
  }

  const auto *init_type = tinf::get_type(init_value);
  if (init_value->extra_type != op_ex_var_const ||
      init_type->use_optional() ||
      init_type->ptype() != variable_type->ptype()) {
    return false;
  }

  switch (init_type->ptype()) {
    case tp_string: {
      const auto *init_string = GenTree::get_constexpr_string(init_value);
      return init_string && init_string->empty();
    }
    case tp_array:
      return init_type->lookup_at_any_key()->get_real_ptype() == tp_any;
    default:
      return false;
  }
}

VarPtr cast_const_array_type(VertexPtr &type_acceptor, const TypeData *required_type) noexcept {
  std::stringstream ss;
  ss << type_acceptor->get_string() << "$" << std::hex << vk::std_hash(type_out(required_type));
  std::string name = ss.str();
  bool is_new = true;
  VarPtr var_id = G->get_global_var(name, VarData::var_const_t, type_acceptor, &is_new);
  var_id->tinf_node.copy_type_from(required_type);  // not inside if(is_new) to avoid race conditions when one thread creates and another uses faster
  if (is_new) {
    var_id->dependency_level = type_acceptor.as<op_var>()->var_id->dependency_level + 1;
  }

  auto casted_var = VertexAdaptor<op_var>::create();
  casted_var->str_val = std::move(name);
  casted_var->extra_type = op_ex_var_const;
  casted_var->location = type_acceptor->location;

  casted_var->var_id = var_id;
  type_acceptor = casted_var;
  return var_id;
}

void cast_array_creation_type(VertexAdaptor<op_array> op_array_vertex, const TypeData *required_type) noexcept {
  if (required_type->get_real_ptype() == tp_mixed) {
    required_type = TypeData::get_type(tp_array, tp_mixed);
  } else if (required_type->use_optional()) {
    required_type = TypeData::create_array_of(required_type->lookup_at_any_key());
  }
  op_array_vertex->tinf_node.set_type(required_type);
}

void explicit_cast_array_type(VertexPtr &type_acceptor, const TypeData *required_type, std::set<VarPtr> *new_var_out = nullptr) noexcept {
  auto existed_type = tinf::get_type(type_acceptor);
  if (existed_type->get_real_ptype() != tp_array ||
      !is_implicit_array_conversion(existed_type, required_type)) {
    return;
  }
  kphp_assert(vk::any_of_equal(required_type->get_real_ptype(), tp_array, tp_mixed));
  if (type_acceptor->extra_type == op_ex_var_const) {
    auto var_id = cast_const_array_type(type_acceptor, required_type);
    if (new_var_out) {
      new_var_out->emplace(var_id);
    }
  }
  if (auto op_array_vertex = type_acceptor.try_as<op_array>()) {
    cast_array_creation_type(op_array_vertex, required_type);
  }
}

} // namespace

VertexPtr OptimizationPass::optimize_set_push_back(VertexAdaptor<op_set> set_op) {
  if (set_op->lhs()->type() != op_index) {
    return set_op;
  }
  VertexAdaptor<op_index> index = set_op->lhs().as<op_index>();
  if (index->has_key() && set_op->rl_type != val_none) {
    return set_op;
  }

  VertexPtr a, b, c;
  a = index->array();
  if (index->has_key()) {
    b = index->key();
  }
  c = set_op->rhs();

  VertexPtr result;

  if (!b) {
    // '$s[] = ...' is forbidden for non-array types;
    // for arrays it's converted to push_back
    PrimitiveType a_ptype = tinf::get_type(a)->get_real_ptype();
    kphp_error (a_ptype == tp_array || a_ptype == tp_mixed,
                fmt_format("Can not use [] for {}", type_out(tinf::get_type(a))));

    if (set_op->rl_type == val_none) {
      result = VertexAdaptor<op_push_back>::create(a, c);
    } else {
      result = VertexAdaptor<op_push_back_return>::create(a, c);
    }
  } else {
    result = VertexAdaptor<op_set_value>::create(a, b, c);
  }
  result->location = set_op->get_location();
  result->extra_type = op_ex_internal_func;
  result->rl_type = set_op->rl_type;
  return result;
}
void OptimizationPass::collect_concat(VertexPtr root, vector<VertexPtr> *collected) {
  if (root->type() == op_string_build || root->type() == op_concat) {
    for (auto i : *root) {
      collect_concat(i, collected);
    }
  } else {
    if (const auto call = try_convert_expr_to_call_to_string_method(root)) {
      root = call;
    }
    collected->push_back(root);
  }
}
VertexPtr OptimizationPass::optimize_string_building(VertexPtr root) {
  vector<VertexPtr> collected;
  collect_concat(root, &collected);
  auto new_root = VertexAdaptor<op_string_build>::create(collected);
  new_root->location = root->get_location();
  new_root->rl_type = root->rl_type;

  return new_root;
}
VertexPtr OptimizationPass::optimize_postfix_inc(VertexPtr root) {
  if (root->rl_type == val_none) {
    auto new_root = VertexAdaptor<op_prefix_inc>::create(root.as<op_postfix_inc>()->expr());
    new_root->rl_type = root->rl_type;
    new_root->location = root->get_location();
    root = new_root;
  }
  return root;
}
VertexPtr OptimizationPass::optimize_postfix_dec(VertexPtr root) {
  if (root->rl_type == val_none) {
    auto new_root = VertexAdaptor<op_prefix_dec>::create(root.as<op_postfix_dec>()->expr());
    new_root->rl_type = root->rl_type;
    new_root->location = root->get_location();
    root = new_root;
  }
  return root;
}
VertexPtr OptimizationPass::optimize_index(VertexAdaptor<op_index> index) {
  if (!index->has_key()) {
    if (index->rl_type == val_l) {
      kphp_error (0, "Unsupported []");
    } else {
      kphp_error (0, "Cannot use [] for reading");
    }
  }
  return index;
}
VertexPtr OptimizationPass::remove_extra_conversions(VertexPtr root) {
  while (OpInfo::type(root->type()) == conv_op || vk::any_of_equal(root->type(), op_conv_array_l, op_conv_int_l, op_conv_string_l)) {
    VertexPtr expr = root.as<meta_op_unary>()->expr();
    const TypeData *tp = tinf::get_type(expr);
    VertexPtr res;
    if (!tp->use_optional()) {
      if (vk::any_of_equal(root->type(), op_conv_int, op_conv_int_l) && tp->ptype() == tp_int) {
        res = expr;
      } else if (root->type() == op_conv_bool && tp->ptype() == tp_bool) {
        res = expr;
      } else if (root->type() == op_conv_float && tp->ptype() == tp_float) {
        res = expr;
      } else if (vk::any_of_equal(root->type(), op_conv_string, op_conv_string_l) && tp->ptype() == tp_string) {
        res = expr;
      } else if (vk::any_of_equal(root->type(), op_conv_array, op_conv_array_l) && tp->get_real_ptype() == tp_array) {
        res = expr;
      } else if (root->type() == op_force_mixed && tp->ptype() == tp_void) {
          expr->rl_type = val_none;
          res = VertexAdaptor<op_seq_rval>::create(expr, VertexAdaptor<op_null>::create());
      }
    }
    if (root->type() == op_conv_mixed) {
      res = expr;
    }
    if (root->type() == op_conv_drop_null){
      if (!tp->can_store_null()) {
        res = expr;
      }
    }
    if (root->type() == op_conv_drop_false){
      if (!tp->can_store_false()) {
        res = expr;
      }
    }
    if (res) {
      res->rl_type = root->rl_type;
      root = res;
    } else {
      break;
    }
  }
  return root;
}

// The function will try to convert the expression to a call to the
// __toString method if the expression has a class type and the class
// has such a method.
// If the conversion fails, an empty VertexPtr will be returned.
VertexPtr OptimizationPass::try_convert_expr_to_call_to_string_method(VertexPtr expr) {
  const auto *type = tinf::get_type(expr);
  if (type == nullptr) {
    return {};
  }

  const auto klass = type->class_type();
  if (!klass) {
    return {};
  }

  const auto *to_string_method = klass->get_instance_method(ClassData::NAME_OF_TO_STRING);
  if (to_string_method == nullptr) {
    kphp_error(0, fmt_format("Converting to a string of a class {} that does not contain a __toString() method",
                             klass->get_name()));
    return {};
  }

  const auto args = std::vector<VertexPtr>{expr};
  auto call_function = VertexAdaptor<op_func_call>::create(args);
  call_function->set_string(std::string{to_string_method->local_name()});
  call_function->func_id = to_string_method->function;

  return call_function;
}

VertexPtr OptimizationPass::convert_strval_to_magic_tostring_method_call(VertexAdaptor<op_conv_string> conv) {
  const auto expr = conv->expr();

  if (const auto call = try_convert_expr_to_call_to_string_method(expr)) {
    return call;
  }

  return conv;
}

VertexPtr OptimizationPass::on_enter_vertex(VertexPtr root) {
  root = remove_extra_conversions(root);

  if (auto set_vertex = root.try_as<op_set>()) {
    explicit_cast_array_type(set_vertex->rhs(), tinf::get_type(set_vertex->lhs()), &current_function->explicit_const_var_ids);
    root = optimize_set_push_back(set_vertex);
  } else if (root->type() == op_string_build || root->type() == op_concat) {
    root = optimize_string_building(root);
  } else if (root->type() == op_postfix_inc) {
    root = optimize_postfix_inc(root);
  } else if (root->type() == op_postfix_dec) {
    root = optimize_postfix_dec(root);
  } else if (root->type() == op_index) {
    root = optimize_index(root.as<op_index>());
  } else if (auto param = root.try_as<op_foreach_param>()) {
    if (!param->x()->ref_flag) {
      auto temp_var = root.as<op_foreach_param>()->temp_var().as<op_var>();
      if (temp_var && temp_var->extra_type == op_ex_var_superlocal) {     // see CreateSwitchForeachVarsPass
        temp_var->var_id->needs_const_iterator_flag = true;
      }
    }
  } else if (auto func_param = root.try_as<op_func_param>()) {
    if (func_param->has_default_value() && func_param->default_value()) {
      explicit_cast_array_type(func_param->default_value(), tinf::get_type(func_param->var()), &current_function->explicit_header_const_var_ids);
    }
  } else if (auto func_call = root.try_as<op_func_call>()) {
    auto func = func_call->func_id;
    if (!func->is_extern()) {
      auto args = func_call->args();
      const auto &params = func->param_ids;
      const size_t elements = std::min(static_cast<size_t>(args.size()), params.size());
      for (size_t index = 0; index < elements; ++index) {
        explicit_cast_array_type(args[index], tinf::get_type(params[index]), &current_function->explicit_const_var_ids);
      }
    }

    if (vk::any_of_equal(func->name, "sprintf", "vsprintf")) {
      root = optimize_printf_like_call(func_call);
    } else if (vk::any_of_equal(func->name, "printf", "vprintf")) {
      constexpr auto need_output = true;
      root = optimize_printf_like_call(func_call, need_output);
    } else if (vk::any_of_equal(func->name, "fprintf", "vfprintf")) {
      constexpr auto need_output = true;
      constexpr auto to_file = true;
      root = optimize_printf_like_call(func_call, need_output, to_file);
    }

  } else if (auto op_array_vertex = root.try_as<op_array>()) {
    if (!var_init_expression_optimization_depth_) {
      for (auto &array_element : *op_array_vertex) {
        const auto *required_type = tinf::get_type(op_array_vertex)->lookup_at_any_key();
        if (vk::any_of_equal(array_element->type(), op_var, op_array)) {
          explicit_cast_array_type(array_element, required_type, &current_function->explicit_const_var_ids);
        } else if (auto array_key_value = array_element.try_as<op_double_arrow>()) {
          explicit_cast_array_type(array_key_value->value(), required_type, &current_function->explicit_const_var_ids);
        }
      }
    }
  } else if (auto op_return_vertex = root.try_as<op_return>()) {
    if (op_return_vertex->has_expr()) {
      explicit_cast_array_type(op_return_vertex->expr(), tinf::get_type(current_function, -1), &current_function->explicit_const_var_ids);
    }
  } else if (auto op_conv_string_vertex = root.try_as<op_conv_string>()) {
    root = convert_strval_to_magic_tostring_method_call(op_conv_string_vertex);
  }

  if (root->rl_type != val_none/* && root->rl_type != val_error*/) {
    tinf::get_type(root);
  }
  return root;
}

struct FormatCallInfo {
  FormatCallInfo() : args(VertexRange(Vertex::iterator{}, Vertex::iterator{})) {}
  // stores a variable that contains all the call arguments,
  // if all arguments are constant
  VertexAdaptor<op_var> var;
  // stores an array that contains all the call arguments,
  // if at least one argument is a function call or a variable
  VertexAdaptor<op_array> array;
  // flag for the presence of a value in format_args_var
  bool is_var{false};
  // stores a set of vertices of the call arguments from var or array
  VertexRange args;
};

VertexPtr OptimizationPass::optimize_printf_like_call(VertexAdaptor<op_func_call> &call, bool need_output, bool to_file) {
  // fprintf, vfprintf have a format string as the second argument,
  // so we need to shift the arguments when getting by index.
  const auto arg_shift = to_file ? 1 : 0;
  const auto args = call->args();
  auto format_arg_raw = args[0 + arg_shift];

  if (format_arg_raw->type() == op_conv_string) {
    format_arg_raw = format_arg_raw.as<op_conv_string>()->expr();
  }

  VertexAdaptor<op_string> format_string;

  switch (format_arg_raw->type()) {
    case op_var: {
      const auto format_var = format_arg_raw.as<op_var>()->var_id;
      const auto format_string_val = format_var->init_val.try_as<op_string>();
      if (!format_string_val) {
        return call;
      }
      format_string = format_string_val;
      break;
    }
    default:
      return call;
  }

  const auto parsed = FormatString::parse_format(format_string->str_val);
  if (!parsed.ok()) {
    for (const auto &error : parsed.errors) {
      kphp_error(0, error);
    }
    return call;
  }

  FormatCallInfo info;

  const auto format_args_raw = args[1 + arg_shift];

  switch (format_args_raw->type()) {
    // if all arguments are constant
    case op_var: {
      info.var = format_args_raw.as<op_var>();
      info.array = info.var->var_id->init_val.try_as<op_array>();
      if (!info.array) {
        return call;
      }
      info.args = info.array->args();
      info.is_var = true;
      break;
    }
    // if at least one argument calls a function or variable
    case op_array: {
      info.array = format_args_raw.as<op_array>();
      info.args = info.array->args();
      break;
    }
    default:
      return call;
  }

  check_printf_like_format(parsed, info.args);
  if (stage::has_error()) {
    return call;
  }

  // if format is ""
  if (parsed.parts.empty()) {
    return VertexAdaptor<op_string>::create();
  }

  std::vector<VertexPtr> vertex_parts;
  size_t index_spec_without_num = 0;

  for (const auto &part : parsed.parts) {
    if (part.is_specifier()) {
      if (part.specifier.arg_num == -1) {
        index_spec_without_num++;
      }

      size_t arg_index = 0;

      if (part.specifier.arg_num != -1) {
        arg_index = part.specifier.arg_num - 1;
      } else {
        arg_index = index_spec_without_num - 1;
      }

      const auto vertex = convert_format_part_to_vertex(part, arg_index, info);
      vertex_parts.push_back(vertex);
      continue;
    }

    const auto vertex = convert_format_part_to_vertex(part, 0, info);
    vertex_parts.push_back(vertex);
  }

  const auto build_vertex = VertexAdaptor<op_string_build>::create(vertex_parts);

  // turns into a call to printf/fprintf("%s", build_vertex)
  // this is necessary because the return value might be used
  if (need_output) {
    FunctionPtr print_func;
    if (to_file) {
      print_func = G->get_function("fprintf");
    } else {
      print_func = G->get_function("printf");
    }

    auto format_arg = VertexAdaptor<op_string>::create();
    format_arg->set_string("%s");
    const auto array_vertex = VertexAdaptor<op_array>::create(build_vertex);
    auto print_call = VertexAdaptor<op_func_call>::create(std::vector<VertexPtr>{format_arg, array_vertex});
    print_call->set_string(std::string(print_func->local_name()));
    print_call->func_id = print_func;
    return print_call;
  }

  return build_vertex;
}

VertexPtr OptimizationPass::convert_format_part_to_vertex(const FormatString::Part &part, size_t arg_index, const FormatCallInfo &info) {
  if (part.is_specifier()) {
    if (part.specifier.is_simple()) {
      return convert_simple_spec_to_vertex(part.specifier, arg_index, info);
    }
    return convert_complex_spec_to_vertex(part.specifier, arg_index, info);
  }

  VertexAdaptor<op_string> vertex = VertexAdaptor<op_string>::create();
  vertex->set_string(part.value);
  return vertex;
}

// turns into a strval(strval/intval/floatval(argument))
VertexPtr OptimizationPass::convert_simple_spec_to_vertex(const FormatString::Specifier &spec, size_t arg_index, const FormatCallInfo &info) {
  VertexPtr element;

  if (info.is_var) {
    // building $arr[$index]
    auto index_vertex = VertexAdaptor<op_int_const>::create();
    index_vertex->set_string(std::to_string(arg_index));
    element = VertexAdaptor<op_index>::create(info.var, index_vertex);
  } else {
    element = info.args[arg_index];
  }

  VertexPtr convert;

  switch (spec.type) {
    case FormatString::SpecifierType::Decimal:
      convert = VertexAdaptor<op_conv_int>::create(element);
      break;
    case FormatString::SpecifierType::Float:
      convert = VertexAdaptor<op_conv_float>::create(element);
      break;
    case FormatString::SpecifierType::String:
      convert = VertexAdaptor<op_conv_string>::create(element);
      break;
    default:
      return element;
  }

  return VertexAdaptor<op_conv_string>::create(convert);
}

// turns into a call to sprintf("specifier", argument)
VertexPtr OptimizationPass::convert_complex_spec_to_vertex(const FormatString::Specifier &spec, size_t arg_index, const FormatCallInfo &info) {
  VertexPtr element;

  if (info.is_var) {
    // building $arr[$index]
    auto index_vertex = VertexAdaptor<op_int_const>::create();
    index_vertex->set_string(std::to_string(arg_index));
    element = VertexAdaptor<op_index>::create(info.var, index_vertex);
  } else {
    element = info.args[arg_index];
  }

  const auto sprintf_func = G->get_function("sprintf");

  auto new_spec = spec;
  new_spec.arg_num = -1; // argument number no longer makes sense

  const auto spec_string = new_spec.to_string();
  auto format_vertex = VertexAdaptor<op_string>::create();
  format_vertex->set_string(spec_string);

  const auto args = std::vector<VertexPtr>{
    format_vertex,
    VertexAdaptor<op_array>::create(element),
  };

  auto call_function = VertexAdaptor<op_func_call>::create(args);
  call_function->set_string(std::string{sprintf_func->local_name()});
  call_function->func_id = sprintf_func;

  return call_function;
}

void OptimizationPass::check_printf_like_format(const FormatString::ParseResult &parsed, const VertexRange &args) {
  for (int i = 0; i < args.size(); ++i) {
    const auto *arg_type = tinf::get_type(args[i]);
    const auto expected_specs = parsed.get_by_index(i + 1);
    for (const auto &spec : expected_specs) {
      kphp_error(spec.specifier.allowed_for(arg_type),
                 fmt_format("For format specifier '{}', type '{}' is expected, but {} argument has type '{}'",
                            TermStringFormat::paint_green(spec.to_string()),
                            TermStringFormat::paint_green(spec.specifier.expected_type()),
                            i + 1, arg_type->as_human_readable()));
    }
  }

  const auto expected_args = parsed.min_count();
  kphp_error(args.size() >= expected_args,
             fmt_format("Not enough parameters for format string '{}', expected number of arguments: {}, passed {}",
                        TermStringFormat::paint_green(parsed.format), TermStringFormat::paint_green(std::to_string(expected_args)),
                        TermStringFormat::paint_green(std::to_string(args.size()))));
}

bool OptimizationPass::user_recursion(VertexPtr root) {
  if (auto var_vertex = root.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    kphp_assert (var);
    if (var->init_val) {
      if (try_optimize_var(var)) {
        ++var_init_expression_optimization_depth_;
        run_function_pass(var->init_val, this);
        kphp_assert(var_init_expression_optimization_depth_ > 0);
        --var_init_expression_optimization_depth_;
        if (!var->is_constant()) {
          explicit_cast_array_type(var->init_val, tinf::get_type(var));
        }
      }
    }
  }
  return false;
}

bool OptimizationPass::check_function(FunctionPtr function) const {
  return !function->is_extern();
}

void OptimizationPass::on_finish() {
  if (current_function->type == FunctionData::func_class_holder) {
    auto class_id = current_function->class_id;
    class_id->members.for_each([](ClassMemberInstanceField &class_field) {
      if (class_field.var->init_val) {
        if (can_init_value_be_removed(class_field.var->init_val, class_field.var)) {
          class_field.var->init_val = {};
        } else {
          explicit_cast_array_type(class_field.var->init_val, tinf::get_type(class_field.var));
        }
      }
    });
  }
}
