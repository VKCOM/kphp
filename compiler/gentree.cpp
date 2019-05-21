#include "compiler/gentree.h"

#include <sstream>

#include "common/algorithms/find.h"
#include "common/type_traits/constexpr_if.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/lambda-generator.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/debug.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

#define CE(x) if (!(x)) {return {};}

namespace ClassMemberModifier {
enum {
  cm_static = 1 << 0,
  cm_public = 1 << 1,
  cm_private = 1 << 2,
  cm_protected = 1 << 3,
  cm_final = 1 << 4,
  cm_abstract = 1 << 5,
};
} // namespace ClassMemberModifier

GenTree::GenTree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os) :
  line_num(0),
  tokens(std::move(tokens)),
  parsed_os(os),
  is_top_of_the_function_(false),
  cur(this->tokens.begin()),
  end(this->tokens.end()),
  is_in_parse_func_call(false),
  processing_file(file) {                  // = stage::get_file()

  kphp_assert (cur != end);
  end--;
  kphp_assert (end->type() == tok_end);

  line_num = cur->line_num;
  stage::set_line(line_num);
}

VertexPtr GenTree::generate_constant_field_class_value() {
  auto value_of_const_field_class = VertexAdaptor<op_string>::create();
  value_of_const_field_class->set_string(cur_class->name);
  return value_of_const_field_class;
}

void GenTree::next_cur() {
  if (cur != end) {
    cur++;
    if (cur->line_num != -1) {
      line_num = cur->line_num;
      stage::set_line(line_num);
    }
  }
}

bool GenTree::test_expect(TokenType tp) {
  return cur->type() == tp;
}

#define expect(tp, msg) ({ \
  bool res__;\
  if (kphp_error (test_expect (tp), format ("Expected %s, found '%s'", msg, cur == end ? "END OF FILE" : cur->to_str().c_str()))) {\
    res__ = false;\
  } else {\
    next_cur();\
    res__ = true;\
  }\
  res__; \
})

#define expect2(tp1, tp2, msg) ({ \
  kphp_error (test_expect (tp1) || test_expect (tp2), format ("Expected %s, found '%s'", msg, cur == end ? "END OF FILE" : cur->to_str().c_str())); \
  if (cur != end) {next_cur();} \
  1;\
})

VertexAdaptor<op_var> GenTree::get_var_name() {
  AutoLocation var_location(this);

  if (cur->type() != tok_var_name) {
    return {};
  }
  auto var = VertexAdaptor<op_var>::create();
  var->str_val = static_cast<string>(cur->str_val);

  set_location(var, var_location);

  next_cur();
  return var;
}

VertexAdaptor<op_var> GenTree::get_var_name_ref() {
  bool ref_flag = false;
  if (cur->type() == tok_and) {
    next_cur();
    ref_flag = true;
  }

  auto name = get_var_name();
  if (name) {
    name->ref_flag = ref_flag;
  } else {
    kphp_error(!ref_flag, "Expected var name");
  }
  return name;
}

int GenTree::open_parent() {
  if (cur->type() == tok_oppar) {
    next_cur();
    return 1;
  }
  return 0;
}

inline void GenTree::skip_phpdoc_tokens() {
  while (cur->type() == tok_phpdoc) {
    next_cur();
  }
}

#define close_parent(is_opened)\
  if (is_opened) {\
    CE (expect (tok_clpar, "')'"));\
  }

template<Operation EmptyOp, class FuncT, class ResultType>
bool GenTree::gen_list(std::vector<ResultType> *res, FuncT f, TokenType delim) {
  //Do not clear res. Result must be appended to it.
  bool prev_delim = false;
  bool next_delim = true;

  while (next_delim) {
    ResultType v = (this->*f)();
    next_delim = cur->type() == delim;

    if (!v) {
      if (EmptyOp != op_err && (prev_delim || next_delim)) {
        if (EmptyOp == op_none) {
          break;
        }

        v = vk::constexpr_if(std::integral_constant<bool, EmptyOp == op_none || EmptyOp == op_err>{},
                             [&v] { return v; },
                             [] { return VertexAdaptor<EmptyOp>::create(); });
      } else if (prev_delim) {
        kphp_error(0, "Expected something after ','");
        return false;
      } else {
        break;
      }
    }

    res->push_back(v);
    prev_delim = true;

    if (next_delim) {
      next_cur();
    }
  }

  return true;
}

template<Operation Op>
VertexPtr GenTree::get_conv() {
  AutoLocation conv_location(this);
  next_cur();
  VertexPtr first_node = get_expression();
  CE (!kphp_error(first_node, "get_conv failed"));
  auto conv = VertexAdaptor<Op>::create(first_node);
  set_location(conv, conv_location);
  return conv;
}

VertexPtr GenTree::get_require(bool once) {
  AutoLocation require_location(this);
  next_cur();
  const bool is_opened = open_parent();
  auto require = VertexAdaptor<op_require>::create(GenTree::get_expression());
  require->once = once;
  close_parent (is_opened);
  set_location(require, require_location);
  return require;
}


template<Operation Op, Operation EmptyOp>
VertexPtr GenTree::get_func_call() {
  AutoLocation call_location(this);
  string name{cur->str_val};
  next_cur();

  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  vector<VertexPtr> next;
  bool ok_next = false;
  {
    StackPushPop<bool> stack_parse_func(in_parse_func_call_stack, is_in_parse_func_call, true);
    ok_next = gen_list<EmptyOp>(&next, &GenTree::get_expression, tok_comma);
  }
  CE (!kphp_error(ok_next, "get argument list failed"));
  CE (expect(tok_clpar, "')'"));

  if (Op == op_isset) {
    CE (!kphp_error(!next.empty(), "isset function requires at least one argument"));
    VertexPtr left = VertexAdaptor<op_isset>::create(next[0]);
    for (size_t i = 1; i < next.size(); i++) {
      auto right = VertexAdaptor<op_isset>::create(next[i]);
      auto log_and = VertexAdaptor<op_log_and>::create(left, right);
      left = log_and;
    }
    set_location(left, call_location);
    return left;
  }

  VertexPtr call = VertexAdaptor<Op>::create(next);
  set_location(call, call_location);

  //hack..
  if (Op == op_func_call) {
    auto func_call = call.as<op_func_call>();
    func_call->set_string(name);
  }
  if (Op == op_constructor_call) {
    auto func_call = call.as<op_constructor_call>();
    func_call->set_string(name);

    // Hack to be more compatible with php
    if (name == "Memcache") {
      func_call->set_string("McMemcache");
    }
  }
  return call;
}

VertexPtr GenTree::get_short_array() {
  AutoLocation call_location(this);
  next_cur();

  vector<VertexPtr> next;
  bool ok_next = gen_list<op_none>(&next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_next, "get short array failed"));
  CE (expect(tok_clbrk, "']'"));

  auto arr = VertexAdaptor<op_array>::create(next);
  set_location(arr, call_location);

  return arr;
}


VertexPtr GenTree::get_string() {
  auto str = VertexAdaptor<op_string>::create();
  set_location(str, AutoLocation(this));
  str->str_val = static_cast<string>(cur->str_val);
  next_cur();
  return str;
}

VertexPtr GenTree::get_string_build() {
  AutoLocation sb_location(this);
  vector<VertexPtr> v_next;
  next_cur();
  bool after_simple_expression = false;
  while (cur != end && cur->type() != tok_str_end) {
    if (cur->type() == tok_str) {
      v_next.push_back(get_string());
      if (after_simple_expression) {
        VertexAdaptor<op_string> last = v_next.back().as<op_string>();
        if (!last->str_val.empty() && last->str_val[0] == '[') {
          kphp_warning("Simple string expressions with [] can work wrong. Use more {}");
        }
      }
      after_simple_expression = false;
    } else if (cur->type() == tok_expr_begin) {
      next_cur();

      VertexPtr add = get_expression();
      CE (!kphp_error(add, "Bad expression in string"));
      v_next.push_back(add);

      CE (expect(tok_expr_end, "'}'"));
      after_simple_expression = false;
    } else {
      after_simple_expression = true;
      VertexPtr add = get_expression();
      CE (!kphp_error(add, "Bad expression in string"));
      v_next.push_back(add);
    }
  }
  CE (expect(tok_str_end, "'\"'"));
  auto sb = VertexAdaptor<op_string_build>::create(v_next);
  set_location(sb, sb_location);
  return sb;
}

