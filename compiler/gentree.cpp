// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/gentree.h"

#include "common/algorithms/contains.h"
#include "common/algorithms/find.h"

#include "common/php-functions.h"
#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/lambda-utils.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/stage.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "compiler/vertex-util.h"

#define CE(x) if (!(x)) {return {};}

GenTree::GenTree(std::vector<Token> tokens, SrcFilePtr file, DataStream<FunctionPtr> &os) :
  tokens(std::move(tokens)),
  parsed_os(os),
  cur(this->tokens.begin()),
  end(this->tokens.end()),
  processing_file(file) {                  // = stage::get_file()

  kphp_assert (cur != end);
  end--;
  kphp_assert (end->type() == tok_end);

  line_num = cur->line_num;
  stage::set_line(line_num);
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
  auto location = auto_location();

  if (cur->type() != tok_var_name) {
    return {};
  }
  auto var = VertexAdaptor<op_var>::create();
  var->str_val = static_cast<std::string>(cur->str_val);

  next_cur();
  return var.set_location(location);
}

VertexPtr GenTree::get_foreach_value() {
  if (cur->type() == tok_list) {
    return get_func_call<op_list_ce, op_lvalue_null>();
  }
  if (cur->type() == tok_opbrk) {
    return get_short_array();
  }
  return get_var_name_ref();
}

VertexAdaptor<op_var> GenTree::get_function_use_var_name_ref() {
  auto result = get_var_name_ref();
  kphp_error(result, fmt_format("function use list: expected varname, found {}", cur->str_val));
  kphp_error(!result->ref_flag, "references to variables in `use` block are forbidden in lambdas");
  return result;
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
  // consider the phpdocs in unexpected locations like array or return to be an ordinary comment
  // (usually such phpdocs contain @see or some text, they don't have @var annotations)
  while (cur->type() == tok_phpdoc) {
    //kphp_error(cur->str_val.find("@var") == std::string::npos, "@var would not be analyzed");
    next_cur();
  }
  // phpdoc comments that need to be analyzed don't come here: see op_phpdoc_var
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

        if constexpr (EmptyOp != op_none && EmptyOp != op_err) {
          v = VertexAdaptor<EmptyOp>::create();
        }
      } else if (prev_delim) {
        // TODO: do not emit this error for funcs like var_name_ref() as
        // they return falsy vertex in case of the parse failure.
        // If we give "expecting something after ," error it will be
        // misleading as there might be something after a comma,
        // we just happen to get a failed parsing.
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

  if (EmptyOp == op_lvalue_null && !res->empty() && res->back()->type() == op_lvalue_null) {
    res->pop_back();
  }

  return true;
}

template<Operation Op>
VertexAdaptor<Op> GenTree::get_conv() {
  auto location = auto_location();
  next_cur();
  VertexPtr converted_expression = get_expression();
  CE (!kphp_error(converted_expression, "get_conv failed"));
  return VertexAdaptor<Op>::create(converted_expression).set_location(location);
}

VertexAdaptor<op_require> GenTree::get_require(bool once) {
  auto location = auto_location();
  next_cur();
  const bool is_opened = open_parent();
  auto require = VertexAdaptor<op_require>::create(GenTree::get_expression());
  require->once = once;
  require->builtin = processing_file->is_from_functions_file;
  if (is_opened) {
    CE(expect(tok_clpar, "')'"));
  }
  return require.set_location(location);
}

template<Operation Op, Operation EmptyOp>
VertexAdaptor<Op> GenTree::get_func_call() {
  auto location = auto_location();
  std::string name{cur->str_val};
  next_cur();

  GenericsInstantiationPhpComment *commentTs{nullptr};
  if constexpr (Op == op_func_call) {
    if (test_expect(tok_commentTs)) {   // f/*<...>*/(args)
      commentTs = parse_php_commentTs(cur->str_val);
      next_cur();
    }
  }

  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  std::vector<VertexPtr> next;
  bool ok_next = gen_list<EmptyOp>(&next, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_next, "get argument list failed"));
  CE (expect(tok_clpar, "')'"));

  auto call = VertexAdaptor<Op>::create_vararg(next).set_location(location);

  if (call->has_get_string()) {
    call->set_string(name);
  }
  if constexpr (Op == op_func_call) {
    if (commentTs != nullptr) {
      call->reifiedTs = new GenericsInstantiationMixin(call->location);
      call->reifiedTs->commentTs = commentTs;
      cur_function->has_commentTs_inside = true;
    }
  }
  return call;
}

VertexAdaptor<op_array> GenTree::get_short_array() {
  auto location = auto_location();
  next_cur();

  std::vector<VertexPtr> arr_elements;
  bool ok_next = gen_list<op_lvalue_null>(&arr_elements, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_next, "get short array failed"));
  CE (expect(tok_clbrk, "']'"));

  return VertexAdaptor<op_array>::create(arr_elements).set_location(location);
}


VertexAdaptor<op_string> GenTree::get_string() {
  auto str = VertexAdaptor<op_string>::create().set_location(auto_location());
  str->str_val = static_cast<std::string>(cur->str_val);
  next_cur();
  return str;
}

VertexAdaptor<op_string_build> GenTree::get_string_build() {
  auto sb_location = auto_location();
  next_cur();

  std::vector<VertexPtr> strings;
  bool after_simple_expression = false;
  while (cur != end && cur->type() != tok_str_end) {
    CE (vk::any_of_equal(cur->type(), tok_str, tok_expr_begin)); // make sure we handle all possible tokens
    if (cur->type() == tok_str) {
      strings.push_back(get_string());
      if (after_simple_expression) {
        auto last = strings.back().as<op_string>();
        if (!last->str_val.empty() && last->str_val[0] == '[') {
          kphp_warning("Simple string expressions with [] can work wrong. Use more {}");
        }
      }
      after_simple_expression = false;
    } else if (cur->type() == tok_expr_begin) {
      // simple expressions produce artificial tok_expr_begin/tok_expr_end without
      // having associated '{' and '}' inside the source code
      after_simple_expression = cur->debug_str.empty();
      next_cur();

      VertexPtr add = get_expression();
      CE (!kphp_error(add, "Bad expression in string"));
      strings.push_back(add);

      CE (expect(tok_expr_end, "'}'"));
    }
  }
  CE (expect(tok_str_end, "'\"'"));
  return VertexAdaptor<op_string_build>::create(strings).set_location(sb_location);
}

VertexPtr GenTree::get_postfix_expression(VertexPtr res, bool parenthesized) {
  //postfix operators x++, x--, x[] and x{}, x->y, x()
  bool need = true;
  while (need && cur != end) {
    auto op = cur;
    TokenType tp = op->type();
    need = false;

    if (tp == tok_inc) {
      auto v = VertexAdaptor<op_postfix_inc>::create(res).set_location(auto_location());
      res = v;
      need = true;
      next_cur();
      CE (!kphp_error(!parenthesized, "Expected variable, found parenthesized expression"));
    } else if (tp == tok_dec) {
      auto v = VertexAdaptor<op_postfix_dec>::create(res).set_location(auto_location());
      res = v;
      need = true;
      next_cur();
      CE (!kphp_error(!parenthesized, "Expected variable, found parenthesized expression"));
    } else if (tp == tok_opbrk || tp == tok_opbrc) {
      auto location = auto_location();
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
      res.set_location(location);
      need = true;
    } else if (tp == tok_arrow) {
      auto location = auto_location();
      next_cur();
      VertexPtr rhs = get_expr_top(true);
      CE (!kphp_error(rhs, "Failed to parse right argument of '->'"));
      res = process_arrow(res, rhs);
      CE(res);
      res.set_location(location);
      need = true;
    } else if (tp == tok_oppar) {
      auto location = auto_location();
      next_cur();
      skip_phpdoc_tokens();
      std::vector<VertexPtr> next;
      next.emplace_back(res);
      bool ok_next = gen_list<op_none>(&next, &GenTree::get_expression, tok_comma);
      CE (!kphp_error(ok_next, "get argument list failed"));
      CE (expect(tok_clpar, "')'"));

      res = VertexAdaptor<op_invoke_call>::create(next).set_location(location);
      need = true;
    }


  }
  return res;
}

void GenTree::check_and_remove_num_separators(std::string &s) {
  bool was_separator = false;

  for (const char &c : s) {
    if (c == '_') {
      if (was_separator) {
        kphp_error_return(false, "Bad numeric constant, several '_' in a row are prohibited");
      }
      was_separator = true;
    } else {
      was_separator = false;
    }
  }

  // if all ok
  s.erase(std::remove_if(s.begin(), s.end(), [](char symbol) { return symbol == '_'; }), s.end());
}

VertexPtr GenTree::get_op_num_const() {
  auto get_vertex_with_str_val = [this] (auto vertex, std::string val) {
    auto res = decltype(vertex)::create();
    res.set_location(auto_location());
    res->str_val = std::move(val);
    return res;
  };

  if (cur->type() == tok_int_const) {
    return get_vertex_with_str_val(VertexAdaptor<op_int_const>{}, static_cast<std::string>(cur->str_val));
  }

  if (cur->type() == tok_float_const) {
    return get_vertex_with_str_val(VertexAdaptor<op_float_const>{}, static_cast<std::string>(cur->str_val));
  }

  if (cur->type() == tok_int_const_sep) {
    std::string val = static_cast<std::string>(cur->str_val);
    check_and_remove_num_separators(val);
    return get_vertex_with_str_val(VertexAdaptor<op_int_const>{}, val);
  }

  if (cur->type() == tok_float_const_sep) {
    std::string val = static_cast<std::string>(cur->str_val);
    check_and_remove_num_separators(val);
    return get_vertex_with_str_val(VertexAdaptor<op_float_const>{}, val);
  }

  return VertexPtr{};
}

