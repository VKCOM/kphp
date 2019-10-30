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

GenTree::GenTree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os) :
  line_num(0),
  tokens(std::move(tokens)),
  parsed_os(os),
  is_top_of_the_function_(false),
  cur(this->tokens.begin()),
  end(this->tokens.end()),
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

#define expect_msg(msg) ({ \
  fmt_format ("Expected {}, found '{}'", msg, cur == end ? "END OF FILE" : cur->to_str().c_str()); \
})

#define expect(tp, msg) ({ \
  bool res__;\
  if (kphp_error (test_expect (tp), expect_msg(msg))) {\
    res__ = false;\
  } else {\
    next_cur();\
    res__ = true;\
  }\
  res__; \
})

#define expect2(tp1, tp2, msg) ({ \
  kphp_error (test_expect (tp1) || test_expect (tp2), expect_msg(msg)); \
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
  if (is_opened) {
    CE(expect(tok_clpar, "')'"));
  }
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
  bool ok_next = gen_list<EmptyOp>(&next, &GenTree::get_expression, tok_comma);
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

  VertexPtr call = VertexAdaptor<Op>::create_vararg(next);
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
    return VertexAdaptor<op_arrow>::create(VertexAdaptor<op_false>::create(), call);
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

      call->set_string(ClassData::NAME_OF_INVOKE_METHOD);

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
      v->str_val = std::to_string(stage::get_line());
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
        fmt_format("It's not allowed using `...` in this place (op: {})", OpInfo::str(res->type()))));
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
      auto print = VertexAdaptor<op_func_call>::create(first_node);
      print->str_val = "print";
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
    case tok_declare:
      // см. GenTree::parse_declare_at_top_of_file
      kphp_error(0, "strict_types declaration must be the very first statement in the script");
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

VertexAdaptor<op_ternary> GenTree::create_ternary_op_vertex(VertexPtr left, VertexPtr right, VertexPtr third) {
  if (right) {
    return VertexAdaptor<op_ternary>::create(left, right, third);
  }

  auto cond_var = create_superlocal_var("shorthand_ternary_cond");

  auto cond = conv_to<tp_bool>(VertexAdaptor<op_set>::create(cond_var, left));

  auto left_var_move = VertexAdaptor<op_move>::create(cond_var.clone());
  return VertexAdaptor<op_ternary>::create(cond, left_var_move, third);
}

VertexAdaptor<op_type_expr_class> GenTree::create_type_help_class_vertex(vk::string_view klass_name) {
  return create_type_help_class_vertex(G->get_class(resolve_uses(cur_function, static_cast<std::string>(klass_name))));
}

VertexAdaptor<op_type_expr_class> GenTree::create_type_help_class_vertex(ClassPtr klass) {
  auto type_rule = VertexAdaptor<op_type_expr_class>::create();
  type_rule->type_help = tp_Class;
  type_rule->class_ptr = klass;
  return type_rule;
}