VertexPtr GenTree::get_postfix_expression(VertexPtr res) {
  //postfix operators ++, --, [], ->
  bool need = true;
  while (need && cur != end) {
    auto op = cur;
    TokenType tp = op->type();
    need = false;

    if (tp == tok_inc) {
      auto v = VertexAdaptor<op_postfix_inc>::create(res);
      set_location(v, AutoLocation(this));
      res = v;
      need = true;
      next_cur();
    } else if (tp == tok_dec) {
      auto v = VertexAdaptor<op_postfix_dec>::create(res);
      set_location(v, AutoLocation(this));
      res = v;
      need = true;
      next_cur();
    } else if (tp == tok_opbrk || tp == tok_opbrc) {
      AutoLocation location(this);
      next_cur();
      VertexPtr i = get_expression();
      if (tp == tok_opbrk) {
        CE (expect(tok_clbrk, "']'"));
      } else {
        CE (expect(tok_clbrc, "'}'"));
      }
      //TODO: it should be to separate operations
      if (!i) {
        auto v = VertexAdaptor<op_index>::create(res);
        res = v;
      } else {
        auto v = VertexAdaptor<op_index>::create(res, i);
        res = v;
      }
      set_location(res, location);
      need = true;
    } else if (tp == tok_arrow) {
      AutoLocation location(this);
      next_cur();
      VertexPtr i = get_expr_top(true);
      CE (!kphp_error(i, "Failed to parse right argument of '->'"));
      auto v = VertexAdaptor<op_arrow>::create(res, i);
      res = v;
      set_location(res, location);
      need = true;
    } else if (tp == tok_oppar) {
      AutoLocation location(this);
      next_cur();
      skip_phpdoc_tokens();
      vector<VertexPtr> next;
      bool ok_next = gen_list<op_err>(&next, &GenTree::get_expression, tok_comma);
      CE (!kphp_error(ok_next, "get argument list failed"));
      CE (expect(tok_clpar, "')'"));

      auto call = VertexAdaptor<op_func_call>::create(next);
      set_location(call, location);

      call->set_string("__invoke");

      res = VertexAdaptor<op_arrow>::create(res, call);
      set_location(res, location);
      need = true;
    }


  }
  return res;
}