VertexPtr GenTree::get_expr_top(bool was_arrow, const PhpDocComment *phpdoc) {
  auto op = cur;

  VertexPtr res, first_node;
  TokenType type = op->type();

  auto get_vertex_with_str_val = [this] (auto vertex, std::string val) {
    auto res = decltype(vertex)::create();
    res.set_location(auto_location());
    res->str_val = std::move(val);
    return res;
  };

  bool return_flag = true; // whether to stop parsing without trying to parse expr as postfix expr
  bool parenthesized = false;
  switch (type) {
    case tok_line_c: {
      res = get_vertex_with_str_val(VertexAdaptor<op_int_const>{}, std::to_string(stage::get_line()));
      next_cur();
      break;
    }
    case tok_file_c: {
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, processing_file->file_name);
      next_cur();
      break;
    }
    case tok_file_relative_c: {
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, processing_file->relative_file_name);
      next_cur();
      break;
    }
    case tok_class_c: {
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, cur_class ? cur_class->name : "");
      next_cur();
      break;
    }
    case tok_dir_c: {
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, processing_file->file_name.substr(0, processing_file->file_name.rfind('/')));
      next_cur();
      break;
    }
    case tok_method_c: {
      std::string fun_name;
      if (cur_function->is_lambda()) {
        fun_name = "{closure}";
      } else if (!cur_function->is_main_function()) {
        fun_name = cur_class
                   ? cur_class->name + "::" + cur_function->name.substr(cur_function->name.rfind('$') + 1)
                   : cur_function->name;
      }
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, fun_name);
      next_cur();
      break;
    }
    case tok_namespace_c: {
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, processing_file->namespace_name);
      next_cur();
      break;
    }
    case tok_func_c: {
      std::string fun_name;
      if (cur_function->is_lambda()) {
        fun_name = "{closure}";
      } else if (!cur_function->is_main_function()) {
        fun_name = cur_function->name.substr(cur_function->name.rfind('$') + 1);
      }
      res = get_vertex_with_str_val(VertexAdaptor<op_string>{}, fun_name);
      next_cur();
      break;
    }
    case tok_int_const_sep: {
      res = get_op_num_const();
      next_cur();
      break;
    }
    case tok_int_const: {
      res = get_op_num_const();
      next_cur();
      break;
    }
    case tok_nan: {
      res = get_vertex_with_str_val(VertexAdaptor<op_float_const>{}, "NAN");
      next_cur();
      break;
    }
    case tok_inf: {
      res = get_vertex_with_str_val(VertexAdaptor<op_float_const>{}, "std::numeric_limits<double>::infinity()");
      next_cur();
      break;
    }
    case tok_float_const_sep: {
      res = get_op_num_const();
      next_cur();
      break;
    }
    case tok_float_const: {
      res = get_op_num_const();
      next_cur();
      break;
    }
    case tok_null: {
      res = VertexAdaptor<op_null>::create().set_location(auto_location());
      next_cur();
      break;
    }
    case tok_false: {
      res = VertexAdaptor<op_false>::create().set_location(auto_location());
      next_cur();
      break;
    }
    case tok_true: {
      res = VertexAdaptor<op_true>::create().set_location(auto_location());
      next_cur();
      break;
    }
    case tok_var_name: {
      res = get_var_name();
      if (cur->type() == tok_double_colon) {  // $class::method(), $class::$field, $class::CONST
        res = get_member_by_name_after_var(res.as<op_var>());
      }
      return_flag = false;
      break;
    }
    case tok_varg: {
      bool good_prefix = cur != tokens.begin() && vk::any_of_equal(std::prev(cur)->type(), tok_comma, tok_oppar, tok_opbrk);
      CE (!kphp_error(good_prefix, "It's not allowed using `...` in this place"));

      next_cur();
      res = get_expression();
      // since the argument for the spread operator can be anything,
      // we do not check the type of the expression here
      res = VertexAdaptor<op_varg>::create(res).set_location(res);
      break;
    }
    case tok_str:
      res = get_string();
      return_flag = false; // string is a valid postfix expr operand
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
    case tok_conv_array:
      res = get_conv<op_conv_array>();
      break;

    case tok_print: {
      auto location = auto_location();
      next_cur();
      first_node = get_expression();
      CE (!kphp_error(first_node, "Failed to get print argument"));
      auto print = VertexAdaptor<op_func_call>::create(first_node).set_location(location);
      print->str_val = "print";
      res = print;
      break;
    }
    case tok_require:
      res = get_require(false);
      break;
    case tok_require_once:
      res = get_require(true);
      break;

    case tok_new: {
      next_cur();
      if (test_expect(tok_func_name)) {   // 'new A()' / 'new \Some\Class($args)'
        auto func_call = get_func_call<op_func_call, op_none>();
        // Hack to be more compatible with php
        if (func_call->str_val == "Memcache") {
          func_call->set_string("McMemcache");
        }
        res = gen_constructor_call_with_args(func_call->str_val, func_call->get_next(), func_call->location);
        CE(res);
      } else if (test_expect(tok_var_name)) {   // 'new $class_name' / 'new $class_name(...$args)'
        res = get_by_name_construct();
        CE(res);
      } else {
        CE(!kphp_error(0, "Expected class name after new"));
      }
      break;
    }
    case tok_func_name: {
      cur++;
      if (!test_expect(tok_oppar) && !test_expect(tok_commentTs)) {
        if (!was_arrow && vk::any_of_equal(op->str_val, "die", "exit")) { // can be called without "()"
          res = get_vertex_with_str_val(VertexAdaptor<op_func_call>{}, static_cast<std::string>(op->str_val));
        } else {
          res = get_vertex_with_str_val(VertexAdaptor<op_func_name>{}, static_cast<std::string>(op->str_val));
        }
        return_flag = was_arrow;
        break;
      }
      cur--;

      // we don't support namespaces for functions yet, but we
      // permit \func() syntax and interpret it as func();
      // this logic is compatible with what we'll get after the
      // function namespaces will be implemented
      auto func_call = get_func_call<op_func_call, op_none>();
      if (func_call && func_call->str_val[0] == '\\' && !vk::contains(func_call->str_val, "::")) {
        func_call->str_val.erase(0, 1);
      }

      res = func_call;
      return_flag = was_arrow;
      break;
    }
    case tok_static:
      next_cur();
      res = get_lambda_function(phpdoc, FunctionModifiers::static_lambda());
      break;
    case tok_function:
    case tok_fn:
      res = get_lambda_function(phpdoc, FunctionModifiers::nonmember());
      break;
    case tok_phpdoc: {      // /** ... */ before expression (not before a statement)
      vk::string_view phpdoc_str = cur->str_val;
      next_cur();
      return get_expr_top(was_arrow, new PhpDocComment(phpdoc_str));
    }
    case tok_isset: {
      auto temp = get_multi_call<op_isset, op_none>(&GenTree::get_expression, true);
      CE (!kphp_error(temp->size(), "isset function requires at least one argument"));
      res = VertexPtr{};
      for (auto right : temp->args()) {
        res = res ? VertexPtr(VertexAdaptor<op_log_and>::create(res, right)) : right;
      }
      res.set_location(temp->location);
      break;
    }
    case tok_declare:
      // see GenTree::parse_declare_at_top_of_file
      kphp_error(0, "strict_types declaration must be the very first statement in the script");
      break;
    case tok_array:
      res = get_func_call<op_array, op_none>();
      return_flag = false; // array is valid postfix expression operand
      break;
    case tok_tuple:
      res = get_func_call<op_tuple, op_none>();
      CE (!kphp_error(res.as<op_tuple>()->size(), "tuple() must have at least one argument"));
      break;
    case tok_shape:
      res = get_shape();
      break;
    case tok_opbrk:
      res = get_short_array();
      return_flag = false; // array is valid postfix expression operand
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
      return_flag = false; // expression inside '()' is valid postfix expression operand
      parenthesized = true;
      break;
    case tok_str_begin:
      res = get_string_build();
      break;
    case tok_clone: {
      next_cur();
      res = VertexAdaptor<op_clone>::create(get_expr_top(false)).set_location(auto_location());
      break;
    }
    default:
      return {};
  }

  if (return_flag) {
    return res;
  }

  return get_postfix_expression(res, parenthesized);
}

VertexAdaptor<op_ternary> GenTree::create_ternary_op_vertex(VertexPtr condition, VertexPtr true_expr, VertexPtr false_expr) {
  if (true_expr) {
    return VertexAdaptor<op_ternary>::create(condition, true_expr, false_expr);
  }

  auto cond_var = VertexUtil::create_superlocal_var("shorthand_ternary_cond", cur_function).set_location(condition);

  auto cond = VertexUtil::create_conv_to(tp_bool, VertexAdaptor<op_set>::create(cond_var, condition));

  auto left_var_move = VertexAdaptor<op_move>::create(cond_var.clone());
  return VertexAdaptor<op_ternary>::create(cond, left_var_move, false_expr);
}