VertexAdaptor<op_type_expr_type> GenTree::create_type_help_vertex(PrimitiveType type, const std::vector<VertexPtr> &children) {
  auto type_rule = VertexAdaptor<op_type_expr_type>::create(children);
  type_rule->type_help = type;
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
      kphp_error (0, fmt_format("Failed to parse second argument in [{}]", OpInfo::str(binary_op_tp)));
      return {};
    }

    VertexPtr third;
    if (ternary) {
      CE (expect(tok_colon, "':'"));
      third = get_expression_impl(true);
      if (!third) {
        kphp_error (0, fmt_format("Failed to parse third argument in [{}]", OpInfo::str(binary_op_tp)));
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

    if (ternary) {
      left = create_ternary_op_vertex(left, right, third);
    } else {
      left = create_vertex(binary_op_tp, left, right);
    }

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

VertexAdaptor<op_seq> GenTree::embrace(VertexPtr v) {
  if (auto seq = v.try_as<op_seq>()) {
    return seq;
  }
  return VertexAdaptor<op_seq>::create(v).set_location(v);
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
  if (vk::any_of_equal(cur->type(), tok_func_name, tok_array, tok_callable)) {
    tok_type_declaration = &*cur;
    next_cur();
  }

  VertexAdaptor<op_var> name = get_var_name_ref();
  if (!name) {
    return {};
  }

  PrimitiveType tp = tp_Unknown;
  VertexAdaptor<meta_op_type_rule> type_rule;
  if (!from_callback && cur->type() == tok_triple_colon) {
    tp = get_func_param_type_help();    // запишется в param->type_help, и при вызове будет неявный cast
  } else {
    type_rule = get_type_rule();
  }

  VertexPtr def_val = get_def_value();
  VertexAdaptor<op_func_param> v;
  if (def_val) {
    v = VertexAdaptor<op_func_param>::create(name, def_val);
  } else {
    v = VertexAdaptor<op_func_param>::create(name);
  }
  set_location(v, st_location);
  if (tok_type_declaration != nullptr) {
    v->type_declaration = static_cast<string>(tok_type_declaration->str_val);
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
    auto name = VertexAdaptor<op_var>::create();
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

    VertexAdaptor<meta_op_type_rule> type_rule = get_type_rule();

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

VertexAdaptor<op_foreach_param> GenTree::get_foreach_param() {
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

  VertexPtr temp_var;
  if (x->ref_flag) {
    temp_var = VertexAdaptor<op_empty>::create();
  } else {
    temp_var = create_superlocal_var("tmp_expr");
  }

  VertexAdaptor<op_foreach_param> res;
  if (key) {
    res = VertexAdaptor<op_foreach_param>::create(xs, x, temp_var, key);
  } else {
    res = VertexAdaptor<op_foreach_param>::create(xs, x, temp_var);
  }
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
      case tp_string:
        return conv_to_lval<tp_string>(x);
      case tp_var:
        return x;
        break;
      default:
        kphp_error (0, "convert_to supports only var, array, string or int with ref_flag");
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
  if (auto var = v.try_as<op_var>()) {
    if (var->extra_type == op_ex_var_const && var->var_id) {
      return var->var_id->init_val;
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

int GenTree::get_id_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr) {
  int id = -1;
  if (auto fun_call = expr.try_as<op_func_call>()) {
    id = arg->int_val - 1;
    if (auto func_id = fun_call->func_id) {
      if (id < 0 || id >= func_id->get_params().size()) {
        id = -1;
      }
    }
  }
  return id;
}

VertexPtr GenTree::get_call_arg_ref(VertexAdaptor<op_type_expr_arg_ref> arg, VertexPtr expr) {
  int arg_id = get_id_arg_ref(arg, expr);
  if (arg_id != -1) {
    auto call_args = expr.as<op_func_call>()->args();
    return arg_id < call_args.size() ? call_args[arg_id] : VertexPtr{};
  }
  return {};
}

PrimitiveType GenTree::get_func_param_type_help() {
  kphp_assert(cur->type() == tok_triple_colon);

  next_cur();
  PhpDocTypeRuleParser parser(cur_function);
  VertexPtr type_expr = parser.parse_from_tokens_silent(cur);
  kphp_error(type_expr->type() == op_type_expr_type, "Incorrect type_help after :::");

  return type_expr->type_help;
}

VertexAdaptor<meta_op_type_rule> GenTree::get_type_rule() {
  VertexAdaptor<meta_op_type_rule> res;

  TokenType tp = cur->type();
  if (vk::any_of_equal(tp, tok_triple_colon, tok_triple_eq, tok_triple_lt, tok_triple_gt)) {
    AutoLocation rule_location(this);
    next_cur();

    PhpDocTypeRuleParser parser(cur_function);
    VertexPtr type_expr = parser.parse_from_tokens_silent(cur);
    CE(!kphp_error(type_expr, "Cannot parse type rule"));

    VertexPtr rule = create_vertex(OpInfo::tok_to_op[tp], type_expr);
    set_location(rule, rule_location);
    res = rule.as<meta_op_type_rule>();
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
  func->cmd_ref() = VertexAdaptor<op_seq>::create(next);
}

template<Operation Op, class FuncT, class ResultType>
VertexAdaptor<op_seq> GenTree::get_multi_call(FuncT &&f, bool parenthesis) {
  AutoLocation seq_location(this);
  next_cur();

  if (parenthesis) {
    CE (expect(tok_oppar, "'('"));
  }

  std::vector<ResultType> next;
  bool ok_next = gen_list<op_err>(&next, f, tok_comma);
  CE (!kphp_error(ok_next, "Failed get_multi_call"));
  if (parenthesis) {
    CE (expect(tok_clpar, "')'"));
  }

  std::vector<VertexAdaptor<Op>> new_next;
  new_next.reserve(next.size());

  for (VertexPtr next_elem : next) {
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

VertexAdaptor<op_exit> GenTree::generate_exit_zero() {
  auto zero = VertexAdaptor<op_int_const>::create();
  zero->str_val = "0";
  return VertexAdaptor<op_exit>::create(zero);
}

VertexAdaptor<op_exit> GenTree::get_exit() {
  AutoLocation exit_location(this);
  next_cur();

  VertexAdaptor<op_exit> exit_v;
  if (open_parent()) {
    if (auto return_code = get_expression()) {
      exit_v = VertexAdaptor<op_exit>::create(return_code);
    }
    CE(expect(tok_clpar, "')'"));
  }

  // exit; <- is a valid statement
  if (!exit_v) {
    exit_v = generate_exit_zero();
  }
  set_location(exit_v, exit_location);

  return exit_v;
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
  auto first_node = get_foreach_param();
  CE (!kphp_error(first_node, "Failed to parse 'foreach' params"));

  CE (expect(tok_clpar, "')'"));

  VertexPtr second_node = get_statement();
  CE (!kphp_error(second_node, "Failed to parse 'foreach' body"));

  auto foreach = VertexAdaptor<op_foreach>::create(first_node, embrace(second_node));
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

  VertexPtr third_node = VertexPtr();
  if (cur->type() == tok_else) {
    next_cur();
    third_node = get_statement();
    CE (!kphp_error(third_node, "Failed to parse 'else' statement"));
  }

  if (third_node) {
    auto v = VertexAdaptor<op_if>::create(first_node, embrace(second_node), embrace(third_node));
    if_vertex = v;
  } else {
    auto v = VertexAdaptor<op_if>::create(first_node, embrace(second_node));
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

  auto for_vertex = VertexAdaptor<op_for>::create(pre_cond, cond, post_cond, embrace(cmd));
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

  auto do_vertex = VertexAdaptor<op_do>::create(second_node, embrace(first_node));
  set_location(do_vertex, do_location);
  return do_vertex;
}

VertexAdaptor<op_var> GenTree::create_superlocal_var(const std::string &name_prefix, PrimitiveType tp) {
  return create_superlocal_var(name_prefix, cur_function, tp);
}

VertexAdaptor<op_var> GenTree::create_superlocal_var(const std::string &name_prefix, FunctionPtr cur_function, PrimitiveType tp) {
  auto v = VertexAdaptor<op_var>::create();
  v->str_val = gen_unique_name(name_prefix, cur_function);
  v->extra_type = op_ex_var_superlocal;
  v->type_help = tp;
  return v;
}

VertexAdaptor<op_switch> GenTree::create_switch_vertex(FunctionPtr cur_function, VertexPtr switch_condition,std::vector<VertexPtr> &&cases) {
  auto temp_var_condition_on_switch = create_superlocal_var("condition_on_switch", cur_function, tp_var);
  auto temp_var_matched_with_one_case = create_superlocal_var("matched_with_one_case", cur_function, tp_bool);
  return VertexAdaptor<op_switch>::create(switch_condition, temp_var_condition_on_switch, temp_var_matched_with_one_case, std::move(cases));
}

VertexPtr GenTree::get_switch() {
  AutoLocation switch_location(this);
  vector<VertexPtr> switch_next;

  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  auto switch_condition = get_expression();
  CE (!kphp_error(switch_condition, "Failed to parse 'switch' expression"));
  CE (expect(tok_clpar, "')'"));

  CE (expect(tok_opbrc, "'{'"));

  while (cur->type() != tok_clbrc) {
    skip_phpdoc_tokens();
    auto cur_type = cur->type();
    VertexPtr case_val;

    AutoLocation case_location(this);
    if (cur_type == tok_case) {
      next_cur();
      case_val = get_expression();
      CE (!kphp_error(case_val, "Failed to parse 'case' value"));

      CE (expect2(tok_colon, tok_semicolon, "':'"));
    } else {
      CE(test_expect(tok_default));
      next_cur();
      CE (expect2(tok_colon, tok_semicolon, "':'"));
    }

    AutoLocation seq_location(this);
    vector<VertexPtr> seq_next;
    while (cur != end && vk::none_of_equal(cur->type(), tok_clbrc, tok_case, tok_default)) {
      if (auto cmd = get_statement()) {
        seq_next.push_back(cmd);
      }
    }

    auto seq = VertexAdaptor<op_seq>::create(seq_next);
    set_location(seq, seq_location);
    if (cur_type == tok_case) {
      switch_next.emplace_back(VertexAdaptor<op_case>::create(case_val, seq));
    } else {
      switch_next.emplace_back(VertexAdaptor<op_default>::create(seq));
    }
    set_location(switch_next.back(), case_location);
  }

  auto switch_vertex = create_switch_vertex(cur_function, switch_condition, std::move(switch_next));
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
  VertexPtr f = get_function(vk::string_view{}, FunctionModifiers::nonmember(), &uses_of_lambda);

  if (auto anon_function = f.try_as<op_function>()) {
    // это constructor call
    kphp_assert(!functions_stack.empty());
    lambda_generators.push_back(generate_anonymous_class(anon_function, cur_function, is_static, std::move(uses_of_lambda), anon_function->func_id));
    auto res = lambda_generators.back()
                                .get_generated_lambda()
                                ->gen_constructor_call_pass_fields_as_args();

    return res;
  }

  return {};
}

// парсим 'static public', 'final protected' и другие modifier'ы перед функцией/переменными класса
ClassMemberModifiers GenTree::parse_class_member_modifier_mask() {
  auto modifiers = ClassMemberModifiers::nonmember();
  while (cur != end) {
    switch (cur->type()) {
      case tok_public:
        modifiers.set_public();
        break;
      case tok_private:
        modifiers.set_private();
        break;
      case tok_protected:
        modifiers.set_protected();
        break;
      case tok_static:
        modifiers.set_static();
        break;
      case tok_final:
        modifiers.set_final();
        break;
      case tok_abstract:
        modifiers.set_abstract();
        break;
      default:
        return modifiers;
    }
    next_cur();
  }
  return modifiers;
}

VertexPtr GenTree::get_class_member(const vk::string_view &phpdoc_str) {
  auto modifiers = parse_class_member_modifier_mask();
  if (!modifiers.is_static()) {
    modifiers.set_instance();
  }

  if (!modifiers.is_public() && !modifiers.is_private() && !modifiers.is_protected()) {
    modifiers.set_public();
  }

  const TokenType cur_tok = cur == end ? tok_end : cur->type();

  if (cur_tok == tok_function) {
    return get_function(phpdoc_str, modifiers);
  }

  if (cur_tok == tok_var_name) {
    kphp_error(!cur_class->is_interface(), "Interfaces may not include member variables");
    kphp_error(!modifiers.is_final() && !modifiers.is_abstract(), "Class fields can not be declared final/abstract");
    if (modifiers.is_static()) {
      return get_static_field_list(phpdoc_str, FieldModifiers{modifiers.access_modifier()});
    }
    return get_instance_var_list(phpdoc_str, FieldModifiers{modifiers.access_modifier()});
  }

  kphp_error(0, "Expected 'function' or $var_name after class member modifiers");
  next_cur();
  return {};
}

VertexAdaptor<op_func_param_list> GenTree::parse_cur_function_param_list() {
  vector<VertexAdaptor<meta_op_func_param>> params_next;

  CE(expect(tok_oppar, "'('"));

  if (cur_function->modifiers.is_instance()) {
    ClassData::patch_func_add_this(params_next, Location(line_num));
  }

  bool ok_params_next = gen_list<op_err>(&params_next, &GenTree::get_func_param, tok_comma);
  CE(!kphp_error(ok_params_next, "Failed to parse function params"));
  CE(expect(tok_clpar, "')'"));

  return VertexAdaptor<op_func_param_list>::create(params_next).set_location(cur_function->root);
}

VertexPtr GenTree::get_function(const vk::string_view &phpdoc_str, FunctionModifiers modifiers, std::vector<VertexAdaptor<op_func_param>> *uses_of_lambda) {
  expect(tok_function, "'function'");
  AutoLocation func_location(this);

  std::string func_name;
  bool is_lambda = uses_of_lambda != nullptr;

  // имя функции — то, что идёт после 'function' (внутри класса сразу же полное$$имя)
  if (is_lambda) {
    func_name = gen_anonymous_function_name(cur_function);   // cur_function пока ещё функция-родитель
  } else {
    CE(expect(tok_func_name, "'tok_func_name'"));
    func_name = static_cast<string>(std::prev(cur)->str_val);
    if (cur_class) {        // fname внутри класса — это full$class$name$$fname
      func_name = replace_backslashes(cur_class->name) + "$$" + func_name;
    }
  }

  if (cur_class) {
    kphp_error(!(modifiers.is_abstract() && cur_class->is_interface()), fmt_format("abstract methods may not be declared in interfaces: {}", cur_class->name));

    if (cur_class->is_interface()) {
      modifiers.set_abstract();
    } else if (cur_class->modifiers.is_final()) {
      modifiers.set_final();
    }
  }

  auto func_root = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>{}, VertexAdaptor<op_seq>{});
  set_location(func_root, func_location);

  // создаём cur_function как func_local, а если body не окажется — сделаем func_extern
  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, FunctionData::create_function(func_name, func_root, FunctionData::func_local));
  cur_function->phpdoc_str = phpdoc_str;
  cur_function->modifiers = modifiers;

  // после имени функции — параметры, затем блок use для замыканий
  CE(cur_function->root->params_ref() = parse_cur_function_param_list());
  CE(parse_function_uses(uses_of_lambda));
  kphp_error(!uses_of_lambda || check_uses_and_args_are_not_intersecting(*uses_of_lambda, cur_function->get_params()),
             "arguments and captured variables(in `use` clause) must have different names");
  // а дальше может идти ::: string в functions.txt
  cur_function->root->type_rule = get_type_rule();
  if (is_lambda) {
    cur_function->modifiers.set_instance();
    cur_function->modifiers.set_public();
  }

  // потом — либо { cmd }, либо ';' — в последнем случае это func_extern
  if (test_expect(tok_opbrc)) {
    CE(!kphp_error(!cur_function->modifiers.is_abstract(), fmt_format("abstract methods must have empty body: {}", cur_function->get_human_readable_name())));
    is_top_of_the_function_ = true;
    cur_function->root->cmd_ref() = get_statement().as<op_seq>();
    CE(!kphp_error(cur_function->root->cmd(), "Failed to parse function body"));
    if (cur_function->is_constructor()) {
      func_force_return(cur_function->root, ClassData::gen_vertex_this(Location(line_num)));
    } else {
      func_force_return(cur_function->root);
    }
  } else {
    CE(!kphp_error(cur_function->modifiers.is_abstract() || processing_file->is_builtin(), "function must have non-empty body"));
    CE (expect(tok_semicolon, "';'"));
    cur_function->type = FunctionData::func_extern;
    cur_function->root->cmd_ref() = VertexAdaptor<op_seq>::create();
  }

  if (!is_lambda) {
    if (cur_function->modifiers.is_instance()) {
      kphp_assert(cur_class);

      cur_class->members.add_instance_method(cur_function);
    } else if (modifiers.is_static()) {
      kphp_assert(cur_class);
      cur_class->members.add_static_method(cur_function);
    }

    // функция готова, регистрируем
    // конструктор регистрируем в самом конце, после парсинга всего класса
    if (!cur_function->is_constructor()) {
      const bool kphp_required_flag = phpdoc_str.find("@kphp-required") != std::string::npos ||
                                      phpdoc_str.find("@kphp-lib-export") != std::string::npos;
      const bool auto_require = cur_function->is_extern()
                                || cur_function->modifiers.is_instance()
                                || kphp_required_flag;
      G->register_and_require_function(cur_function, parsed_os, auto_require);
    }

    require_lambdas();
  } else if (!stage::has_error()) {
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

static inline bool is_class_name_allowed(const vk::string_view &name) {
  static std::set<vk::string_view> disallowed_names{"rpc_connection", "Long", "ULong", "UInt"};

  return disallowed_names.find(name) == disallowed_names.end();
}

void GenTree::parse_extends_implements() {
  if (test_expect(tok_extends)) {     // extends идёт раньше implements, менять местами нельзя
    next_cur();                       // (в php тоже так)
    kphp_error_return(test_expect(tok_func_name), "Class name expected after 'extends'");
    kphp_error(!cur_class->is_trait(), "Traits may not extend each other");
    cur_class->add_str_dependent(cur_function, cur_class->class_type, cur->str_val);
    next_cur();
  }

  if (test_expect(tok_implements)) {
    next_cur();
    kphp_error(test_expect(tok_func_name), "Class name expected after 'implements'");
    cur_class->add_str_dependent(cur_function, ClassType::interface, cur->str_val);
    next_cur();
  }
}

VertexPtr GenTree::get_class(const vk::string_view &phpdoc_str, ClassType class_type) {
  AutoLocation class_location(this);

  ClassModifiers modifiers;
  if (test_expect(tok_abstract)) {
    modifiers.set_abstract();
  } else if (test_expect(tok_final)) {
    modifiers.set_final();
  }

  if (modifiers.is_abstract() || modifiers.is_final()) {
    next_cur();
    CE(!kphp_error(cur->type() == tok_class, "`class` epxtected after abstract/final keyword"));
  }

  CE(vk::any_of_equal(cur->type(), tok_class, tok_interface, tok_trait));
  next_cur();

  CE (!kphp_error(test_expect(tok_func_name), "Class name expected"));

  auto name_str = cur->str_val;
  next_cur();

  std::string full_class_name;
  if (processing_file->namespace_name.empty() && phpdoc_tag_exists(phpdoc_str, php_doc_tag::kphp_tl_class)) {
    kphp_assert(processing_file->is_builtin());
    full_class_name = G->env().get_tl_namespace_prefix() + name_str;
  } else {
    full_class_name = processing_file->namespace_name.empty() ? std::string{name_str} : processing_file->namespace_name + "\\" + name_str;
  }

  kphp_error(processing_file->namespace_uses.find(name_str) == processing_file->namespace_uses.end(),
             "Class name is the same as one of 'use' at the top of the file");

  StackPushPop<ClassPtr> c_alive(class_stack, cur_class, ClassPtr(new ClassData()));
  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, cur_class->gen_holder_function(full_class_name));

  if (!is_class_name_allowed(name_str)) {
    kphp_error (false, fmt_format("Sorry, kPHP doesn't support class name {}", name_str));
  }

  cur_class->modifiers = modifiers;
  cur_class->class_type = class_type;
  if (cur_class->is_interface()) {
    cur_class->modifiers.set_abstract();
  }
  parse_extends_implements();

  auto name_vertex = VertexAdaptor<op_func_name>::create();
  set_location(name_vertex, AutoLocation(this));
  name_vertex->str_val = static_cast<std::string>(name_str);

  cur_class->file_id = processing_file;
  cur_class->set_name_and_src_name(full_class_name, phpdoc_str);    // с полным неймспейсом и слешами
  cur_class->is_immutable = phpdoc_tag_exists(phpdoc_str, php_doc_tag::kphp_immutable_class);
  cur_class->location_line_num = line_num;

  VertexPtr body_vertex = get_statement();    // это пустой op_seq
  CE (!kphp_error(body_vertex, "Failed to parse class body"));

  cur_class->members.add_constant("class", generate_constant_field_class_value());    // A::class

  if (auto constructor_method = cur_class->members.get_constructor()) {
    CE (!kphp_error(!cur_class->modifiers.is_abstract(), "constructor in interfaces/abstract classes has not been supported yet"));
    cur_class->has_custom_constructor = true;
    G->register_and_require_function(constructor_method, parsed_os, true);
  }

  G->register_class(cur_class);
  ++G->stats.total_classes;
  G->register_and_require_function(cur_function, parsed_os, true);  // прокидываем класс по пайплайну

  return {};
}


LambdaGenerator GenTree::generate_anonymous_class(VertexAdaptor<op_function> function,
                                                  FunctionPtr parent_function,
                                                  bool is_static,
                                                  std::vector<VertexAdaptor<op_func_param>> &&uses_of_lambda,
                                                  FunctionPtr already_created_function /*= FunctionPtr{}*/) {
  auto anon_location = function->location;

  return LambdaGenerator{parent_function, anon_location, is_static}
    .add_uses(uses_of_lambda)
    .add_invoke_method(function, already_created_function)
    .add_constructor_from_uses()
    .generate(parent_function);
}

VertexAdaptor<op_func_call> GenTree::generate_call_on_instance_var(VertexPtr instance_var, FunctionPtr function) {
  auto params = function->get_params_as_vector_of_vars(1);
  auto call_method = VertexAdaptor<op_func_call>::create(instance_var, params);

  if (function->has_variadic_param) {
    kphp_assert(!call_method->args().empty());
    auto &last_param_passed_to_method = *std::prev(call_method->args().end());
    last_param_passed_to_method = VertexAdaptor<op_varg>::create(params.back()).set_location(params.back());
  }

  call_method->set_string(std::string{function->local_name()});
  call_method->extra_type = op_ex_func_call_arrow;

  return call_method;
}

void GenTree::get_traits_uses() {
  next_cur();
  auto name = cur->str_val;
  if (!expect(tok_func_name, "trait name expected")|| !expect(tok_semicolon, "`;` expected")) {
    return;
  }

  kphp_error(!cur_class->is_interface(), "You may not use traits inside interface");
  cur_class->add_str_dependent(cur_function, ClassType::trait, name);
}

void GenTree::get_use() {
  kphp_assert(test_expect(tok_use));

  do {
    next_cur();
    auto name = cur->str_val;
    if (!expect(tok_func_name, "<namespace path>")) {
      return;
    }

    kphp_assert(!name.empty());

    if (name[0] == '\\') {
      name = name.substr(1);
    }
    auto alias = name.substr(name.rfind('\\') + 1);
    kphp_error(!alias.empty(), "KPHP doesn't support use of global namespace");

    if (test_expect(tok_as)) {
      next_cur();
      alias = cur->str_val;
      if (!expect(tok_func_name, "<use alias>")) {
        return;
      }
    }

    kphp_error(processing_file->namespace_uses.emplace(alias, name).second, "Duplicate 'use' at the top of the file");
  } while (test_expect(tok_comma));

  expect(tok_semicolon, "';'");
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
              fmt_format("Wrong namespace name, expected {}", expected_namespace_name));

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

void GenTree::parse_declare_at_top_of_file() {
  kphp_assert(test_expect(tok_declare));

  next_cur();
  expect(tok_oppar, "(");

  if (test_expect(tok_func_name)) {
    // В declare могут быть следующие директивы: strict_types, encoding, ticks.
    // Поддерживаем только strict_types.
    kphp_error(cur->str_val == "strict_types", fmt_format("Unsupported declare '{}'", cur->str_val));
    next_cur();
  } else {
    kphp_error(0, expect_msg("'tok_func_name'"));
  }

  expect(tok_eq1, "=");

  if (test_expect(tok_int_const)) {
    kphp_error(vk::any_of_equal(cur->str_val, "0", "1"), "strict_types declaration must have 0 or 1 as its value");
    next_cur();
  } else {
    kphp_error(0, expect_msg("'tok_int_const'"));
  }

  expect(tok_clpar, ")");
}

VertexPtr GenTree::get_static_field_list(const vk::string_view &phpdoc_str, FieldModifiers modifiers) {
  cur--;      // он был $field_name, делаем перед, т.к. get_multi_call() делает next_cur()
  VertexPtr v = get_multi_call<op_static>(&GenTree::get_expression);
  CE (check_statement_end());

  for (auto seq : *v) {
    // node это либо op_var $a (когда нет дефолтного значения), либо op_set(op_var $a, init_val)
    VertexPtr node = seq.as<op_static>()->args()[0];
    switch (node->type()) {
      case op_var: {
        cur_class->members.add_static_field(node.as<op_var>(), VertexPtr{}, modifiers, phpdoc_str);
        break;
      }
      case op_set: {
        auto set_expr = node.as<op_set>();
        kphp_error_act(set_expr->lhs()->type() == op_var, "unexpected expression in 'static'", break);
        cur_class->members.add_static_field(set_expr->lhs().as<op_var>(), set_expr->rhs(), modifiers, phpdoc_str);
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
    case tok_unset: {
      auto res = get_multi_call<op_unset>(&GenTree::get_expression, true);
      CE (check_statement_end());
      if (res->size() == 1) {
        return res->args()[0];
      }
      return res;
    }
    case tok_var_dump: {
      auto res = get_multi_call<op_func_call>(&GenTree::get_expression, true);
      for (VertexPtr x : res->args()) {
        x.as<op_func_call>()->str_val = "var_dump";
      }
      CE (check_statement_end());
      if (res->size() == 1) {
        return res->args()[0];
      }
      return res;
    }
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
    case tok_abstract:
      if (cur_function->type == FunctionData::func_class_holder) {
        return get_class_member(phpdoc_str);
      } else if (vk::any_of_equal(cur->type(), tok_final, tok_abstract)) {
        return get_class(phpdoc_str, ClassType::klass);
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
    case tok_echo: {
      auto res = get_multi_call<op_func_call>(&GenTree::get_expression);
      for (VertexPtr x : res->args()) {
        x.as<op_func_call>()->str_val = "echo";
      }
      CE (check_statement_end());
      return res;
    }
    case tok_dbg_echo: {
      auto res = get_multi_call<op_func_call>(&GenTree::get_expression);
      for (VertexPtr x : res->args()) {
        x.as<op_func_call>()->str_val = "dbg_echo";
      }
      CE (check_statement_end());
      return res;
    }
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
        return get_class_member(phpdoc_str);
      }
      return get_function(phpdoc_str, FunctionModifiers::nonmember());

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
      auto try_vertex = VertexAdaptor<op_try>::create(embrace(first_node), second_node.as<op_var>(), embrace(third_node));
      set_location(try_vertex, try_location);
      return try_vertex;
    }
    case tok_inline_html: {
      auto html_code = VertexAdaptor<op_string>::create();
      set_location(html_code, AutoLocation(this));
      html_code->str_val = static_cast<string>(cur->str_val);

      auto echo_cmd = VertexAdaptor<op_func_call>::create(html_code);
      echo_cmd->str_val = "print";
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
      if (cur_class) {
        get_traits_uses();
      } else {
        kphp_error(cur_function && cur_function->type == FunctionData::func_global, "'use' can be declared only in global scope");
        get_use();
      }
      return VertexAdaptor<op_empty>::create();
    }
    case tok_var: {
      next_cur();
      get_instance_var_list(phpdoc_str, FieldModifiers{}.set_public());
      CE (check_statement_end());
      auto empty = VertexAdaptor<op_empty>::create();
      return empty;
    }
    case tok_class:
      return get_class(phpdoc_str, ClassType::klass);
    case tok_interface:
      return get_class(phpdoc_str, ClassType::interface);
    case tok_trait:
      return get_class(phpdoc_str, ClassType::trait);
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
        res->type_rule = get_type_rule();
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

VertexPtr GenTree::get_instance_var_list(const vk::string_view &phpdoc_str, FieldModifiers modifiers) {
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

  cur_class->members.add_instance_field(var, def_val, modifiers, phpdoc_str);

  if (test_expect(tok_comma)) {
    next_cur();
    get_instance_var_list(phpdoc_str, modifiers);
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
  auto v_func_params = VertexAdaptor<op_func_param_list>::create();
  auto root = VertexAdaptor<op_function>::create(v_func_params, VertexAdaptor<op_seq>{});
  set_location(root, AutoLocation{this});

  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, FunctionData::create_function(processing_file->main_func_name, root, FunctionData::func_global));
  processing_file->main_function = cur_function;

  if (test_expect(tok_declare)) {
    parse_declare_at_top_of_file();
  }
  if (test_expect(tok_namespace)) {
    parse_namespace_and_uses_at_top_of_file();
  }

  AutoLocation seq_location(this);
  std::vector<VertexPtr> seq_next;
  auto set_run_flag = VertexAdaptor<op_set>::create(processing_file->get_main_func_run_var(), VertexAdaptor<op_true>::create());
  seq_next.push_back(set_run_flag);
  get_seq(seq_next);
  seq_next.push_back(VertexAdaptor<op_return>::create());
  cur_function->root->cmd_ref() = VertexAdaptor<op_seq>::create(seq_next);
  set_location(cur_function->root->cmd(), seq_location);

  G->register_and_require_function(cur_function, parsed_os, true);  // global функция — поэтому required
  require_lambdas();

  if (cur != end) {
    fmt_fprintf(stderr, "line {}: something wrong\n", line_num);
    kphp_error (0, "Cannot compile (probably problems with brace balance)");
  }
}

void php_gen_tree(vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os) {
  GenTree gen(std::move(tokens), file, os);
  gen.run();
}

#undef CE