VertexPtr GenTree::get_expr_top(bool was_arrow) {
  auto op = cur;

  VertexPtr res, first_node;
  TokenType type = op->type();

  bool return_flag = true;
  switch (type) {
    case tok_line_c: {
      auto v = VertexAdaptor<op_int_const>::create();
      set_location(v, AutoLocation(this));
      v->str_val = int_to_str(stage::get_line());
      res = v;
      next_cur();
      break;
    }
    case tok_file_c: {
      auto v = VertexAdaptor<op_string>::create();
      set_location(v, AutoLocation(this));
      v->str_val = processing_file->file_name;
      next_cur();
      res = v;
      break;
    }
    case tok_class_c: {
      auto v = VertexAdaptor<op_string>::create();
      set_location(v, AutoLocation(this));
      v->str_val = cur_class ? cur_class->name : "";
      next_cur();
      res = v;
      break;
    }
    case tok_dir_c: {
      auto v = VertexAdaptor<op_string>::create();
      set_location(v, AutoLocation(this));
      v->str_val = processing_file->file_name.substr(0, processing_file->file_name.rfind('/'));
      next_cur();
      res = v;
      break;
    }
    case tok_method_c: {
      auto v = VertexAdaptor<op_string>::create();
      set_location(v, AutoLocation(this));
      next_cur();
      if (is_anonymous_function_name(cur_function->name)) {
        v->str_val = "{closure}";
      } else if (cur_function->name != processing_file->main_func_name) {
        if (cur_class) {
          v->str_val = cur_class->name + "::" + cur_function->name.substr(cur_function->name.rfind('$') + 1);
        } else {
          v->str_val = cur_function->name;
        }
      }
      res = v;
      break;
    }
    case tok_namespace_c: {
      auto v = VertexAdaptor<op_string>::create();
      set_location(v, AutoLocation(this));
      v->str_val = processing_file->namespace_name;
      next_cur();
      res = v;
      break;
    }
    case tok_func_c: {
      auto v = VertexAdaptor<op_string>::create();
      set_location(v, AutoLocation(this));
      if (is_anonymous_function_name(cur_function->name)) {
        v->str_val = "{closure}";
      } else if (cur_function->name != processing_file->main_func_name) {
        v->str_val = cur_function->name.substr(cur_function->name.rfind('$') + 1);
      }
      next_cur();
      res = v;
      break;
    }
    case tok_int_const: {
      auto v = VertexAdaptor<op_int_const>::create();
      set_location(v, AutoLocation(this));
      v->str_val = static_cast<string>(cur->str_val);
      next_cur();
      res = v;
      break;
    }
    case tok_float_const: {
      auto v = VertexAdaptor<op_float_const>::create();
      set_location(v, AutoLocation(this));
      v->str_val = static_cast<string>(cur->str_val);
      next_cur();
      res = v;
      break;
    }
    case tok_null: {
      auto v = VertexAdaptor<op_null>::create();
      set_location(v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_false: {
      auto v = VertexAdaptor<op_false>::create();
      set_location(v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_true: {
      auto v = VertexAdaptor<op_true>::create();
      set_location(v, AutoLocation(this));
      next_cur();
      res = v;
      break;
    }
    case tok_var_name: {
      res = get_var_name();
      return_flag = false;
      break;
    }
    case tok_varg: {
      bool good_prefix = cur != tokens.begin() && vk::any_of_equal(std::prev(cur)->type(), tok_comma, tok_oppar);
      CE (!kphp_error(good_prefix, "It's not allowed using `...` in this place"));

      next_cur();
      res = get_expression();
      CE (!kphp_error(res && vk::any_of_equal(res->type(), op_var, op_array, op_func_call, op_arrow, op_index, op_conv_array) ,
        format("It's not allowed using `...` in this place (op: %s)", OpInfo::str(res->type()).c_str())));
      res = VertexAdaptor<op_varg>::create(res).set_location(res);
      break;
    }
    case tok_str:
      res = get_string();
      break;
    case tok_conv_int:
      res = get_conv<op_conv_int>();
      break;
    case tok_conv_bool:
      res = get_conv<op_conv_bool>();
      break;
    case tok_conv_float:
      res = get_conv<op_conv_float>();
      break;
    case tok_conv_string:
      res = get_conv<op_conv_string>();
      break;
    case tok_conv_long:
      res = get_conv<op_conv_long>();
      break;
    case tok_conv_uint:
      res = get_conv<op_conv_uint>();
      break;
    case tok_conv_ulong:
      res = get_conv<op_conv_ulong>();
      break;
    case tok_conv_array:
      res = get_conv<op_conv_array>();
      break;

    case tok_print: {
      AutoLocation print_location(this);
      next_cur();
      first_node = get_expression();
      CE (!kphp_error(first_node, "Failed to get print argument"));
      first_node = conv_to<tp_string>(first_node);
      auto print = VertexAdaptor<op_print>::create(first_node);
      set_location(print, print_location);
      res = print;
      break;
    }

    case tok_exit:
      res = get_exit();
      break;
    case tok_require:
      res = get_require(false);
      break;
    case tok_require_once:
      res = get_require(true);
      break;

    case tok_new:
      next_cur();
      CE(!kphp_error(cur->type() == tok_func_name, "Expected class name after new"));
      res = get_func_call<op_constructor_call, op_none>();
      break;
    case tok_func_name: {
      cur++;
      if (!test_expect(tok_oppar)) {
        cur--;
        auto v = VertexAdaptor<op_func_name>::create();
        set_location(v, AutoLocation(this));
        next_cur();
        v->str_val = static_cast<string>(op->str_val);
        res = v;
        return_flag = was_arrow;
        break;
      }
      cur--;
      res = get_func_call<op_func_call, op_err>();
      return_flag = was_arrow;
      break;
    }
    case tok_static:
      next_cur();
      res = get_anonymous_function(true);
      break;
    case tok_function: {
      res = get_anonymous_function();
      break;
    }
    case tok_isset:
      res = get_func_call<op_isset, op_err>();
      break;
    case tok_array:
      res = get_func_call<op_array, op_none>();
      break;
    case tok_tuple:
      res = get_func_call<op_tuple, op_err>();
      CE (!kphp_error(res.as<op_tuple>()->size(), "tuple() must have at least one argument"));
      break;
    case tok_opbrk:
      res = get_short_array();
      break;
    case tok_list:
      res = get_func_call<op_list_ce, op_lvalue_null>();
      break;
    case tok_defined:
      res = get_func_call<op_defined, op_err>();
      break;
    case tok_min: {
      auto min_v = get_func_call<op_min, op_err>().as<op_min>();
      VertexRange args = min_v->args();
      if (args.size() == 1) {
        args[0] = GenTree::conv_to(args[0], tp_array);
      }
      res = min_v;
      break;
    }
    case tok_max: {
      auto max_v = get_func_call<op_max, op_err>().as<op_max>();
      VertexRange args = max_v->args();
      if (args.size() == 1) {
        args[0] = GenTree::conv_to(args[0], tp_array);
      }
      res = max_v;
      break;
    }

    case tok_oppar:
      next_cur();
      res = get_expression();
      CE (!kphp_error(res, "Failed to parse expression after '('"));
      CE (expect(tok_clpar, "')'"));
      return_flag = cur->type() != tok_arrow;
      break;
    case tok_str_begin:
      res = get_string_build();
      break;
    case tok_clone: {
      next_cur();
      res = VertexAdaptor<op_clone>::create(get_expr_top(false));
      break;
    }
    default:
      return {};
  }

  if (return_flag) {
    return res;
  }

  res = get_postfix_expression(res);
  return res;
}

VertexPtr GenTree::create_ternary_op_vertex(VertexPtr left, VertexPtr right, VertexPtr third) {
  if (right) {
    return VertexAdaptor<op_ternary>::create(left, right, third);
  }

  string left_name = gen_shorthand_ternary_name(cur_function);
  auto left_var = VertexAdaptor<op_var>::create();
  left_var->str_val = left_name;
  left_var->extra_type = op_ex_var_superlocal;

  auto left_set = VertexAdaptor<op_set>::create(left_var, left);
  auto left_set_bool = conv_to<tp_bool>(left_set);

  auto left_var_copy = VertexAdaptor<op_var>::create();
  left_var_copy->str_val = left_name;
  left_var_copy->extra_type = op_ex_var_superlocal;

  auto left_var_move = VertexAdaptor<op_move>::create(left_var_copy);
  return VertexAdaptor<op_ternary>::create(left_set_bool, left_var_move, third);
}

VertexAdaptor<op_class_type_rule> GenTree::create_type_help_class_vertex(vk::string_view klass_name) {
  return create_type_help_class_vertex(G->get_class(resolve_uses(cur_function, static_cast<std::string>(klass_name))));
}

VertexAdaptor<op_class_type_rule> GenTree::create_type_help_class_vertex(ClassPtr klass) {
  auto type_rule = VertexAdaptor<op_class_type_rule>::create();
  type_rule->type_help = tp_Class;
  type_rule->class_ptr = klass;
  return type_rule;
}

VertexPtr GenTree::get_unary_op(int op_priority_cur, Operation unary_op_tp, bool till_ternary) {
  AutoLocation expr_location(this);
  next_cur();

  VertexPtr left = get_binary_op(op_priority_cur, till_ternary);
  if (!left) {
    return VertexPtr();
  }

  if (unary_op_tp == op_log_not) {
    left = conv_to<tp_bool>(left);
  }
  if (unary_op_tp == op_not) {
    left = conv_to<tp_int>(left);
  }
  VertexPtr expr = create_vertex(unary_op_tp, left);
  set_location(expr, expr_location);

  if (expr->type() == op_minus || expr->type() == op_plus) {
    VertexPtr maybe_num = expr.as<meta_op_unary>()->expr();
    if (auto num = maybe_num.try_as<meta_op_num>()) {
      // +N оставляем как "N", а -N делаем константу "-N" (но если N начинается с минуса, то "N", чтобы парсить `- - -7`)
      if (expr->type() == op_minus) {
        num->str_val = num->str_val[0] == '-' ? num->str_val.substr(1) : "-" + num->str_val;
      }
      return num;
    }
  }

  return expr;
}

VertexPtr GenTree::get_binary_op(int op_priority_cur, bool till_ternary) {
  if (op_priority_cur >= OpInfo::op_priority_end) {
    return get_expr_top(false);
  }

  if (cur != end) {
    const Operation unary_op_tp = OpInfo::tok_to_unary_op[cur->type()];
    if (unary_op_tp != op_err && OpInfo::priority(unary_op_tp) <= op_priority_cur) {
      return get_unary_op(op_priority_cur, unary_op_tp, till_ternary);
    }
  }

  const bool ternary = op_priority_cur == OpInfo::ternaryP;
  VertexPtr left = get_binary_op(op_priority_cur + 1, till_ternary);
  if (!left || (ternary && till_ternary)) {
    return left;
  }

  while (cur != end) {
    const Operation binary_op_tp = OpInfo::tok_to_binary_op[cur->type()];
    if (binary_op_tp == op_err || OpInfo::priority(binary_op_tp) != op_priority_cur) {
      break;
    }

    AutoLocation expr_location(this);
    const bool left_to_right = OpInfo::fixity(binary_op_tp) == left_opp;

    next_cur();
    VertexPtr right = ternary
                      ? get_expression()
                      : get_binary_op(op_priority_cur + left_to_right,
                                      till_ternary && op_priority_cur >= OpInfo::ternaryP);
    if (!right && !ternary) {
      kphp_error (0, format("Failed to parse second argument in [%s]", OpInfo::str(binary_op_tp).c_str()));
      return {};
    }

    VertexPtr third;
    if (ternary) {
      CE (expect(tok_colon, "':'"));
      third = get_expression_impl(true);
      if (!third) {
        kphp_error (0, format("Failed to parse third argument in [%s]", OpInfo::str(binary_op_tp).c_str()));
        return {};
      }
      if (right) {
        left = conv_to<tp_bool>(left);
      }
    }

    if (vk::any_of_equal(binary_op_tp, op_log_or, op_log_and, op_log_or_let, op_log_and_let, op_log_xor_let)) {
      left = conv_to<tp_bool>(left);
      right = conv_to<tp_bool>(right);
    }
    if (vk::any_of_equal(binary_op_tp, op_set_or, op_set_and, op_set_xor, op_set_shl, op_set_shr)) {
      right = conv_to<tp_int>(right);
    }
    if (vk::any_of_equal(binary_op_tp, op_or, op_and, op_xor)) {
      left = conv_to<tp_int>(left);
      right = conv_to<tp_int>(right);
    }

    left = ternary
           ? create_ternary_op_vertex(left, right, third)
           : create_vertex(binary_op_tp, left, right);

    set_location(left, expr_location);
    if (!(left_to_right || ternary)) {
      break;
    }
  }
  return left;
}

VertexPtr GenTree::get_expression_impl(bool till_ternary) {
  return get_binary_op(OpInfo::op_priority_begin, till_ternary);
}

VertexPtr GenTree::get_expression() {
  skip_phpdoc_tokens();
  return get_expression_impl(false);
}

VertexPtr GenTree::embrace(VertexPtr v) {
  if (v->type() != op_seq) {
    auto brace = VertexAdaptor<op_seq>::create(v);
    ::set_location(brace, v->get_location());
    return brace;
  }

  return v;
}

VertexPtr GenTree::get_def_value() {
  VertexPtr val;

  if (cur->type() == tok_eq1) {
    next_cur();
    val = get_expression();
    kphp_error (val, "Cannot parse function parameter");
  }

  return val;
}

VertexAdaptor<op_func_param> GenTree::get_func_param_without_callbacks(bool from_callback) {
  AutoLocation st_location(this);
  const Token *tok_type_declaration = nullptr;
  if (cur->type() == tok_func_name) {
    tok_type_declaration = &*cur;
    next_cur();
  }

  VertexAdaptor<op_var> name = get_var_name_ref();
  if (!name) {
    return {};
  }

  vector<VertexPtr> next{name};

  PrimitiveType tp = tp_Unknown;
  VertexPtr type_rule;
  if (!from_callback && cur->type() == tok_triple_colon) {
    tp = get_func_param_type_help();    // запишется в param->type_help, и при вызове будет неявный cast
  } else {
    type_rule = get_type_rule();
  }

  VertexPtr def_val = get_def_value();
  if (def_val) {
    next.push_back(def_val);
  }
  auto v = VertexAdaptor<op_func_param>::create(next);
  set_location(v, st_location);
  if (tok_type_declaration != nullptr) {
    v->type_declaration = static_cast<string>(tok_type_declaration->str_val);
    v->type_help = tp_Class;
  }

  if (type_rule) {
    v->type_rule = type_rule;
  } else if (tp != tp_Unknown) {
    v->type_help = tp;
  }

  return v;
}

VertexAdaptor<op_func_param> GenTree::get_func_param_from_callback() {
  return get_func_param_without_callbacks(true);
}

VertexAdaptor<meta_op_func_param> GenTree::get_func_param() {
  AutoLocation st_location(this);
  if (test_expect(tok_func_name) && ((cur + 1)->type() == tok_oppar)) { // callback
    auto name = VertexAdaptor<op_func_name>::create();
    set_location(name, st_location);
    name->str_val = static_cast<string>(cur->str_val);
    kphp_assert(name->str_val == "callback");
    next_cur();

    CE (expect(tok_oppar, "'('"));
    std::vector<VertexAdaptor<op_func_param>> callback_params;
    bool ok_params_next = gen_list<op_err>(&callback_params, &GenTree::get_func_param_from_callback, tok_comma);
    CE (!kphp_error(ok_params_next, "Failed to parse callback params"));
    auto params = VertexAdaptor<op_func_param_list>::create(callback_params);
    set_location(params, st_location);
    CE (expect(tok_clpar, "')'"));

    VertexPtr type_rule = get_type_rule();

    VertexPtr def_val = get_def_value();
    kphp_assert(!def_val || (def_val->type() == op_func_name && def_val->get_string() == "TODO"));

    VertexAdaptor<op_func_param_callback> v;
    if (def_val) {
      v = VertexAdaptor<op_func_param_callback>::create(name, params, def_val);
    } else {
      v = VertexAdaptor<op_func_param_callback>::create(name, params);
    }

    v->type_rule = type_rule;
    set_location(v, st_location);

    return v;
  } else if (test_expect(tok_varg)) {   // variadic param (can not be inside callback, as it modifies cur_function)
    next_cur();
    if (auto name_var_arg = get_var_name()) {
      kphp_error(!cur_function->has_variadic_param, "Function can not have ...$variadic more than once");
      cur_function->has_variadic_param = true;
      return VertexAdaptor<op_func_param>::create(name_var_arg).set_location(name_var_arg);
    }
  }

  return get_func_param_without_callbacks();
}

VertexPtr GenTree::get_foreach_param() {
  AutoLocation location(this);
  VertexPtr xs = get_expression();
  CE (!kphp_error(xs, ""));

  CE (expect(tok_as, "'as'"));
  skip_phpdoc_tokens();

  VertexAdaptor<op_var> key;
  VertexAdaptor<op_var> x = get_var_name_ref();

  CE (!kphp_error(x, ""));
  if (cur->type() == tok_double_arrow) {
    next_cur();
    key = x;
    x = get_var_name_ref();
    CE (!kphp_error(x, ""));
  }

  vector<VertexPtr> next{xs, x};
  auto empty = VertexAdaptor<op_empty>::create();
  next.push_back(empty); // will be replaced
  if (key) {
    next.push_back(key);
  }
  auto res = VertexAdaptor<op_foreach_param>::create(next);
  set_location(res, location);
  return res;
}

VertexPtr GenTree::conv_to(VertexPtr x, PrimitiveType tp, bool ref_flag) {
  if (ref_flag) {
    switch (tp) {
      case tp_array:
        return conv_to_lval<tp_array>(x);
      case tp_int:
        return conv_to_lval<tp_int>(x);
      case tp_var:
        return x;
        break;
      default:
        kphp_error (0, "convert_to not array with ref_flag");
        return x;
    }
  }
  switch (tp) {
    case tp_int:
      return conv_to<tp_int>(x);
    case tp_bool:
      return conv_to<tp_bool>(x);
    case tp_string:
      return conv_to<tp_string>(x);
    case tp_float:
      return conv_to<tp_float>(x);
    case tp_array:
      return conv_to<tp_array>(x);
    case tp_UInt:
      return conv_to<tp_UInt>(x);
    case tp_Long:
      return conv_to<tp_Long>(x);
    case tp_ULong:
      return conv_to<tp_ULong>(x);
    case tp_regexp:
      return conv_to<tp_regexp>(x);
    default:
      return x;
  }
}

VertexPtr GenTree::get_actual_value(VertexPtr v) {
  if (v->type() == op_var && v->extra_type == op_ex_var_const && v->get_var_id()) {
    return v->get_var_id()->init_val;
  }
  if (v->type() == op_define_val) {
    DefinePtr d = v.as<op_define_val>()->define_id;
    if (d->type() == DefineData::def_const) {
      return d->val;
    }
  }
  return v;
}

const std::string *GenTree::get_constexpr_string(VertexPtr v) {
  v = get_actual_value(v);
  if (auto conv_vertex = v.try_as<op_conv_string>()) {
    return get_constexpr_string(conv_vertex->expr());
  }
  if (auto str_vertex = v.try_as<op_string>()) {
    return &str_vertex->get_string();
  }
  return nullptr;
}

int GenTree::get_id_arg_ref(VertexAdaptor<op_arg_ref> arg, VertexPtr expr) {
  int id = -1;
  if (auto fun_call = expr.try_as<op_func_call>()) {
    id = arg->int_val - 1;
    if (auto func_id = fun_call->get_func_id()) {
      if (id < 0 || id >= func_id->get_params().size()) {
        id = -1;
      }
    }
  }
  return id;
}

VertexPtr GenTree::get_call_arg_ref(VertexAdaptor<op_arg_ref> arg, VertexPtr expr) {
  int arg_id = get_id_arg_ref(arg, expr);
  if (arg_id != -1) {
    auto call_args = expr.as<op_func_call>()->args();
    return arg_id < call_args.size() ? call_args[arg_id] : VertexPtr{};
  }
  return {};
}

PrimitiveType GenTree::get_ptype() {
  PrimitiveType tp;
  TokenType tok = cur->type();
  switch (tok) {
    case tok_int:
      tp = tp_int;
      break;
    case tok_string:
      tp = tp_string;
      break;
    case tok_float:
      tp = tp_float;
      break;
    case tok_array:
      tp = tp_array;
      break;
    case tok_bool:
      tp = tp_bool;
      break;
    case tok_var:
      tp = tp_var;
      break;
    case tok_tuple:
      tp = tp_tuple;
      break;
    case tok_func_name:
      tp = get_ptype_by_name(static_cast<string>(cur->str_val));
      break;
    default:
      tp = tp_Error;
      break;
  }
  if (tp != tp_Error) {
    next_cur();
  }
  return tp;
}

PrimitiveType GenTree::get_func_param_type_help() {
  kphp_assert(cur->type() == tok_triple_colon);

  next_cur();
  PrimitiveType res = get_ptype();
  kphp_error (res != tp_Error, "Cannot parse type");

  return res;
}

VertexPtr GenTree::get_type_rule_func() {
  AutoLocation rule_location(this);
  string name{cur->str_val};
  next_cur();
  CE (expect(tok_lt, "<"));
  vector<VertexPtr> next;
  bool ok_next = gen_list<op_err>(&next, &GenTree::get_type_rule_, tok_comma);
  CE (!kphp_error(ok_next, "Failed get_type_rule_func"));
  CE (expect(tok_gt, ">"));

  auto rule = VertexAdaptor<op_type_rule_func>::create(next);
  set_location(rule, rule_location);
  rule->str_val = name;
  return rule;
}

VertexPtr GenTree::get_type_rule_() {
  PrimitiveType tp = get_ptype();
  TokenType tok = cur->type();
  VertexPtr res = VertexPtr(nullptr);
  if (tp != tp_Error) {
    AutoLocation arr_location(this);

    vector<VertexPtr> next;
    if (tok == tok_lt) {        // array<...>, tuple<...,...>
      kphp_error(vk::any_of_equal(tp, tp_array, tp_tuple, tp_future, tp_future_queue), "Cannot parse type_rule");
      bool allow_comma_sep = tp == tp_tuple;
      do {
        next_cur();
        next.push_back(get_type_rule_());
        CE (!kphp_error(next.back(), "Cannot parse type_rule (1)"));
      } while (allow_comma_sep && test_expect(tok_comma));
      CE (expect(tok_gt, "'>'"));
    }

    auto arr = VertexAdaptor<op_type_rule>::create(next);
    arr->type_help = tp;
    set_location(arr, arr_location);
    res = arr;
  } else if (tok == tok_func_name) {
    if (vk::any_of_equal(cur->str_val, "lca", "OrFalse")) {
      res = get_type_rule_func();
    } else if (cur->str_val == "instance") {
      res = get_type_rule_func();
      kphp_error(res->size() == 1, format("Allowed only one arg_ref for 'instance<>', got %d", res->size()));
      kphp_error(res->back().try_as<op_arg_ref>(), "Allowed only arg_ref for 'instance<>'");
    } else if (cur->str_val == "self") {
      //TODO: why no next_cur here?
      auto self = VertexAdaptor<op_self>::create();
      res = self;
    } else if (cur->str_val == "CONST") {
      next_cur();
      res = get_type_rule_();
      if (res) {
        res->extra_type = op_ex_rule_const;
      }
    } else if (cur->str_val[0] == '\\' || ('A' <= cur->str_val[0] && cur->str_val[0] <= 'Z')) {
      auto rule = create_type_help_class_vertex(cur->str_val);
      kphp_error(rule->class_ptr, format("Unknown class %s in type rule", string(cur->str_val).c_str()));
      next_cur();
      res = rule;
    } else {
      kphp_error(0, format("Can't parse type_rule. Unknown string [%s]", string(cur->str_val).c_str()));
    }
  } else if (tok == tok_xor) {
    next_cur();
    if (kphp_error (test_expect(tok_int_const), "Int expected")) {
      return VertexPtr();
    }
    auto v = VertexAdaptor<op_arg_ref>::create();
    set_location(v, AutoLocation(this));
    v->int_val = std::atoi(std::string(cur->str_val).c_str());
    res = v;
    next_cur();
    while (test_expect(tok_opbrk)) {
      AutoLocation opbrk_location(this);
      next_cur();
      CE (expect(tok_clbrk, "]"));
      auto index = VertexAdaptor<op_index>::create(res);
      set_location(index, opbrk_location);
      res = index;
    }

    if (test_expect(tok_oppar)) {
      AutoLocation oppar_location(this);
      next_cur();
      CE (expect(tok_clpar, ")"));
      auto call = VertexAdaptor<op_type_rule_func>::create(res);
      call->set_string("callback_call");
      set_location(call, oppar_location);
      res = call;
    }
  }
  return res;
}

VertexPtr GenTree::get_type_rule() {
  VertexPtr res, first;

  TokenType tp = cur->type();
  if (vk::any_of_equal(tp, tok_triple_colon, tok_triple_eq, tok_triple_lt, tok_triple_gt)) {
    AutoLocation rule_location(this);
    next_cur();
    first = get_type_rule_();
    CE(!kphp_error(first, "Cannot parse type rule"));

    VertexPtr rule = create_vertex(OpInfo::tok_to_op[tp], first);
    set_location(rule, rule_location);
    res = rule;
  }
  return res;
}

void GenTree::func_force_return(VertexAdaptor<op_function> func, VertexPtr val) {
  VertexPtr cmd = func->cmd();
  assert (cmd->type() == op_seq);

  VertexAdaptor<op_return> return_node;
  if (val) {
    return_node = VertexAdaptor<op_return>::create(val);
  } else {
    return_node = VertexAdaptor<op_return>::create();
  }

  vector<VertexPtr> next = cmd->get_next();
  next.push_back(return_node);
  auto seq = VertexAdaptor<op_seq>::create(next);
  func->cmd() = seq;
}

template<Operation Op, class FuncT, class ResultType>
VertexAdaptor<op_seq> GenTree::get_multi_call(FuncT &&f) {
  TokenType type = cur->type();
  AutoLocation seq_location(this);
  next_cur();

  std::vector<ResultType> next;
  bool ok_next = gen_list<op_err>(&next, f, tok_comma);
  CE (!kphp_error(ok_next, "Failed get_multi_call"));

  std::vector<VertexAdaptor<Op>> new_next;
  new_next.reserve(next.size());

  for (VertexPtr next_elem : next) {
    if (type == tok_echo || type == tok_dbg_echo) {
      next_elem = conv_to<tp_string>(next_elem);
    }
    new_next.emplace_back(VertexAdaptor<Op>::create(next_elem).set_location(next_elem));
  }
  auto seq = VertexAdaptor<op_seq>::create(new_next);
  set_location(seq, seq_location);
  return seq;
}


VertexPtr GenTree::get_return() {
  AutoLocation ret_location(this);
  next_cur();
  skip_phpdoc_tokens();
  VertexPtr return_val = get_expression();
  VertexAdaptor<op_return> ret;
  if (!return_val) {
    ret = VertexAdaptor<op_return>::create();
  } else {
    if (cur_function->is_constructor()) {
      kphp_error(return_val->type() == op_var && return_val->get_string() == "this",
                 "Class constructor may return only $this or nothing.");
    }
    ret = VertexAdaptor<op_return>::create(return_val);
  }
  set_location(ret, ret_location);
  CE (expect(tok_semicolon, "';'"));
  return ret;
}

VertexPtr GenTree::get_exit() {
  AutoLocation exit_location(this);
  next_cur();
  bool is_opened = open_parent();
  VertexPtr exit_val;
  if (is_opened) {
    exit_val = get_expression();
    close_parent (is_opened);
  }
  if (!exit_val) {
    auto tmp = VertexAdaptor<op_int_const>::create();
    tmp->str_val = "0";
    exit_val = tmp;
  }
  auto v = VertexAdaptor<op_exit>::create(exit_val);
  set_location(v, exit_location);
  return v;
}

template<Operation Op>
VertexPtr GenTree::get_break_continue() {
  AutoLocation res_location(this);
  next_cur();
  VertexPtr first_node = get_expression();
  CE (expect(tok_semicolon, "';'"));

  if (!first_node) {
    auto one = VertexAdaptor<op_int_const>::create();
    one->str_val = "1";
    first_node = one;
  }

  auto res = VertexAdaptor<Op>::create(first_node);
  set_location(res, res_location);
  return res;
}

VertexPtr GenTree::get_foreach() {
  AutoLocation foreach_location(this);
  next_cur();

  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr first_node = get_foreach_param();
  CE (!kphp_error(first_node, "Failed to parse 'foreach' params"));

  CE (expect(tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error(second_node, "Failed to parse 'foreach' body"));

  auto temp_node = VertexAdaptor<op_empty>::create();

  auto foreach = VertexAdaptor<op_foreach>::create(first_node, embrace(second_node), temp_node);
  set_location(foreach, foreach_location);
  return foreach;
}

VertexPtr GenTree::get_while() {
  AutoLocation while_location(this);
  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr first_node = get_expression();
  CE (!kphp_error(first_node, "Failed to parse 'while' condition"));
  first_node = conv_to<tp_bool>(first_node);
  CE (expect(tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error(second_node, "Failed to parse 'while' body"));

  auto while_vertex = VertexAdaptor<op_while>::create(first_node, embrace(second_node));
  set_location(while_vertex, while_location);
  return while_vertex;
}

VertexPtr GenTree::get_if() {
  AutoLocation if_location(this);
  VertexPtr if_vertex;
  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr first_node = get_expression();
  CE (!kphp_error(first_node, "Failed to parse 'if' condition"));
  first_node = conv_to<tp_bool>(first_node);
  CE (expect(tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error(second_node, "Failed to parse 'if' body"));
  second_node = embrace(second_node);

  VertexPtr third_node = VertexPtr();
  if (cur->type() == tok_else) {
    next_cur();
    third_node = get_statement();
    CE (!kphp_error(third_node, "Failed to parse 'else' statement"));
  }

  if (third_node) {
    third_node = embrace(third_node);
    auto v = VertexAdaptor<op_if>::create(first_node, second_node, third_node);
    if_vertex = v;
  } else {
    auto v = VertexAdaptor<op_if>::create(first_node, second_node);
    if_vertex = v;
  }
  set_location(if_vertex, if_location);
  return if_vertex;
}

VertexPtr GenTree::get_for() {
  AutoLocation for_location(this);
  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();

  AutoLocation pre_cond_location(this);
  vector<VertexPtr> first_next;
  bool ok_first_next = gen_list<op_err>(&first_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_first_next, "Failed to parse 'for' precondition"));
  auto pre_cond = VertexAdaptor<op_seq>::create(first_next);
  set_location(pre_cond, pre_cond_location);

  CE (expect(tok_semicolon, "';'"));

  AutoLocation cond_location(this);
  vector<VertexPtr> second_next;
  bool ok_second_next = gen_list<op_err>(&second_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_second_next, "Failed to parse 'for' action"));
  if (second_next.empty()) {
    auto v_true = VertexAdaptor<op_true>::create();
    second_next.push_back(v_true);
  } else {
    second_next.back() = conv_to<tp_bool>(second_next.back());
  }
  auto cond = VertexAdaptor<op_seq_comma>::create(second_next);
  set_location(cond, cond_location);

  CE (expect(tok_semicolon, "';'"));

  AutoLocation post_cond_location(this);
  vector<VertexPtr> third_next;
  bool ok_third_next = gen_list<op_err>(&third_next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_third_next, "Failed to parse 'for' postcondition"));
  auto post_cond = VertexAdaptor<op_seq>::create(third_next);
  set_location(post_cond, post_cond_location);

  CE (expect(tok_clpar, "')'"));

  VertexPtr cmd = get_statement();
  CE (!kphp_error(cmd, "Failed to parse 'for' statement"));

  cmd = embrace(cmd);
  auto for_vertex = VertexAdaptor<op_for>::create(pre_cond, cond, post_cond, cmd);
  set_location(for_vertex, for_location);
  return for_vertex;
}

VertexPtr GenTree::get_do() {
  AutoLocation do_location(this);
  next_cur();
  VertexPtr first_node = get_statement();
  CE (!kphp_error(first_node, "Failed to parser 'do' condition"));

  CE (expect(tok_while, "'while'"));

  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr second_node = get_expression();
  CE (!kphp_error(second_node, "Failed to parse 'do' statement"));
  second_node = conv_to<tp_bool>(second_node);
  CE (expect(tok_clpar, "')'"));
  CE (expect(tok_semicolon, "';'"));

  auto do_vertex = VertexAdaptor<op_do>::create(second_node, first_node);
  set_location(do_vertex, do_location);
  return do_vertex;
}

VertexPtr GenTree::get_switch() {
  AutoLocation switch_location(this);
  vector<VertexPtr> switch_next;

  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr switch_val = get_expression();
  CE (!kphp_error(switch_val, "Failed to parse 'switch' expression"));
  switch_next.push_back(switch_val);
  CE (expect(tok_clpar, "')'"));

  CE (expect(tok_opbrc, "'{'"));

  // they will be replaced by vars later.
  // It can't be done now, gen_name is not working here.
  auto temp_ver1 = VertexAdaptor<op_empty>::create();
  switch_next.push_back(temp_ver1);
  auto temp_ver2 = VertexAdaptor<op_empty>::create();
  switch_next.push_back(temp_ver2);
  auto temp_ver3 = VertexAdaptor<op_empty>::create();
  switch_next.push_back(temp_ver3);
  auto temp_ver4 = VertexAdaptor<op_empty>::create();
  switch_next.push_back(temp_ver4);

  while (cur->type() != tok_clbrc) {
    skip_phpdoc_tokens();
    TokenType cur_type = cur->type();
    VertexPtr case_val;

    AutoLocation case_location(this);
    if (cur_type == tok_case) {
      next_cur();
      case_val = get_expression();
      CE (!kphp_error(case_val, "Failed to parse 'case' value"));

      CE (expect2(tok_colon, tok_semicolon, "':'"));
    }
    if (cur_type == tok_default) {
      next_cur();
      CE (expect2(tok_colon, tok_semicolon, "':'"));
    }

    AutoLocation seq_location(this);
    vector<VertexPtr> seq_next;
    while (cur != end) {
      if (cur->type() == tok_clbrc) {
        break;
      }
      if (cur->type() == tok_case) {
        break;
      }
      if (cur->type() == tok_default) {
        break;
      }
      VertexPtr cmd = get_statement();
      if (cmd) {
        seq_next.push_back(cmd);
      }
    }

    auto seq = VertexAdaptor<op_seq>::create(seq_next);
    set_location(seq, seq_location);
    if (cur_type == tok_case) {
      auto case_block = VertexAdaptor<op_case>::create(case_val, seq);
      set_location(case_block, case_location);
      switch_next.push_back(case_block);
    } else if (cur_type == tok_default) {
      auto case_block = VertexAdaptor<op_default>::create(seq);
      set_location(case_block, case_location);
      switch_next.push_back(case_block);
    }
  }

  auto switch_vertex = VertexAdaptor<op_switch>::create(switch_next);
  set_location(switch_vertex, switch_location);

  CE (expect(tok_clbrc, "'}'"));
  return switch_vertex;
}

bool GenTree::parse_function_uses(std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda) {
  if (test_expect(tok_use)) {
    kphp_error_act(uses_of_lambda, "Unexpected `use` token", return false);

    next_cur();
    if (!expect(tok_oppar, "`(`")) {
      return false;
    }

    std::vector<VertexAdaptor<op_var>> uses_as_vars;
    bool ok_params_next = gen_list<op_err>(&uses_as_vars, &GenTree::get_var_name_ref, tok_comma);

    for (auto &v : uses_as_vars) {
      kphp_error(!v->ref_flag, "references to variables in `use` block are forbidden in lambdas");
      uses_of_lambda->emplace_back(VertexAdaptor<op_func_param>::create(v));
    }

    return ok_params_next && expect(tok_clpar, "`)`");
  }

  return true;
}

bool GenTree::check_uses_and_args_are_not_intersecting(const std::vector<VertexAdaptor<op_func_param>> &uses, const VertexRange &params) {
  std::set<std::string> uniq_uses;
  std::transform(uses.begin(), uses.end(),
                 std::inserter(uniq_uses, uniq_uses.begin()),
                 [](VertexAdaptor<op_func_param> v) { return v->var()->get_string(); });

  return std::none_of(params.begin(), params.end(),
                      [&](VertexPtr p) { return uniq_uses.find(p.as<meta_op_func_param>()->var()->get_string()) != uniq_uses.end(); });
}

VertexPtr GenTree::get_anonymous_function(bool is_static/* = false*/) {
  std::vector<VertexAdaptor<op_func_param>> uses_of_lambda;
  VertexPtr f = get_function(vk::string_view{}, access_nonmember, false, &uses_of_lambda);

  if (auto anon_function = f.try_as<op_function>()) {
    // это constructor call
    return generate_anonymous_class(anon_function, parsed_os, cur_function, is_static, std::move(uses_of_lambda));
  }

  return {};
}

// парсим 'static public', 'final protected' и другие modifier'ы перед функцией/переменными класса
unsigned int GenTree::parse_class_member_modifier_mask() {
  unsigned int mask = 0;
  while (cur != end) {
    switch (cur->type()) {
      case tok_public:
        mask |= ClassMemberModifier::cm_public;
        break;
      case tok_private:
        mask |= ClassMemberModifier::cm_private;
        break;
      case tok_protected:
        mask |= ClassMemberModifier::cm_protected;
        break;
      case tok_static:
        mask |= ClassMemberModifier::cm_static;
        break;
      case tok_final:
        mask |= ClassMemberModifier::cm_final;
        break;
        //case tok_abstract:
        //  mask |= ClassMemberModifier::cm_abstract;
        //  break;

      default:
        return mask;
    }
    next_cur();
  }
  return mask;
}

void GenTree::check_class_member_modifier_mask(unsigned int mask, TokenType cur_tok) {
  kphp_error(!(cur_tok == tok_var_name && (mask & ClassMemberModifier::cm_final)),
             "Class fields can not be declared final");
  kphp_error(!(cur_tok == tok_var_name && (mask & ClassMemberModifier::cm_abstract)),
             "Class fields can not be declared abstract");
  kphp_error(!((mask & ClassMemberModifier::cm_final) && (mask & ClassMemberModifier::cm_abstract)),
             "Can not use final and abstract at the same time");
  int access_mod = mask & (ClassMemberModifier::cm_public | ClassMemberModifier::cm_protected | ClassMemberModifier::cm_private);
  kphp_error(!(access_mod & (access_mod - 1)),
             "Mupliple access modifiers (e.g. public and private at the same time) are not allowed");
}

VertexPtr GenTree::get_class_member(const vk::string_view &phpdoc_str) {
  unsigned int mask = parse_class_member_modifier_mask();

  const TokenType cur_tok = cur == end ? tok_end : cur->type();
  check_class_member_modifier_mask(mask, cur_tok);  // тут лучше продолжить парсить если ошибка, она напишется потом

  const bool is_static = (mask & ClassMemberModifier::cm_static) != 0;
  const AccessType access_type =
    is_static
    ? ((mask & ClassMemberModifier::cm_private) ? access_static_private :
       (mask & ClassMemberModifier::cm_protected) ? access_static_protected : access_static_public)
    : ((mask & ClassMemberModifier::cm_private) ? access_private :
       (mask & ClassMemberModifier::cm_protected) ? access_protected : access_public);

  if (cur_tok == tok_function) {
    return get_function(phpdoc_str, access_type, (mask & ClassMemberModifier::cm_final) != 0);
  }
  if (cur_tok == tok_var_name) {
    kphp_error(!cur_class->is_interface(), "Interfaces may not include member variables");
    return is_static ? get_static_field_list(phpdoc_str, access_type) : get_instance_var_list(phpdoc_str, access_type);
  }

  kphp_error(0, "Expected 'function' or $var_name after class member modifiers");
  next_cur();
  return {};
}

VertexAdaptor<op_func_param_list> GenTree::parse_cur_function_param_list() {
  vector<VertexAdaptor<meta_op_func_param>> params_next;

  CE(expect(tok_oppar, "'('"));

  if (cur_function->is_instance_function() && !cur_function->is_constructor()) {
    cur_class->patch_func_add_this(params_next, line_num);
  }

  bool ok_params_next = gen_list<op_err>(&params_next, &GenTree::get_func_param, tok_comma);
  CE(!kphp_error(ok_params_next, "Failed to parse function params"));
  CE(expect(tok_clpar, "')'"));

  return VertexAdaptor<op_func_param_list>::create(params_next).set_location(cur_function->root);
}

VertexPtr GenTree::get_function(const vk::string_view &phpdoc_str, AccessType access_type, bool is_final, std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda) {
  expect(tok_function, "'function'");
  AutoLocation func_location(this);

  auto func_name = VertexAdaptor<op_func_name>::create();
  set_location(func_name, func_location);

  // имя функции — то, что идёт после 'function' (внутри класса сразу же полное$$имя)
  if (uses_of_lambda != nullptr) {
    func_name->str_val = gen_anonymous_function_name(cur_function);   // cur_function пока ещё функция-родитель
  } else {
    CE(expect(tok_func_name, "'tok_func_name'"));
    func_name->str_val = static_cast<string>(std::prev(cur)->str_val);
    if (cur_class) {        // fname внутри класса — это full$class$name$$fname
      func_name->str_val = replace_backslashes(cur_class->name) + "$$" + func_name->str_val;
    }
  }

  auto func_root = VertexAdaptor<op_function>::create(func_name, VertexPtr{}, VertexPtr{});
  set_location(func_root, func_location);

  // создаём cur_function как func_local, а если body не окажется — сделаем func_extern
  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, FunctionData::create_function(func_root, FunctionData::func_local));
  cur_function->phpdoc_str = phpdoc_str;
  cur_function->is_final = is_final;

  if (FunctionData::is_instance_access_type(access_type)) {
    kphp_assert(cur_class);
    cur_class->members.add_instance_method(cur_function, access_type);
  } else if (FunctionData::is_static_access_type(access_type)) {
    kphp_assert(cur_class);
    cur_class->members.add_static_method(cur_function, access_type);
  }

  // после имени функции — параметры, затем блок use для замыканий
  CE(cur_function->root->params() = parse_cur_function_param_list());
  CE(parse_function_uses(uses_of_lambda));
  kphp_error(!uses_of_lambda || check_uses_and_args_are_not_intersecting(*uses_of_lambda, cur_function->get_params()),
             "arguments and captured variables(in `use` clause) must have different names");
  // а дальше может идти ::: string в functions.txt
  cur_function->root->type_rule = get_type_rule();

  // потом — либо { cmd }, либо ';' — в последнем случае это func_extern
  if (test_expect(tok_opbrc)) {
    CE(!kphp_error(!(cur_class && cur_class->is_interface()), format("methods in interface must have empty body: %s", cur_class->name.c_str())));
    is_top_of_the_function_ = true;
    cur_function->root->cmd() = get_statement();
    CE(!kphp_error(cur_function->root->cmd(), "Failed to parse function body"));
    if (cur_function->is_constructor()) {
      func_force_return(cur_function->root, ClassData::gen_vertex_this(line_num));
    } else {
      func_force_return(cur_function->root);
    }
  } else {
    CE(!kphp_error((cur_class && cur_class->is_interface()) || processing_file->is_builtin(), "function must have non-empty body"));
    CE (expect(tok_semicolon, "';'"));
    cur_function->type = FunctionData::func_extern;
    cur_function->root->cmd() = VertexAdaptor<op_seq>::create();
  }

  // функция готова, регистрируем
  // конструктор регистрируем в самом конце, после парсинга всего класса
  if (!cur_function->is_constructor()) {
    const bool kphp_required_flag = phpdoc_str.find("@kphp-required") != std::string::npos ||
                                    phpdoc_str.find("@kphp-lib-export") != std::string::npos;
    const bool auto_require = cur_function->is_extern()
                              || cur_function->is_instance_function()
                              || kphp_required_flag;
    G->register_and_require_function(cur_function, parsed_os, auto_require);
  }

  if (uses_of_lambda != nullptr && !stage::has_error()) {
    return cur_function->root;
  }

  return {};
}

bool GenTree::check_seq_end() {
  if (!test_expect(tok_clbrc)) {
    kphp_error (0, "Failed to parse sequence");
    while (cur != end && !test_expect(tok_clbrc)) {
      next_cur();
    }
  }
  return expect (tok_clbrc, "'}'");
}

bool GenTree::check_statement_end() {
  //if (test_expect (tok_clbrc)) {
  //return true;
  //}
  if (!test_expect(tok_semicolon)) {
    stage::set_line(line_num);
    kphp_error (0, "Failed to parse statement. Expected `;`");
    while (cur != end && !test_expect(tok_clbrc) && !test_expect(tok_semicolon)) {
      next_cur();
    }
  }
  return expect (tok_semicolon, "';'");
}

static inline bool is_class_name_allowed(const string &name) {
  static std::set<std::string> disallowed_names{"rpc_connection", "Long", "ULong", "UInt"};

  return disallowed_names.find(name) == disallowed_names.end();
}

void GenTree::parse_extends_implements() {
  if (test_expect(tok_extends)) {     // extends идёт раньше implements, менять местами нельзя
    next_cur();                       // (в php тоже так)
    kphp_error_return(test_expect(tok_func_name), "Class name expected after 'extends'");
    auto full_class_name = resolve_uses(cur_function, static_cast<std::string>(cur->str_val), '\\');
    cur_class->str_dependents.emplace_back(cur_class->class_type, full_class_name);
    next_cur();
  }

  if (test_expect(tok_implements)) {
    next_cur();
    kphp_error(test_expect(tok_func_name), "Class name expected after 'implements'");
    string full_class_name = resolve_uses(cur_function, static_cast<std::string>(cur->str_val), '\\');
    cur_class->str_dependents.emplace_back(ClassType::interface, full_class_name);
    next_cur();
  }
}

VertexPtr GenTree::get_class(const vk::string_view &phpdoc_str, ClassType class_type) {
  AutoLocation class_location(this);
  CE(vk::any_of_equal(cur->type(), tok_class, tok_interface));
  next_cur();

  CE (!kphp_error(test_expect(tok_func_name), "Class name expected"));

  string name_str = static_cast<string>(cur->str_val);
  next_cur();

  string full_class_name = processing_file->namespace_name.empty() ? name_str : processing_file->namespace_name + "\\" + name_str;
  kphp_error(processing_file->namespace_uses.find(name_str) == processing_file->namespace_uses.end(),
             "Class name is the same as one of 'use' at the top of the file");

  StackPushPop<ClassPtr> c_alive(class_stack, cur_class, ClassPtr(new ClassData()));
  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, cur_class->gen_holder_function(full_class_name));

  if (!is_class_name_allowed(name_str)) {
    kphp_error (false, format("Sorry, kPHP doesn't support class name %s", name_str.c_str()));
  }

  cur_class->class_type = class_type;
  parse_extends_implements();

  auto name_vertex = VertexAdaptor<op_func_name>::create();
  set_location(name_vertex, AutoLocation(this));
  name_vertex->str_val = name_str;

  auto class_vertex = VertexAdaptor<op_class>::create(name_vertex);
  set_location(class_vertex, class_location);

  cur_class->file_id = processing_file;
  cur_class->phpdoc_str = phpdoc_str;
  cur_class->root = class_vertex;
  cur_class->set_name_and_src_name(full_class_name);    // с полным неймспейсом и слешами
  cur_class->is_immutable = PhpDocTypeRuleParser::is_tag_in_phpdoc(phpdoc_str, php_doc_tag::kphp_immutable_class);

  VertexPtr body_vertex = get_statement();    // это пустой op_seq
  CE (!kphp_error(body_vertex, "Failed to parse class body"));

  cur_class->members.add_constant("class", generate_constant_field_class_value());    // A::class

  if (auto constructor_method = cur_class->members.get_constructor()) {
    CE (!kphp_error(!cur_class->is_interface(), "constructor in interfaces have not been supported yet"));
    if (!cur_class->is_builtin()) {
      cur_class->patch_func_constructor(constructor_method->root);
    }
    G->register_and_require_function(constructor_method, parsed_os, true);
  } else if (cur_class->is_class()) {
    bool non_static = cur_class->members.has_any_instance_var() || cur_class->members.has_any_instance_method();
    non_static |= std::any_of(cur_class->str_dependents.begin(), cur_class->str_dependents.end(),
                              [](const ClassData::StrDependence &dep) {
                                return dep.type == ClassType::interface;
                              });

    if (non_static) {
      cur_class->create_default_constructor(line_num, parsed_os);
    }
  }

  G->register_class(cur_class);
  ++G->stats.total_classes;
  G->register_and_require_function(cur_function, parsed_os, true);  // прокидываем класс по пайплайну

  return {};
}


VertexPtr GenTree::generate_anonymous_class(VertexAdaptor<op_function> function,
                                            DataStream<FunctionPtr> &os,
                                            FunctionPtr cur_function,
                                            bool is_static,
                                            std::vector<VertexAdaptor<op_func_param>> &&uses_of_lambda) {
  auto anon_location = function->name()->location;

  return LambdaGenerator{cur_function, anon_location, is_static}
    .add_uses(uses_of_lambda)
    .add_invoke_method(function)
    .add_constructor_from_uses()
    .generate_and_require(cur_function, os)
    ->gen_constructor_call_pass_fields_as_args();
}

VertexPtr GenTree::get_use() {
  kphp_assert(test_expect(tok_use));
  next_cur();
  while (true) {
    if (!test_expect(tok_func_name)) {
      expect(tok_func_name, "<namespace path>");
    }
    string name = static_cast<string>(cur->str_val);
    kphp_assert(!name.empty());
    if (name[0] == '\\') {
      name = name.substr(1);
    }
    string alias = name.substr(name.rfind('\\') + 1);
    kphp_error(!alias.empty(), "KPHP doesn't support use of global namespace");
    next_cur();
    if (test_expect(tok_as)) {
      next_cur();
      if (!test_expect(tok_func_name)) {
        expect(tok_func_name, "<use alias>");
      }
      alias = static_cast<string>(cur->str_val);
      next_cur();
    }
    kphp_error(processing_file->namespace_uses.emplace(alias, name).second,
               "Duplicate 'use' at the top of the file");
    if (!test_expect(tok_comma)) {
      break;
    }
    next_cur();
  }
  expect2(tok_semicolon, tok_comma, "';' or ','");
  return VertexPtr();
}

void GenTree::parse_namespace_and_uses_at_top_of_file() {
  kphp_assert (test_expect(tok_namespace));
  kphp_assert (processing_file->namespace_name.empty());

  next_cur();
  kphp_error (test_expect(tok_func_name), "Namespace name expected");
  processing_file->namespace_name = static_cast<string>(cur->str_val);
  std::string real_unified_dir = processing_file->unified_dir_name;
  if (processing_file->owner_lib) {
    vk::string_view current_file_unified_dir = real_unified_dir;
    vk::string_view lib_unified_dir = processing_file->owner_lib->unified_lib_dir();
    kphp_assert_msg(current_file_unified_dir.starts_with(lib_unified_dir), "lib processing file should be in lib dir");
    real_unified_dir.erase(0, lib_unified_dir.size() + 1);
  }
  string expected_namespace_name = replace_characters(real_unified_dir, '/', '\\');
  kphp_error (processing_file->namespace_name == expected_namespace_name,
              format("Wrong namespace name, expected %s", expected_namespace_name.c_str()));

  next_cur();
  expect (tok_semicolon, "';'");

  if (stage::has_error()) {
    while (cur != end) {
      ++cur;
    }
  }
  while (test_expect(tok_use)) {
    get_use();
  }
}

VertexPtr GenTree::get_static_field_list(const vk::string_view &phpdoc_str, AccessType access_type) {
  cur--;      // он был $field_name, делаем перед, т.к. get_multi_call() делает next_cur()
  VertexPtr v = get_multi_call<op_static>(&GenTree::get_expression);
  CE (check_statement_end());

  for (auto seq : *v) {
    // node это либо op_var $a (когда нет дефолтного значения), либо op_set(op_var $a, init_val)
    VertexPtr node = seq.as<op_static>()->args()[0];
    switch (node->type()) {
      case op_var: {
        cur_class->members.add_static_field(node.as<op_var>(), VertexAdaptor<op_empty>::create(), access_type, phpdoc_str);
        break;
      }
      case op_set: {
        auto set_expr = node.as<op_set>();
        kphp_error_act(set_expr->lhs()->type() == op_var, "unexpected expression in 'static'", break);
        cur_class->members.add_static_field(set_expr->lhs().as<op_var>(), set_expr->rhs(), access_type, phpdoc_str);
        break;
      }
      default:
        kphp_error(0, "unexpected expression in 'static'");
    }
  }

  return VertexAdaptor<op_empty>::create();
}

VertexPtr GenTree::get_statement(const vk::string_view &phpdoc_str) {
  VertexPtr res, first_node, second_node, third_node, forth_node, tmp_node;
  TokenType type = cur->type();

  VertexPtr type_rule;
  is_top_of_the_function_ &= vk::any_of_equal(type, tok_global, tok_opbrc);

  switch (type) {
    case tok_opbrc:
      next_cur();
      res = get_seq();
      kphp_error (res, "Failed to parse sequence");
      CE (check_seq_end());
      return res;
    case tok_return:
      res = get_return();
      return res;
    case tok_continue:
      res = get_break_continue<op_continue>();
      return res;
    case tok_break:
      res = get_break_continue<op_break>();
      return res;
    case tok_unset:
      res = get_func_call<op_unset, op_err>();
      CE (check_statement_end());
      return res;
    case tok_var_dump:
      res = get_func_call<op_var_dump, op_err>();
      CE (check_statement_end());
      return res;
    case tok_define:
      res = get_func_call<op_define, op_err>();
      CE (check_statement_end());
      return res;
    case tok_global:
      if (G->env().get_warnings_level() >= 2 && cur_function && cur_function->type == FunctionData::func_local && !is_top_of_the_function_) {
        kphp_warning("`global` keyword is allowed only at the top of the function");
      }
      res = get_multi_call<op_global>(&GenTree::get_var_name);
      CE (check_statement_end());
      return res;
    case tok_protected:
    case tok_public:
    case tok_private:
    case tok_final:
      if (cur_function->type == FunctionData::func_class_holder) {
        return get_class_member(phpdoc_str);
      }
      next_cur();
      kphp_error(0, "Unexpected access modifier outside of class body");
      return {};
    case tok_static:
      if (cur_function->type == FunctionData::func_class_holder) {
        return get_class_member(phpdoc_str);
      }
      if (cur + 1 != end && ((cur + 1)->type() == tok_var_name)) {   // статическая переменная функции
        res = get_multi_call<op_static>(&GenTree::get_expression);
        CE (check_statement_end());
        return res;
      }
      next_cur();
      kphp_error(0, "Expected function or variable after keyword `static`");
      return {};
    case tok_echo:
      res = get_multi_call<op_echo>(&GenTree::get_expression);
      CE (check_statement_end());
      return res;
    case tok_dbg_echo:
      res = get_multi_call<op_dbg_echo>(&GenTree::get_expression);
      CE (check_statement_end());
      return res;
    case tok_throw: {
      AutoLocation throw_location(this);
      next_cur();
      first_node = get_expression();
      CE (!kphp_error(first_node, "Empty expression in throw"));
      auto throw_vertex = VertexAdaptor<op_throw>::create(first_node);
      set_location(throw_vertex, throw_location);
      CE (check_statement_end());
      return throw_vertex;
    }

    case tok_while:
      return get_while();
    case tok_if:
      return get_if();
    case tok_for:
      return get_for();
    case tok_do:
      return get_do();
    case tok_foreach:
      return get_foreach();
    case tok_switch:
      return get_switch();
    case tok_phpdoc: {
      const Token *token = &*cur;
      next_cur();
      return get_statement(token->str_val);
    }
    case tok_function:
      if (cur_class) {      // пропущен access modifier — значит, public
        return get_function(phpdoc_str, access_public);
      }
      return get_function(phpdoc_str, access_nonmember);

    case tok_try: {
      AutoLocation try_location(this);
      next_cur();
      first_node = get_statement();
      CE (!kphp_error(first_node, "Cannot parse try block"));
      CE (expect(tok_catch, "'catch'"));
      CE (expect(tok_oppar, "'('"));
      CE (expect(tok_func_name, "'Exception'"));
      second_node = get_expression();
      CE (!kphp_error(second_node, "Cannot parse catch ( ??? )"));
      CE (!kphp_error(second_node->type() == op_var, "Expected variable name in 'catch'"));

      CE (expect(tok_clpar, "')'"));
      third_node = get_statement();
      CE (!kphp_error(third_node, "Cannot parse catch block"));
      auto try_vertex = VertexAdaptor<op_try>::create(embrace(first_node), second_node, embrace(third_node));
      set_location(try_vertex, try_location);
      return try_vertex;
    }
    case tok_inline_html: {
      auto html_code = VertexAdaptor<op_string>::create();
      set_location(html_code, AutoLocation(this));
      html_code->str_val = static_cast<string>(cur->str_val);

      auto echo_cmd = VertexAdaptor<op_echo>::create(html_code);
      set_location(echo_cmd, AutoLocation(this));
      next_cur();
      return echo_cmd;
    }
    case tok_at: {
      AutoLocation noerr_location(this);
      next_cur();
      first_node = get_statement();
      CE (first_node);
      auto noerr = VertexAdaptor<op_noerr>::create(first_node);
      set_location(noerr, noerr_location);
      return noerr;
    }
    case tok_clbrc: {
      return res;
    }
    case tok_const: {
      AutoLocation const_location(this);
      next_cur();

      bool has_access_modifier = std::distance(tokens.begin(), cur) > 1 && vk::any_of_equal(std::prev(cur, 2)->type(), tok_public, tok_private, tok_protected);
      bool const_in_global_scope = functions_stack.size() == 1 && !cur_class;
      bool const_in_class = !!cur_class;
      string const_name{cur->str_val};

      CE (!kphp_error(const_in_global_scope || const_in_class, "const expressions supported only inside classes and namespaces or in global scope"));
      CE (!kphp_error(test_expect(tok_func_name), "expected constant name"));
      CE (!kphp_error(!has_access_modifier, "unexpected const after private/protected/public keyword"));

      next_cur();
      CE (expect(tok_eq1, "'='"));
      VertexPtr v = get_expression();
      CE (check_statement_end());

      if (const_in_class) {
        cur_class->members.add_constant(const_name, v);
        return VertexAdaptor<op_empty>::create();
      }

      auto name = VertexAdaptor<op_string>::create();
      name->str_val = const_name;
      auto def = VertexAdaptor<op_define>::create(name, v);
      set_location(def, const_location);

      return def;
    }
    case tok_use: {
      AutoLocation const_location(this);
      kphp_error(!cur_class && cur_function && cur_function->type == FunctionData::func_global, "'use' can be declared only in global scope");
      get_use();
      auto empty = VertexAdaptor<op_empty>::create();
      return empty;
    }
    case tok_var: {
      next_cur();
      get_instance_var_list(phpdoc_str, access_public);
      CE (check_statement_end());
      auto empty = VertexAdaptor<op_empty>::create();
      return empty;
    }
    case tok_class:
      res = get_class(phpdoc_str, ClassType::klass);
      return res;
    case tok_interface:
      res = get_class(phpdoc_str, ClassType::interface);
      return res;
    default:
      res = get_expression();
      if (!res) {
        if (cur->type() == tok_semicolon) {
          auto empty = VertexAdaptor<op_empty>::create();
          set_location(empty, AutoLocation(this));
          res = empty;
        } else if (!phpdoc_str.empty()) {
          return res;
        } else {
          CE (check_statement_end());
          return res;
        }
      } else {
        type_rule = get_type_rule();
        res->type_rule = type_rule;
        if (res->type() == op_set) {
          res.as<op_set>()->phpdoc_str = phpdoc_str;
        }
      }
      CE (check_statement_end());
      //CE (expect (tok_semicolon, "';'"));
      return res;
  }
  kphp_fail();
}

VertexPtr GenTree::get_instance_var_list(const vk::string_view &phpdoc_str, AccessType access_type) {
  kphp_error(cur_class, "var declaration is outside of class");

  const vk::string_view &var_name = cur->str_val;
  CE (expect(tok_var_name, "expected variable name"));

  VertexPtr def_val;
  if (test_expect(tok_eq1)) {
    next_cur();
    def_val = get_expression();
  }

  auto var = VertexAdaptor<op_var>::create();
  var->str_val = static_cast<string>(var_name);
  set_location(var, AutoLocation(this));

  cur_class->members.add_instance_field(var, def_val, access_type, phpdoc_str);

  if (test_expect(tok_comma)) {
    next_cur();
    get_instance_var_list(phpdoc_str, access_type);
  }

  return VertexPtr();
}

void GenTree::get_seq(std::vector<VertexPtr> &seq_next) {
  while (cur != end && !test_expect(tok_clbrc)) {
    VertexPtr cur_node = get_statement();
    if (!cur_node) {
      continue;
    }
    seq_next.push_back(cur_node);
  }
}

VertexPtr GenTree::get_seq() {
  vector<VertexPtr> seq_next;
  AutoLocation seq_location(this);

  get_seq(seq_next);

  auto seq = VertexAdaptor<op_seq>::create(seq_next);
  set_location(seq, seq_location);

  return seq;
}

void GenTree::run() {
  auto v_func_name = VertexAdaptor<op_func_name>::create();
  v_func_name->set_string(processing_file->main_func_name);
  auto v_func_params = VertexAdaptor<op_func_param_list>::create();
  auto root = VertexAdaptor<op_function>::create(v_func_name, v_func_params, VertexPtr{});

  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, FunctionData::create_function(root, FunctionData::func_global));
  processing_file->main_function = cur_function;

  if (test_expect(tok_namespace)) {
    parse_namespace_and_uses_at_top_of_file();
  }

  AutoLocation seq_location(this);
  std::vector<VertexPtr> seq_next;
  auto set_run_flag = VertexAdaptor<op_set>::create(processing_file->get_main_func_run_var(), VertexAdaptor<op_true>::create());
  seq_next.push_back(set_run_flag);
  get_seq(seq_next);
  seq_next.push_back(VertexAdaptor<op_return>::create());
  cur_function->root->cmd() = VertexAdaptor<op_seq>::create(seq_next);
  set_location(cur_function->root->cmd(), seq_location);

  G->register_and_require_function(cur_function, parsed_os, true);  // global функция — поэтому required

  if (cur != end) {
    fprintf(stderr, "line %d: something wrong\n", line_num);
    kphp_error (0, "Cannot compile (probably problems with brace balance)");
  }
}

void php_gen_tree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os) {
  GenTree gen(std::move(tokens), file, os);
  gen.run();
}

#undef CE