VertexPtr GenTree::get_unary_op(int op_priority_cur, Operation unary_op_tp, bool till_ternary) {
  auto location = auto_location();
  next_cur();

  VertexPtr left = get_binary_op(op_priority_cur, till_ternary);
  if (!left) {
    return {};
  }

  if (unary_op_tp == op_log_not) {
    left = VertexUtil::create_conv_to(tp_bool, left);
  }
  if (unary_op_tp == op_not) {
    left = VertexUtil::create_conv_to(tp_int, left);
  }
  VertexPtr expr = create_vertex(unary_op_tp, left).set_location(location);

  if (expr->type() == op_minus || expr->type() == op_plus) {
    VertexPtr maybe_num = expr.as<meta_op_unary>()->expr();
    if (auto num = maybe_num.try_as<meta_op_num>()) {
      // keep the +N as is, but turn -N into a constant (but if N starts with minus, then make it "N" so we can parse `- - -7`)
      if (expr->type() == op_minus) {
        num->str_val = num->str_val[0] == '-' ? num->str_val.substr(1) : "-" + num->str_val;
      }
      return num;
    }
  }

  return expr;
}

TokenType transform_compare_operation_token(TokenType token) noexcept {
  switch (token) {
    case tok_gt:
      return tok_lt;
    case tok_ge:
      return tok_le;
    case tok_neq_lg:
    case tok_neq2:
      return tok_eq2;
    case tok_neq3:
      return tok_eq3;
    default:
      return token;
  }
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
    const TokenType origin_token = cur->type();
    const TokenType token = transform_compare_operation_token(origin_token);

    const Operation binary_op_tp = OpInfo::tok_to_binary_op[token];
    if (binary_op_tp == op_err || OpInfo::priority(binary_op_tp) != op_priority_cur) {
      break;
    }

    auto expr_location = auto_location();
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
        left = VertexUtil::create_conv_to(tp_bool, left);
      }
    }

    if (vk::any_of_equal(binary_op_tp, op_log_or, op_log_and, op_log_or_let, op_log_and_let, op_log_xor_let)) {
      left = VertexUtil::create_conv_to(tp_bool, left);
      right = VertexUtil::create_conv_to(tp_bool, right);
    }
    if (vk::any_of_equal(binary_op_tp, op_set_or, op_set_and, op_set_xor, op_set_shl, op_set_shr)) {
      right = VertexUtil::create_conv_to(tp_int, right);
    }
    if (vk::any_of_equal(binary_op_tp, op_or, op_and, op_xor)) {
      left = VertexUtil::create_conv_to(tp_int, left);
      right = VertexUtil::create_conv_to(tp_int, right);
    }

    if (vk::any_of_equal(origin_token, tok_gt, tok_ge)) {
      std::swap(left, right);
    }

    if (ternary) {
      left = create_ternary_op_vertex(left, right, third);
    } else {
      left = create_vertex(binary_op_tp, left, right);
    }

    left.set_location(expr_location);
    if (vk::any_of_equal(origin_token, tok_neq2, tok_neq_lg, tok_neq3)) {
      left = VertexAdaptor<op_log_not>::create(left).set_location(expr_location);
    }

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

VertexPtr GenTree::get_def_value() {
  VertexPtr val;

  if (cur->type() == tok_eq1) {
    next_cur();
    val = get_expression();
    kphp_error (val, "Cannot parse function parameter");
  }

  return val;
}

VertexAdaptor<op_func_param> GenTree::get_func_param() {
  auto location = auto_location();

  const TypeHint *type_hint = get_typehint();
  bool is_varg = false;

  // if the argument is vararg and has a type hint — e.g. int ...$a — then cur points to $a, as ... were consumed by the type lexer
  if (type_hint && std::prev(cur, 1)->type() == tok_varg) {
    is_varg = true;
  } else if (test_expect(tok_varg)) {
    next_cur();
    is_varg = true;
  }
  if (is_varg) {
    kphp_error(!cur_function->has_variadic_param, "Function can not have ...$variadic more than once");
    cur_function->has_variadic_param = true;
  }

  VertexAdaptor<op_var> name = get_var_name_ref();
  if (!name) {
    kphp_error(!type_hint, "Syntax error: missing varname after typehint");
    return {};
  }

  bool is_cast_param = false;
  if (cur->type() == tok_triple_colon) {  // $x ::: string — a cast param
    is_cast_param = true;
    next_cur();
    type_hint = get_typehint();
  }

  VertexPtr def_val = get_def_value();
  VertexAdaptor<op_func_param> v;
  if (def_val) {
    kphp_error(!is_varg, "Variadic argument can not have a default value");
    v = VertexAdaptor<op_func_param>::create(name, def_val).set_location(location);
  } else {
    v = VertexAdaptor<op_func_param>::create(name).set_location(location);
  }
  if (is_varg) {
    v->extra_type = op_ex_param_variadic;
  }

  if (type_hint) {
    // if "T $a = null" (default argument null), then type of $a is ?T (strange, but PHP works this way)
    if (def_val && def_val->type() == op_null) {
      type_hint = TypeHintOptional::create(type_hint, true, false);
    }
    v->type_hint = type_hint;
  }
  v->is_cast_param = is_cast_param;

  return v;
}

std::pair<VertexAdaptor<op_foreach_param>, VertexPtr> GenTree::get_foreach_param() {
  auto location = auto_location();
  VertexPtr array_expression = get_expression();
  CE (!kphp_error(array_expression, ""));

  CE (expect(tok_as, "'as'"));
  skip_phpdoc_tokens();

  VertexAdaptor<op_var> key;
  VertexAdaptor<op_var> value;
  VertexPtr value_expr = get_foreach_value();
  VertexPtr list;

  // This error message is given for both key/value parts
  // as we can't easily distinguish $k=>$v from just $v if
  // get_var_name_ref failed to parse a variable.
  auto tok = *cur;
  CE (!kphp_error(value_expr, fmt_format("Expected a var name, ref or list, found {}", tok.str_val)));

  if (vk::any_of_equal(value_expr->type(), op_list_ce, op_array)) {
    value = VertexUtil::create_superlocal_var("list", cur_function);
    list = value_expr;
  } else {
    value = value_expr.as<op_var>();
  }

  if (cur->type() == tok_double_arrow) {
    next_cur();
    key = value;
    tok = *cur;
    value = get_var_name_ref();
    CE (!kphp_error(value, fmt_format("Expected a var name, ref or list as a foreach value, found {}", tok.str_val)));
  }

  VertexPtr temp_var;
  if (value->ref_flag) {
    temp_var = VertexAdaptor<op_empty>::create();
  } else {
    temp_var = VertexUtil::create_superlocal_var("tmp_expr", cur_function);
  }

  auto param = key ? VertexAdaptor<op_foreach_param>::create(array_expression, value, temp_var, key).set_location(location)
                   : VertexAdaptor<op_foreach_param>::create(array_expression, value, temp_var).set_location(location);
  return {param, list};
}

VertexPtr GenTree::get_call_arg_for_param(VertexAdaptor<op_func_call> call, VertexAdaptor<op_func_param> param, int param_i) {
  if (param_i < call->args().size()) {
    return call->args()[param_i];
  }
  if (param->has_default_value()) {
    return param->default_value();
  }
  return {};
}

template<Operation Op, Operation EmptyOp, class FuncT, class ResultType>
VertexAdaptor<op_seq> GenTree::get_multi_call(FuncT &&f, bool parenthesis) {
  auto location = auto_location();
  next_cur();

  if (parenthesis) {
    CE (expect(tok_oppar, "'('"));
  }

  std::vector<ResultType> next;
  bool ok_next = gen_list<EmptyOp>(&next, f, tok_comma);
  CE (!kphp_error(ok_next, "Failed get_multi_call"));
  if (parenthesis) {
    CE (expect(tok_clpar, "')'"));
  }

  std::vector<VertexAdaptor<Op>> new_next;
  new_next.reserve(next.size());

  for (VertexPtr next_elem : next) {
    new_next.emplace_back(VertexAdaptor<Op>::create(next_elem).set_location(next_elem));
  }
  return VertexAdaptor<op_seq>::create(new_next).set_location(location);
}


VertexAdaptor<op_return> GenTree::get_return() {
  auto location = auto_location();
  next_cur();
  skip_phpdoc_tokens();
  VertexPtr return_val = get_expression();
  if (!return_val && cur_function->is_main_function()) {
    return_val = VertexAdaptor<op_null>::create();
  }

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
  CE (expect(tok_semicolon, "';'"));
  return ret.set_location(location);
}


template<Operation Op>
VertexAdaptor<Op> GenTree::get_break_or_continue() {
  auto location = auto_location();
  next_cur();
  // https://www.php.net/manual/en/control-structures.continue.php
  auto level_of_enclosing_loops_to_skip = get_expression();
  CE (expect(tok_semicolon, "';'"));

  if (!level_of_enclosing_loops_to_skip) {
    auto one = VertexUtil::create_int_const(1);
    level_of_enclosing_loops_to_skip = one;
  }

  return VertexAdaptor<Op>::create(level_of_enclosing_loops_to_skip).set_location(location);
}

VertexAdaptor<op_foreach> GenTree::get_foreach() {
  auto foreach_location = auto_location();
  next_cur();

  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();

  auto [foreach_param, foreach_list] = get_foreach_param();

  CE (!kphp_error(foreach_param, "Failed to parse 'foreach' params"));
  CE (expect(tok_clpar, "')'"));

  VertexPtr body = get_statement();
  CE (!kphp_error(body, "Failed to parse 'foreach' body"));

  // If foreach value is list expression, we rewrite the body
  // to include the list assignment as a first statement.
  if (foreach_list) {
    auto list_assign = VertexAdaptor<op_set>::create(foreach_list, foreach_param->x().clone()).set_location(foreach_location);
    body = VertexAdaptor<op_seq>::create(list_assign, body).set_location(foreach_location);
  }

  return VertexAdaptor<op_foreach>::create(foreach_param, VertexUtil::embrace(body)).set_location(foreach_location);
}

VertexAdaptor<op_while> GenTree::get_while() {
  auto while_location = auto_location();
  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr condition = get_expression();
  CE (!kphp_error(condition, "Failed to parse 'while' condition"));
  condition = VertexUtil::create_conv_to(tp_bool, condition);
  CE (expect(tok_clpar, "')'"));

  VertexPtr body = get_statement();
  CE (!kphp_error(body, "Failed to parse 'while' body"));

  return VertexAdaptor<op_while>::create(condition, VertexUtil::embrace(body)).set_location(while_location);
}

VertexAdaptor<op_if> GenTree::get_if() {
  auto if_location = auto_location();
  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  VertexPtr condition = get_expression();
  CE (!kphp_error(condition, "Failed to parse 'if' condition"));
  condition = VertexUtil::create_conv_to(tp_bool, condition);
  CE (expect(tok_clpar, "')'"));

  VertexPtr true_cmd = get_statement();
  CE (!kphp_error(true_cmd, "Failed to parse 'if' body"));

  if (cur->type() == tok_else) {
    next_cur();
    auto false_cmd = get_statement();
    CE (!kphp_error(false_cmd, "Failed to parse 'else' statement"));
    return VertexAdaptor<op_if>::create(condition, VertexUtil::embrace(true_cmd), VertexUtil::embrace(false_cmd)).set_location(if_location);
  }

  return VertexAdaptor<op_if>::create(condition, VertexUtil::embrace(true_cmd)).set_location(if_location);
}

VertexAdaptor<op_for> GenTree::get_for() {
  auto for_location = auto_location();
  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();

  auto pre_cond_location = auto_location();
  std::vector<VertexPtr> init_statements;
  bool ok_first_next = gen_list<op_err>(&init_statements, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_first_next, "Failed to parse 'for' precondition"));
  auto initialization = VertexAdaptor<op_seq>::create(init_statements).set_location(pre_cond_location);

  CE (expect(tok_semicolon, "';'"));

  auto cond_location = auto_location();
  std::vector<VertexPtr> condition_expressions;
  bool ok_second_next = gen_list<op_err>(&condition_expressions, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_second_next, "Failed to parse 'for' action"));
  if (condition_expressions.empty()) {
    condition_expressions.push_back(VertexAdaptor<op_true>::create());
  } else {
    condition_expressions.back() = VertexUtil::create_conv_to(tp_bool, condition_expressions.back());
  }
  auto condition = VertexAdaptor<op_seq_comma>::create(condition_expressions).set_location(cond_location);

  CE (expect(tok_semicolon, "';'"));

  auto iteration_location = auto_location();
  std::vector<VertexPtr> iteration_expressions;
  bool ok_third_next = gen_list<op_err>(&iteration_expressions, &GenTree::get_expression, tok_comma);
  CE (!kphp_error(ok_third_next, "Failed to parse 'for' postcondition"));
  auto iteration = VertexAdaptor<op_seq>::create(iteration_expressions).set_location(iteration_location);

  CE (expect(tok_clpar, "')'"));

  VertexPtr cmd = get_statement();
  CE (!kphp_error(cmd, "Failed to parse 'for' statement"));

  return VertexAdaptor<op_for>::create(initialization, condition, iteration, VertexUtil::embrace(cmd)).set_location(for_location);
}

VertexAdaptor<op_do> GenTree::get_do() {
  auto location = auto_location();
  next_cur();
  VertexPtr body = get_statement();
  CE (!kphp_error(body, "Failed to parser 'do' statement"));

  CE (expect(tok_while, "'while'"));
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();

  VertexPtr condition = get_expression();

  CE (!kphp_error(condition, "Failed to parse 'do' condition"));
  condition = VertexUtil::create_conv_to(tp_bool, condition);
  CE (expect(tok_clpar, "')'"));
  CE (expect(tok_semicolon, "';'"));

  return VertexAdaptor<op_do>::create(VertexUtil::embrace(body), condition).set_location(location);
}

VertexAdaptor<op_switch> GenTree::get_switch() {
  auto location = auto_location();

  next_cur();
  CE (expect(tok_oppar, "'('"));
  skip_phpdoc_tokens();
  auto switch_condition = get_expression();
  CE (!kphp_error(switch_condition, "Failed to parse 'switch' expression"));
  CE (expect(tok_clpar, "')'"));

  CE (expect(tok_opbrc, "'{'"));

  std::vector<VertexPtr> cases;
  while (cur->type() != tok_clbrc) {
    skip_phpdoc_tokens();
    auto cur_type = cur->type();
    VertexPtr case_val;

    auto case_location = auto_location();
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

    auto seq_location = auto_location();
    std::vector<VertexPtr> body_statements;
    while (cur != end && vk::none_of_equal(cur->type(), tok_clbrc, tok_case, tok_default)) {
      if (auto cmd = get_statement()) {
        body_statements.push_back(cmd);
      }
    }

    auto seq = VertexAdaptor<op_seq>::create(body_statements).set_location(seq_location);
    if (cur_type == tok_case) {
      cases.emplace_back(VertexAdaptor<op_case>::create(case_val, seq));
    } else {
      cases.emplace_back(VertexAdaptor<op_default>::create(seq));
    }
    cases.back().set_location(case_location);
  }
  CE (expect(tok_clbrc, "'}'"));

  return VertexUtil::create_switch_vertex(cur_function, switch_condition.set_location(location), std::move(cases)).set_location(location);
}

VertexAdaptor<op_shape> GenTree::get_shape() {
  auto location = auto_location();

  next_cur();
  CE (expect(tok_oppar, "'('"));
  VertexPtr inner_array = get_expression();
  CE (expect(tok_clpar, "')'"));
  CE (!kphp_error(inner_array && inner_array->type() == op_array && !inner_array->empty(), "shape() must have an array associative non-empty inside"));

  std::set<int64_t> keys_hashes;
  for (VertexPtr arr_elem : inner_array.as<op_array>()->args()) {
    auto double_arrow = arr_elem.try_as<op_double_arrow>();
    CE (!kphp_error(double_arrow, "shape() must have an array associative non-empty inside"));
    auto key_v = double_arrow->lhs();
    CE (!kphp_error(vk::any_of_equal(key_v->type(), op_int_const, op_string, op_func_name), "keys of shape() must be constant"));
    const std::string &key_str = key_v->get_string();
    const std::int64_t hash = string_hash(key_str.data(), key_str.size());
    CE (!kphp_error(keys_hashes.insert(hash).second, "keys of shape() are not unique"));
  }

  // shape->args() is a sequence of op_double_arrow
  return VertexAdaptor<op_shape>::create(inner_array.as<op_array>()->args()).set_location(location);
}

// `new $c` / `new $c()` / `new $c(a, r, g, s)` / `new $c(...$args)`
// convert into `_by_name_construct($c, ...)`
// later, if $c is constexpr (class-string<T> for example), this call will be replaced with `new T(...)`
// see replace-extern-func-calls.cpp
VertexPtr GenTree::get_by_name_construct() {
  kphp_assert(cur->type() == tok_var_name);  // cur is $c
  auto v_class_name = VertexAdaptor<op_var>::create().set_location(auto_location());
  v_class_name->str_val = static_cast<std::string>(cur->str_val); // 'c'

  std::vector<VertexPtr> args = {v_class_name};

  if ((cur + 1)->type() == tok_oppar) { // `new $c(...)`
    auto dummy_call = get_func_call<op_func_call, op_none>();
    if (dummy_call) {
      args.insert(args.end(), dummy_call->begin(), dummy_call->end());
    }
  } else {  // `new $c`
    next_cur();
  }

  auto v_by_name = VertexAdaptor<op_func_call>::create(args).set_location(auto_location());
  v_by_name->str_val = "_by_name_construct";
  v_by_name->auto_inserted = true;
  return v_by_name;
}

// `$c::...`, expected to be `$c::method(...)` / `$c::CONST` / `$c::$field`
// convert into `_by_name_call_method($c, 'method', ...)`, etc.
// later, if $c is constexpr, this calls will be replaced with `T::method(...)` / `T::CONST` / `T::$field`
// see replace-extern-func-calls.cpp
VertexPtr GenTree::get_member_by_name_after_var(VertexAdaptor<op_var> v_before) {
  kphp_assert(cur->type() == tok_double_colon);
  next_cur();

  // $c::method(...) -> _by_name_call_method($c, 'method', ...)
  if (cur->type() == tok_func_name && (cur+1)->type() == tok_oppar) {
    auto v_method_name = VertexAdaptor<op_string>::create().set_location(auto_location());
    v_method_name->str_val = static_cast<std::string>(cur->str_val);

    auto dummy_call = get_func_call<op_func_call, op_none>();
    std::vector<VertexPtr> args = {v_before, v_method_name};
    if (dummy_call) {   // 'if' not to fail on unclosed '('
      args.insert(args.end(), dummy_call->begin(), dummy_call->end());
    }

    auto v_by_name = VertexAdaptor<op_func_call>::create(args).set_location(auto_location());
    v_by_name->str_val = "_by_name_call_method";
    v_by_name->auto_inserted = true;
    return v_by_name;
  }

  // $c::CONST -> _by_name_get_const($c, 'CONST')
  // $c::CONST[0] and similar also supported
  if (cur->type() == tok_func_name && (cur+1)->type() != tok_oppar) {
    auto v_const_name = VertexAdaptor<op_string>::create().set_location(auto_location());
    v_const_name->str_val = static_cast<std::string>(cur->str_val);

    next_cur();
    std::vector<VertexPtr> args = {v_before, v_const_name};

    auto v_by_name = VertexAdaptor<op_func_call>::create(args).set_location(auto_location());
    v_by_name->str_val = "_by_name_get_const";
    v_by_name->auto_inserted = true;
    return v_by_name;
  }

  // $c::$field -> _by_name_get_field($c, 'field')
  // $c::$field[0] += and similar also supported
  // note, that for $c::$var(, we fire an error, because it's $c::$method(), an unsupported dynamic call by name
  if (cur->type() == tok_var_name) {
    kphp_error_act((cur+1)->type() != tok_oppar, "Syntax '$class_name::$method_name()' is invalid in KPHP.\nProbably, you need switch-case or code generation instead of invocations via dynamic names.", return v_before);

    auto v_field_name = VertexAdaptor<op_string>::create().set_location(auto_location());
    v_field_name->str_val = static_cast<std::string>(cur->str_val);

    next_cur();
    std::vector<VertexPtr> args = {v_before, v_field_name};

    auto v_by_name = VertexAdaptor<op_func_call>::create(args).set_location(auto_location());
    v_by_name->str_val = "_by_name_get_field";
    v_by_name->auto_inserted = true;
    return v_by_name;
  }

  kphp_error(0, fmt_format("Unrecognized syntax after '${}::'", v_before->str_val));
  return v_before;
}

// analyze phpdoc /** ... */ NOT above a function/field/etc, but inside a function
// we search for @var tags inside and convert them to op_phpdoc_var vertices
VertexPtr GenTree::get_phpdoc_inside_function() {
  kphp_assert(cur->type() == tok_phpdoc);
  VertexPtr result;
  vk::string_view phpdoc_str = cur->str_val;

  // there can be several @var directives (for example, a block of @var comments above the list() assignment)
  for (const PhpDocTag &tag: PhpDocComment(phpdoc_str).tags) {
    if (tag.type == PhpDocType::var) {
      // we are able to parse @var in gentree: @kphp-generic was already parsed, and generic T won't mess with class T
      // classes resolving (and error for unknown classes) will happen later, in parse-and-apply-phpdoc.cpp
      auto tag_parsed = tag.value_as_type_and_var_name(cur_function, cur_function->get_this_or_topmost_if_lambda()->genericTs);
      kphp_error_act(tag_parsed, fmt_format("Failed to parse @var inside {}", cur_function->as_human_readable()), break);

      // tag could look like /* @var type $v comment */ or just /* @var type comment */ $v = ...
      auto inner_var = VertexAdaptor<op_var>::create().set_location(auto_location());
      if (!tag_parsed.var_name.empty()) {
        inner_var->str_val = tag_parsed.var_name;
      } else if ((cur + 1)->type() == tok_var_name) {
        inner_var->str_val = static_cast<std::string>((cur + 1)->str_val);
      } else {
        kphp_error(0, "@var with empty var name");
      }

      auto phpdoc_var = VertexAdaptor<op_phpdoc_var>::create(inner_var).set_location(inner_var);
      phpdoc_var->type_hint = tag_parsed.type_hint;

      result = result ? VertexPtr(VertexAdaptor<op_seq>::create(result, phpdoc_var)) : VertexPtr(phpdoc_var);
      cur_function->has_var_tags_inside = true;
    }
  }

  next_cur();   // only here, not at the beginning: in case of phpdoc errors to point to phpdoc
  return result ?: VertexPtr(VertexAdaptor<op_empty>::create());
}

bool GenTree::parse_cur_function_uses() {
  if (!expect(tok_use, "`use`") || !expect(tok_oppar, "`(`")) {
    return false;
  }

  std::vector<VertexAdaptor<op_var>> uses_as_vars;
  gen_list<op_err>(&uses_as_vars, &GenTree::get_function_use_var_name_ref, tok_comma);

  for (auto &v : uses_as_vars) {
    cur_function->uses_list.emplace_front(v);   // the order — use($v1, $v2) or use($v2, $v1) — doesn't matter
  }

  return expect(tok_clpar, "`)`");
}

bool GenTree::test_if_uses_and_arguments_intersect(const std::forward_list<VertexAdaptor<op_var>> &uses_list, const VertexRange &params) {
  return std::any_of(uses_list.begin(), uses_list.end(), [params](auto var_as_use) {
    return std::any_of(params.begin(), params.end(), [var_as_use](auto p) {
      return p.template as<op_func_param>()->var()->str_val == var_as_use->str_val;
    });
  });
}

VertexAdaptor<op_lambda> GenTree::get_lambda_function(const PhpDocComment *phpdoc, FunctionModifiers modifiers) {
  auto v_op_function_lambda = get_function(true, phpdoc, modifiers);
  CE (v_op_function_lambda);

  cur_function->has_lambdas_inside = true;

  auto v_lambda = VertexAdaptor<op_lambda>::create().set_location(v_op_function_lambda);
  v_lambda->func_id = v_op_function_lambda->func_id;
  kphp_assert(v_lambda->func_id->is_lambda());
  return v_lambda;
}

// parsing 'static public', 'final protected' and other function/class members modifiers
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

// get_identifier consumes tok_func_name and returns its string value;
// for classes it can also consume the semi-reserved keywords like "else"
std::string GenTree::get_identifier() {
  bool ok = test_expect(tok_func_name);

  if (!ok && cur_class && !cur->str_val.empty()) {
    // check whether it could be a semi-reserved keyword
    ok = is_alpha(cur->str_val[0]);
  }

  kphp_error(ok, expect_msg("identifier"));

  next_cur();
  return static_cast<std::string>(std::prev(cur)->str_val);
}

VertexPtr GenTree::get_class_member(const PhpDocComment *phpdoc) {
  auto modifiers = parse_class_member_modifier_mask();
  if (!modifiers.is_static()) {
    modifiers.set_instance();
  }

  if (!modifiers.is_public() && !modifiers.is_private() && !modifiers.is_protected()) {
    modifiers.set_public();
  }

  if (test_expect(tok_function)) {
    get_function(false, phpdoc, modifiers);
    return {};
  }

  const TypeHint *type_hint = get_typehint();

  if (test_expect(tok_var_name)) {
    kphp_error(!cur_class->is_interface(), "Interfaces may not include member variables");
    kphp_error(!modifiers.is_final() && !modifiers.is_abstract(), "Class fields can not be declared final/abstract");
    if (modifiers.is_static()) {
      return get_static_field_list(phpdoc, FieldModifiers{modifiers.access_modifier()}, type_hint);
    }
    get_instance_var_list(phpdoc, FieldModifiers{modifiers.access_modifier()}, type_hint);
    return {};
  }

  kphp_error(0, "Expected 'function' or $var_name after class member modifiers");
  next_cur();
  return {};
}

VertexAdaptor<op_func_param_list> GenTree::parse_cur_function_param_list() {
  std::vector<VertexAdaptor<op_func_param>> params_next;

  CE(expect(tok_oppar, "'('"));

  if (cur_function->modifiers.is_instance()) {    // add implicit $this
    params_next.emplace_back(ClassData::gen_param_this(auto_location()));
  }

  bool ok_params_next = gen_list<op_err>(&params_next, &GenTree::get_func_param, tok_comma);
  CE(!kphp_error(ok_params_next, "Failed to parse function params"));
  CE(expect(tok_clpar, "')'"));

  for (size_t i = 1; i < params_next.size(); ++i) {
    if (!params_next[i]->has_default_value()) {
      kphp_error(!params_next[i - 1]->has_default_value(), "Optional parameter is provided before required");
    }
  }

  if (cur_function->is_generic() && cur_function->genericTs->is_variadic()) {
    kphp_error(cur_function->has_variadic_param, "Variadic generic can be used only for a function with ...$variadic parameter");
  }

  return VertexAdaptor<op_func_param_list>::create(params_next).set_location(cur_function->root);
}

VertexAdaptor<op_function> GenTree::get_function(bool is_lambda, const PhpDocComment *phpdoc, FunctionModifiers modifiers) {
  bool is_arrow = test_expect(tok_fn);
  CE(expect2(tok_fn, tok_function, "function"));
  auto func_location = auto_location();

  std::string func_name;

  // a function name is a token that immediately follow a 'function' token (full$$name inside a class)
  if (is_lambda) {
    func_name = gen_anonymous_function_name(cur_function); // cur_function is a parent function here
  } else {
    func_name = get_identifier();
    CE(!func_name.empty());
  }

  // a class/interface method
  if (cur_class && !is_lambda) {
    bool is_magic_method = func_name.size() > 2 && func_name[0] == '_' && func_name[1] == '_';
    kphp_error(!is_magic_method || is_magic_method_name_allowed(func_name), "This magic method is unsupported");
    kphp_error(!(modifiers.is_abstract() && cur_class->is_interface()), fmt_format("abstract methods may not be declared in interfaces: {}", cur_class->name));

    // a function name inside a class is full$class$name$$fname
    func_name = replace_backslashes(cur_class->name) + "$$" + func_name;

    if (cur_class->is_interface()) {
      modifiers.set_abstract();
    }
    if (cur_class->modifiers.is_final()) {
      modifiers.set_final();
    }
  }

  auto func_root = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>::create(), VertexAdaptor<op_seq>::create()).set_location(func_location);
  FunctionData::func_type_t func_type = is_lambda ? FunctionData::func_lambda : processing_file->is_builtin() ? FunctionData::func_extern : FunctionData::func_local;

  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, FunctionData::create_function(func_name, func_root, func_type));
  cur_function->phpdoc = phpdoc;
  cur_function->modifiers = modifiers;
  cur_function->is_internal = processing_file->is_builtin() && processing_file->short_file_name == "kphp_internal_txt";

  if (is_lambda) {
    cur_function->outer_function = functions_stack[functions_stack.size() - 2];
  }

  // we parse @kphp-generic here, in gentree, not in ParseAndApplyPhpdocF pipe,
  // because when all @param/@var will be parsed in parallel, we already should know what classes/functions are generic
  // in other words, we treat <T> in declaration as a language syntax, which is expressed in phpdoc only due to PHP limitations
  // this also helps parse /*<T>*/ in a function body (and in inner lambdas) in gentree, able to differ class T vs generic T
  if (phpdoc && phpdoc->has_tag(PhpDocType::kphp_generic)) {
    stage::set_location(Location{cur_function->file_id, cur_function, func_location.line});
    cur_function->genericTs = GenericsDeclarationMixin::create_for_function_from_phpdoc(cur_function, phpdoc);
  }

  CE(cur_function->root->param_list_ref() = parse_cur_function_param_list());

  if (is_lambda && !is_arrow && test_expect(tok_use)) {
    CE(parse_cur_function_uses());
    kphp_error(!test_if_uses_and_arguments_intersect(cur_function->uses_list, cur_function->get_params()),
               "arguments and captured variables (in `use` clause) must have different names");
  }

  if (test_expect(tok_colon) || test_expect(tok_triple_colon)) {    // function.txt allows ::: besides :
    next_cur();
    cur_function->return_typehint = get_typehint();
    kphp_error(cur_function->return_typehint, "Expected return typehint after :");
  }

  if (cur_function->modifiers.is_instance()) {
    cur_class->members.add_instance_method(cur_function);
  } else if (cur_function->modifiers.is_static()) {
    cur_class->members.add_static_method(cur_function);
  }

  if (is_arrow) {
    CE (expect(tok_double_arrow, "'=>'"));
    auto body_expr = get_expression();
    CE (!kphp_error(body_expr, "Bad expression in arrow function body"));
    auto return_stmt = VertexAdaptor<op_return>::create(body_expr).set_location(func_location);
    cur_function->root->cmd_ref() = VertexAdaptor<op_seq>::create(return_stmt).set_location(func_location);
    auto_capture_vars_from_body_in_arrow_lambda(cur_function);
  } else if (test_expect(tok_opbrc)) { // then we have '{ cmd }' or ';' — function is marked as func_extern in the latter case
    CE(!kphp_error(!cur_function->modifiers.is_abstract(), fmt_format("abstract methods must have empty body: {}", cur_function->as_human_readable())));
    is_top_of_the_function_ = true;
    cur_function->root->cmd_ref() = get_statement().as<op_seq>();
    CE(!kphp_error(cur_function->root->cmd(), "Failed to parse function body"));
    if (cur_function->is_constructor()) {
      VertexUtil::func_force_return(cur_function->root, ClassData::gen_vertex_this(Location(line_num)));
    } else {
      VertexUtil::func_force_return(cur_function->root);
    }
  } else {
    CE(!kphp_error(cur_function->modifiers.is_abstract() || processing_file->is_builtin(), "function must have non-empty body"));
    CE (expect(tok_semicolon, "';'"));
    cur_function->root->cmd_ref() = VertexAdaptor<op_seq>::create();
  }

  // the function is ready, register it;
  // the constructor is registered later, after the entire class is parsed
  if (!cur_function->is_constructor()) {
    bool kphp_required_flag = phpdoc && phpdoc->has_tag(PhpDocType::kphp_required, PhpDocType::kphp_lib_export);
    bool auto_require = cur_function->is_extern()
                        || cur_function->modifiers.is_abstract()
                        || cur_function->modifiers.is_instance()
                        || cur_function->is_lambda()
                        || kphp_required_flag;
    G->register_and_require_function(cur_function, parsed_os, auto_require);
  }

  return cur_function->root;
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

void GenTree::parse_extends_implements() {
  if (test_expect(tok_extends)) {     // extends comes before 'implements', the order is fixed
    do {
      next_cur();                       // (same as in PHP)
      kphp_error_return(test_expect(tok_func_name), "Class name expected after 'extends'");
      kphp_error(!cur_class->is_trait(), "Traits may not extend each other");
      cur_class->add_str_dependent(cur_function, cur_class->class_type, cur->str_val);
      next_cur();
    } while (cur_class->is_interface() && test_expect(tok_comma));
  }

  if (test_expect(tok_implements)) {
    do {
      next_cur();
      kphp_error(test_expect(tok_func_name), "Class name expected after 'implements'");
      cur_class->add_str_dependent(cur_function, ClassType::interface, cur->str_val);
      next_cur();
    } while (test_expect(tok_comma));
  }
}

VertexPtr GenTree::get_class(const PhpDocComment *phpdoc, ClassType class_type) {
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
  if (processing_file->namespace_name.empty() && phpdoc && phpdoc->has_tag(PhpDocType::kphp_tl_class)) {
    kphp_assert(processing_file->is_builtin());
    full_class_name = G->settings().tl_namespace_prefix.get() + name_str;
  } else {
    full_class_name = processing_file->namespace_name.empty() ? std::string{name_str} : processing_file->namespace_name + "\\" + name_str;
  }

  kphp_error(processing_file->namespace_uses.find(name_str) == processing_file->namespace_uses.end(),
             "Class name is the same as one of 'use' at the top of the file");

  ClassPtr class_ptr = ClassPtr(new ClassData{class_type});
  StackPushPop<ClassPtr> c_alive(class_stack, cur_class, class_ptr);
  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, cur_class->gen_holder_function(full_class_name));

  cur_class->modifiers = modifiers;
  if (cur_class->is_interface()) {
    cur_class->modifiers.set_abstract();
  }
  // @kphp-generic should be applied in gentree, even before parsing extends, see comment for functions
  if (phpdoc && phpdoc->has_tag(PhpDocType::kphp_generic)) {
    kphp_error(0, "@kphp-generic for classes is unsupported yet");
  }

  parse_extends_implements();

  auto name_vertex = VertexAdaptor<op_func_name>::create().set_location(auto_location());
  name_vertex->str_val = static_cast<std::string>(name_str);

  cur_class->file_id = processing_file;
  cur_class->set_name_and_src_name(full_class_name);    // with full namespaces and slashes
  cur_class->phpdoc = phpdoc;
  cur_class->is_immutable = phpdoc && phpdoc->has_tag(PhpDocType::kphp_immutable_class);
  cur_class->location_line_num = line_num;

  bool registered = G->register_class(cur_class);
  if (registered) {
    ++G->stats.total_classes;
  }

  VertexPtr body_vertex = get_statement();    // an empty op_seq
  CE (!kphp_error(body_vertex, "Failed to parse class body"));

  cur_class->add_class_constant(); // A::class

  if (auto constructor_method = cur_class->members.get_constructor()) {
    cur_class->has_custom_constructor = !constructor_method->modifiers.is_abstract();
    G->register_and_require_function(constructor_method, parsed_os, true);
  }

  if (registered) {
    G->register_and_require_function(cur_function, parsed_os, true);  // push the class down the pipeline
  }

  return {};
}


VertexAdaptor<op_func_call> GenTree::gen_constructor_call_with_args(const std::string &allocated_class_name, std::vector<VertexPtr> args, const Location &location) {
  auto alloc = VertexAdaptor<op_alloc>::create().set_location(location);
  alloc->allocated_class_name = allocated_class_name;
  auto constructor_call = VertexAdaptor<op_func_call>::create(alloc, std::move(args)).set_location(location);
  constructor_call->str_val = ClassData::NAME_OF_CONSTRUCT;
  constructor_call->extra_type = op_ex_constructor_call;

  return constructor_call;
}

VertexAdaptor<op_func_call> GenTree::gen_constructor_call_with_args(ClassPtr allocated_class, std::vector<VertexPtr> args, const Location &location) {
  auto res_func_call = gen_constructor_call_with_args(allocated_class->name, std::move(args), location);
  res_func_call->args()[0].as<op_alloc>()->allocated_class = allocated_class;
  res_func_call->func_id = allocated_class->construct_function;

  return res_func_call;
}

VertexAdaptor<op_var> GenTree::auto_capture_this_in_lambda(FunctionPtr f_lambda) {
  auto v_captured_this = VertexAdaptor<op_var>::create();
  v_captured_this->str_val = "this";
  v_captured_this->extra_type = op_ex_var_this;

  kphp_assert(f_lambda->is_lambda());
  // bubble up $this through all nested lambdas upwards
  while (f_lambda->is_lambda()) {
    G->operate_on_function_locking(f_lambda->name, [v_captured_this](FunctionPtr &f_lambda) {
      if (f_lambda->uses_list.empty() || f_lambda->uses_list.front()->extra_type != op_ex_var_this) {
        f_lambda->uses_list.emplace_front(v_captured_this);
      }
    });
    f_lambda = f_lambda->outer_function;
  }

  return v_captured_this;
}

VertexPtr GenTree::process_arrow(VertexPtr lhs, VertexPtr rhs) {
  if (rhs->type() == op_func_name) {
    auto inst_prop = VertexAdaptor<op_instance_prop>::create(lhs);
    inst_prop->str_val = rhs->get_string();
    return inst_prop;

  } else if (auto as_func_call = rhs.try_as<op_func_call>()) {
    auto new_root = VertexAdaptor<op_func_call>::create(lhs, as_func_call->args());
    new_root->extra_type = op_ex_func_call_arrow;
    new_root->str_val = as_func_call->str_val;
    new_root->reifiedTs = as_func_call->reifiedTs;
    return new_root;

  } else {
    kphp_error (false, "Operator '->' expects property or function call as its right operand");
    return {};
  }
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
  processing_file->namespace_name = static_cast<std::string>(cur->str_val);
  std::string real_relative_dir = processing_file->relative_dir_name;
  if (processing_file->owner_lib) {
    vk::string_view current_file_relative_dir = real_relative_dir;
    vk::string_view lib_unified_dir = processing_file->owner_lib->unified_lib_dir();
    kphp_assert_msg(current_file_relative_dir.starts_with(lib_unified_dir), "lib processing file should be in lib dir");
    real_relative_dir.erase(0, lib_unified_dir.size() + 1);
  }
  // to properly handle the vendor/ folder, assert from below is commented-out
  //string expected_namespace_name = replace_characters(real_unified_dir, '/', '\\');
  //kphp_error (processing_file->namespace_name == expected_namespace_name, fmt_format("Wrong namespace name, expected {}", expected_namespace_name));

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
  // check whether phpdocs are followed by a declare statement;
  // if they are, parse declare statement (otherwise don't consume any tokens)
  int i = 0;
  while (std::next(cur, i)->type() == tok_phpdoc) {
    i++;
  }
  if (std::next(cur, i)->type() != tok_declare) {
    return;
  }
  // a lot of code is not prepared to strict_types=1 without implicit casts,
  // so we expect an additional annotation that turns this feature on;
  // TODO: remove this toggle later and enable strict_types by default
  bool enabled = false;
  while (cur->type() == tok_phpdoc) {
    if (!enabled && vk::contains(cur->str_val, "@kphp-strict-types-enable")) {
      enabled = true;
    }
    next_cur();
  }

  kphp_assert(test_expect(tok_declare));
  next_cur();
  expect(tok_oppar, "(");

  if (test_expect(tok_func_name)) {
    // declare() may have these directives: strict_types, encoding, ticks;
    // only strict_types is supported
    kphp_error(cur->str_val == "strict_types", fmt_format("Unsupported declare '{}'", cur->str_val));
    next_cur();
  } else {
    kphp_error(0, expect_msg("'tok_func_name'"));
  }

  expect(tok_eq1, "=");

  if (test_expect(tok_int_const)) {
    kphp_error(vk::any_of_equal(cur->str_val, "0", "1"), "strict_types declaration must have 0 or 1 as its value");
    processing_file->is_strict_types = enabled && cur->str_val == "1";
    next_cur();
  } else {
    kphp_error(0, expect_msg("'tok_int_const'"));
  }

  expect(tok_clpar, ")");
  expect(tok_semicolon, ";");
}

VertexAdaptor<op_empty> GenTree::get_static_field_list(const PhpDocComment *phpdoc, FieldModifiers modifiers, const TypeHint *type_hint) {
  cur--;      // it was a $field_name; we do it before the get_multi_call() as it does next_cur() by itself
  VertexAdaptor<op_seq> v = get_multi_call<op_static, op_err>(&GenTree::get_expression);
  CE (check_statement_end());

  for (auto seq: v->args()) {
    // node is a op_var $a (if there is no default value) or op_set(op_var $a, init_val)
    VertexPtr node = seq.as<op_static>()->args()[0];
    switch (node->type()) {
      case op_var: {
        cur_class->members.add_static_field(node.as<op_var>(), VertexPtr{}, modifiers, phpdoc, type_hint);
        break;
      }
      case op_set: {
        auto set_expr = node.as<op_set>();
        kphp_error_act(set_expr->lhs()->type() == op_var, "unexpected expression in 'static'", break);
        cur_class->members.add_static_field(set_expr->lhs().as<op_var>(), set_expr->rhs(), modifiers, phpdoc, type_hint);
        break;
      }
      default:
        kphp_error(0, "unexpected expression in 'static'");
    }
  }

  return VertexAdaptor<op_empty>::create();
}

const TypeHint *GenTree::get_typehint() {
  // optimization: don't start the lexer if it's 100% not a type hint here (so it wouldn't be parsed, and it's okay)
  // without this, everything still works, just a bit slower
  if (vk::any_of_equal(cur->type(), TokenType::tok_var_name, TokenType::tok_clpar, TokenType::tok_and, TokenType::tok_varg)) {
    return {};
  }

  try {
    PhpDocTypeHintParser parser(cur_function, cur_function->get_this_or_topmost_if_lambda()->genericTs);
    return parser.parse_from_tokens(cur);
  } catch (std::runtime_error &) {
    kphp_error(0, "Cannot parse type hint");
    return {};
  }
}

// parse a php comment like /*<int, A>*/ or /*<T>*/ which expresses manual types for generic instantiation
GenericsInstantiationPhpComment *GenTree::parse_php_commentTs(vk::string_view str_commentTs) {
  PhpDocTypeHintParser parser(cur_function, cur_function->get_this_or_topmost_if_lambda()->genericTs);
  std::vector<Token> doc_tokens = phpdoc_to_tokens(str_commentTs);
  std::vector<const TypeHint *> vectorTs;
  auto cur_tok = doc_tokens.cbegin();

  while (cur_tok != doc_tokens.cend() && cur_tok->type() != tok_end) {
    const TypeHint *instantiationT = nullptr;
    try {
      instantiationT = parser.parse_from_tokens(cur_tok);
    } catch (std::runtime_error &ex) {
      kphp_error_act(0, fmt_format("Could not parse generic instantiation comment: {}", ex.what()), return nullptr);
    }

    kphp_error_act(cur_tok->type() == tok_comma || cur_tok->type() == tok_end, "expected ','", return nullptr);
    cur_tok++;
    vectorTs.emplace_back(instantiationT);
  }

  return new GenericsInstantiationPhpComment(vectorTs);
}

VertexAdaptor<op_catch> GenTree::get_catch() {
  CE (expect(tok_catch, "'catch'"));
  CE (expect(tok_oppar, "'('"));
  auto exception_class = cur->str_val;
  CE (expect(tok_func_name, "type that implements Throwable"));
  auto exception_var_name = get_expression();
  CE (!kphp_error(exception_var_name, "Cannot parse catch"));
  CE (!kphp_error(exception_var_name->type() == op_var, "Expected variable name in 'catch'"));

  CE (expect(tok_clpar, "')'"));
  auto catch_body = get_statement();
  CE (!kphp_error(catch_body, "Cannot parse catch block"));

  auto catch_op = VertexAdaptor<op_catch>::create(exception_var_name.as<op_var>(), VertexUtil::embrace(catch_body));
  catch_op->type_declaration = resolve_uses(cur_function, static_cast<std::string>(exception_class));

  return catch_op;
}

VertexPtr GenTree::get_statement(const PhpDocComment *phpdoc) {
  TokenType type = cur->type();

  is_top_of_the_function_ &= vk::any_of_equal(type, tok_global, tok_opbrc);

  switch (type) {
    case tok_opbrc: {
      next_cur();
      auto res = get_seq();
      kphp_error (res, "Failed to parse sequence");
      CE (check_seq_end());
      return res;
    }
    case tok_return:
      return get_return();
    case tok_continue:
      return get_break_or_continue<op_continue>();
    case tok_break:
      return get_break_or_continue<op_break>();
    case tok_unset: {
      auto res = get_multi_call<op_unset, op_none>(&GenTree::get_expression, true);
      CE (check_statement_end());
      if (res->size() == 1) {
        return res->args()[0];
      }
      return res;
    }
    case tok_var_dump: {
      auto res = get_multi_call<op_func_call, op_none>(&GenTree::get_expression, true);
      CE (res && check_statement_end());
      for (VertexPtr x : res->args()) {
        x.as<op_func_call>()->str_val = "var_dump";
      }
      if (res->size() == 1) {
        return res->args()[0];
      }
      return res;
    }
    case tok_define: {
      auto res = get_func_call<op_define, op_err>();
      CE (check_statement_end());
      return res;
    }
    case tok_global: {
      if (G->settings().warnings_level.get() >= 2 && cur_function && cur_function->type == FunctionData::func_local && !is_top_of_the_function_) {
        kphp_warning("`global` keyword is allowed only at the top of the function");
      }
      auto res = get_multi_call<op_global, op_err>(&GenTree::get_var_name);
      CE (check_statement_end());
      return res;
    }
    case tok_protected:
    case tok_public:
    case tok_private:
      if (std::next(cur, 1)->type() == tok_const) {
        next_cur();

        auto access = AccessModifiers::public_;
        if (type == tok_protected) {
          access = AccessModifiers::protected_;
        } else if (type == tok_private) {
          access = AccessModifiers::private_;
        }
        return get_const_after_explicit_access_modifier(access);
      }
    // fall through
    case tok_final:
    case tok_abstract:
      if (cur_function->type == FunctionData::func_class_holder) {
        return get_class_member(phpdoc);
      } else if (vk::any_of_equal(cur->type(), tok_final, tok_abstract)) {
        return get_class(phpdoc, ClassType::klass);
      }
      next_cur();
      kphp_error(0, "Unexpected access modifier outside of class body");
      return {};
    case tok_static: {
      if (cur_function->type == FunctionData::func_class_holder) {
        return get_class_member(phpdoc);
      }
      if (cur + 1 != end && ((cur + 1)->type() == tok_var_name)) {   // a function-scoped static variable
        auto res = get_multi_call<op_static, op_err>(&GenTree::get_expression);
        CE (check_statement_end());
        return res;
      }
      next_cur();
      kphp_error(0, "Expected function or variable after keyword `static`");
      return {};
    }
    case tok_echo: {
      auto res = get_multi_call<op_func_call, op_err>(&GenTree::get_expression);
      for (VertexPtr x : res->args()) {
        x.as<op_func_call>()->str_val = "echo";
      }
      CE (check_statement_end());
      return res;
    }
    case tok_dbg_echo: {
      auto res = get_multi_call<op_func_call, op_err>(&GenTree::get_expression);
      for (VertexPtr x : res->args()) {
        x.as<op_func_call>()->str_val = "dbg_echo";
      }
      CE (check_statement_end());
      return res;
    }
    case tok_throw: {
      auto location = auto_location();
      next_cur();
      auto throw_expr = get_expression();
      CE (!kphp_error(throw_expr, "Empty expression in throw"));
      auto throw_vertex = VertexAdaptor<op_throw>::create(throw_expr).set_location(location);
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
    case tok_phpdoc: {      // enter /** ... */
      // phpdoc above a function/class/field
      auto next = (cur+1)->type();
      if (vk::any_of_equal(next, tok_class, tok_interface, tok_trait, tok_function, tok_public, tok_private, tok_protected, tok_final, tok_abstract, tok_var, tok_const)
          || (next == tok_static && cur_function->type == FunctionData::func_class_holder)) {
        vk::string_view phpdoc_str = cur->str_val;
        next_cur();
        return get_statement(new PhpDocComment(phpdoc_str));
      }
      // phpdoc above a lambda before assignment: /** ... */ $f = function() { ... };
      // we do not support complex expressions on the left-hand side (like $f[0] = fn)
      // todo reconsider this when I'll rewrite phpdoc parsing
      if (cur+5 < tokens.end() && next == tok_var_name && (cur+2)->type() == tok_eq1 && vk::any_of_equal((cur+3)->type(), tok_function, tok_fn, tok_static)) {
        vk::string_view phpdoc_str = cur->str_val;
        if (phpdoc_str.find("@var") == std::string::npos) { // if it's not @var for $f, but @param for lambda
          next_cur();
          auto lhs = get_expr_top(false);
          next_cur();
          auto rhs = get_expr_top(false, new PhpDocComment(phpdoc_str));
          return rhs ? VertexPtr(VertexAdaptor<op_set>::create(lhs, rhs)) : rhs;
        }
      }
      // otherwise it's a phpdoc-statement: it may contain @var which will be used for assumptions and tinf
      return get_phpdoc_inside_function();
    }
    case tok_function:
      if (cur_class) {      // no access modifier implies 'public'
        get_class_member(phpdoc);
      } else {
        get_function(false, phpdoc, FunctionModifiers::nonmember());
      }
      return {};

    case tok_try: {
      auto location = auto_location();
      next_cur();
      auto try_body = get_statement();
      CE (!kphp_error(try_body, "Cannot parse try block"));

      std::vector<VertexPtr> catch_list;
      while (test_expect(tok_catch)) {
        auto catch_op = get_catch();
        CE (!kphp_error(catch_op, "Cannot parse catch statement"));
        catch_op.set_location(location);
        catch_list.emplace_back(catch_op);
      }
      CE (!kphp_error(!catch_list.empty(), "Expected at least 1 'catch' statement"));

      return VertexAdaptor<op_try>::create(VertexUtil::embrace(try_body), std::move(catch_list)).set_location(location);
    }
    case tok_inline_html: {
      auto html_code = VertexAdaptor<op_string>::create().set_location(auto_location());
      html_code->str_val = static_cast<std::string>(cur->str_val);

      auto echo_cmd = VertexAdaptor<op_func_call>::create(html_code).set_location(auto_location());
      echo_cmd->str_val = "print";
      next_cur();
      return echo_cmd;
    }
    case tok_at: {
      auto location = auto_location();
      next_cur();
      auto expression = get_statement();
      CE (expression);
      return VertexAdaptor<op_noerr>::create(expression).set_location(location);
    }
    case tok_clbrc: {
      return {};
    }
    case tok_const: {
      return get_const(AccessModifiers::public_);
    }
    case tok_use: {
      if (cur_class) {
        get_traits_uses();
      } else {
        kphp_error(cur_function && cur_function->is_main_function(), "'use' can be declared only in global scope");
        get_use();
      }
      return VertexAdaptor<op_empty>::create();
    }
    case tok_var: {
      next_cur();
      get_instance_var_list(phpdoc, FieldModifiers{}.set_public(), get_typehint());
      CE (check_statement_end());
      auto empty = VertexAdaptor<op_empty>::create();
      return empty;
    }
    case tok_class:
      return get_class(phpdoc, ClassType::klass);
    case tok_interface:
      return get_class(phpdoc, ClassType::interface);
    case tok_trait: {
      auto res = get_class(phpdoc, ClassType::trait);
      kphp_error(processing_file->namespace_uses.empty(), "Usage of operator `use`(Aliasing/Importing) with traits is temporarily prohibited");
      return res;
    }
    default: {
      auto res = get_expression();
      CE (check_statement_end());
      if (!res) {
        res = VertexAdaptor<op_empty>::create().set_location(auto_location());
      }
      return res;
    }
  }
  kphp_fail();
}

VertexPtr GenTree::get_const_after_explicit_access_modifier(AccessModifiers access) {
  CE(!kphp_error(cur_class, "Access modifier for const allowed in class only"));
  if (cur_class->is_interface()) {
    CE(!kphp_error(access == AccessModifiers::public_, "const access modifier in interface must be public"));
  }

  return get_const(access);
}

VertexPtr GenTree::get_const(AccessModifiers access) {
  auto location = auto_location();
  next_cur();

  bool const_in_global_scope = functions_stack.size() == 1 && !cur_class;
  bool const_in_class = !!cur_class;

  CE (!kphp_error(const_in_global_scope || const_in_class, "const expressions supported only inside classes and namespaces or in global scope"));
  std::string const_name = get_identifier();
  CE (!const_name.empty());
  kphp_error(const_name != "class", "A class constant must not be called 'class'; it is reserved for class name fetching");

  CE (expect(tok_eq1, "'='"));
  VertexPtr v = get_expression();
  CE (check_statement_end());

  if (const_in_class) {
    cur_class->members.add_constant(const_name, v, access);
    return VertexAdaptor<op_empty>::create();
  }

  auto name = VertexAdaptor<op_string>::create();
  name->str_val = const_name;
  return VertexAdaptor<op_define>::create(name, v).set_location(location);
}

void GenTree::get_instance_var_list(const PhpDocComment *phpdoc, FieldModifiers modifiers, const TypeHint *type_hint) {
  kphp_error(cur_class, "var declaration is outside of class");

  vk::string_view var_name = cur->str_val;
  if (!expect(tok_var_name, "expected variable name")) {
    return;
  }

  VertexPtr def_val;
  if (test_expect(tok_eq1)) {
    next_cur();
    def_val = get_expression();
  }

  auto var = VertexAdaptor<op_var>::create().set_location(auto_location());
  var->str_val = static_cast<std::string>(var_name);

  cur_class->members.add_instance_field(var, def_val, modifiers, phpdoc, type_hint);

  if (test_expect(tok_comma)) {
    next_cur();
    get_instance_var_list(phpdoc, modifiers, type_hint);
  }
}

void GenTree::get_seq(std::vector<VertexPtr> &seq_next) {
  while (cur != end && !test_expect(tok_clbrc)) {
    if (auto cur_node = get_statement()) {
      seq_next.push_back(cur_node);
    }
  }
}

VertexAdaptor<op_seq> GenTree::get_seq() {
  auto location = auto_location();

  std::vector<VertexPtr> seq_next;
  get_seq(seq_next);

  return VertexAdaptor<op_seq>::create(seq_next).set_location(location);
}

void GenTree::run() {
  auto v_func_params = VertexAdaptor<op_func_param_list>::create();
  auto root = VertexAdaptor<op_function>::create(v_func_params, VertexAdaptor<op_seq>{}).set_location(auto_location());

  StackPushPop<FunctionPtr> f_alive(functions_stack, cur_function, FunctionData::create_function(processing_file->main_func_name, root, FunctionData::func_main));
  processing_file->main_function = cur_function;

  parse_declare_at_top_of_file();

  if (test_expect(tok_phpdoc) && std::next(cur, 1)->type() == tok_namespace) {
    skip_phpdoc_tokens();
  }
  if (test_expect(tok_namespace)) {
    parse_namespace_and_uses_at_top_of_file();
  }

  auto location = auto_location();
  std::vector<VertexPtr> seq_next;
  auto set_run_flag = VertexAdaptor<op_set>::create(processing_file->get_main_func_run_var(), VertexAdaptor<op_true>::create());
  seq_next.push_back(set_run_flag);
  get_seq(seq_next);
  seq_next.push_back(VertexAdaptor<op_return>::create(VertexAdaptor<op_null>::create()));
  cur_function->root->cmd_ref() = VertexAdaptor<op_seq>::create(seq_next).set_location(location);

  G->register_and_require_function(cur_function, parsed_os, true);  // global function — therefore required

  if (processing_file->file_name == G->settings().functions_file.get()) {
    G->set_functions_txt_parsed();
  }

  if (cur != end) {
    fmt_fprintf(stderr, "line {}: something wrong\n", line_num);
    kphp_error (0, "Cannot compile (probably problems with brace balance)");
  }
}

bool GenTree::is_magic_method_name_allowed(const std::string &name) {
  static std::set<std::string> names = {
    ClassData::NAME_OF_CONSTRUCT,
    ClassData::NAME_OF_CLONE,
    ClassData::NAME_OF_TO_STRING,
    ClassData::NAME_OF_WAKEUP,
    // other __-staring methods like __invoke/_self$ are implicitly generated and aren't allowed to occur in php
  };

  return vk::contains(names, name);
}

#undef CE
