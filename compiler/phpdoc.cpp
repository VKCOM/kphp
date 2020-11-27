// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/phpdoc.h"

#include <cstdio>
#include <utility>

#include "common/termformat/termformat.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"
#include "compiler/utils/string-utils.h"

using std::vector;
using std::string;

const std::map<string, php_doc_tag::doc_type> php_doc_tag::str2doc_type = {
  {"@param",                     param},
  {"@kphp-inline",               kphp_inline},
  {"@kphp-flatten",              kphp_flatten},
  {"@kphp-infer",                kphp_infer},
  {"@kphp-required",             kphp_required},
  {"@kphp-lib-export",           kphp_lib_export},
  {"@kphp-sync",                 kphp_sync},
  {"@kphp-should-not-throw",     kphp_should_not_throw},
  {"@type",                      var},
  {"@var",                       var},
  {"@return",                    returns},
  {"@returns",                   returns},
  {"@kphp-disable-warnings",     kphp_disable_warnings},
  {"@kphp-extern-func-info",     kphp_extern_func_info},
  {"@kphp-pure-function",        kphp_pure_function},
  {"@kphp-template",             kphp_template},
  {"@kphp-return",               kphp_return},
  {"@kphp-memcache-class",       kphp_memcache_class},
  {"@kphp-immutable-class",      kphp_immutable_class},
  {"@kphp-tl-class",             kphp_tl_class},
  {"@kphp-const",                kphp_const},
  {"@kphp-no-return",            kphp_noreturn},
  {"@kphp-warn-unused-result",   kphp_warn_unused_result},
  {"@kphp-serializable",         kphp_serializable},
  {"@kphp-reserved-fields",      kphp_reserved_fields},
  {"@kphp-serialized-field",     kphp_serialized_field},
  {"@kphp-profile",              kphp_profile},
  {"@kphp-profile-allow-inline", kphp_profile_allow_inline},
};

vector<php_doc_tag> parse_php_doc(vk::string_view phpdoc) {
  if (phpdoc.empty()) {
    return {};
  }

  int line_num_of_function_declaration = stage::get_line();

  vector<string> lines(1);
  bool have_star = false;
  for (char c : phpdoc) {
    if (!have_star) {
      if (c == ' ' || c == '\t') {
        continue;
      }
      if (c == '*') {
        have_star = true;
        continue;
      }
      kphp_error(0, "failed to parse php_doc");
      return vector<php_doc_tag>();
    }
    if (c == '\n') {
      lines.push_back("");
      have_star = false;
      continue;
    }
    if (lines.back().empty() && (c == ' ' || c == '\t')) {
      continue;
    }
    lines.back() += c;
  }
  vector<php_doc_tag> result;
  for (int i = 0; i < lines.size(); i++) {
    if (lines[i][0] == '@') {
      result.emplace_back(php_doc_tag());
      size_t pos = lines[i].find(' ');

      auto name = lines[i].substr(0, pos);
      auto type = php_doc_tag::get_doc_type(name);
      if (vk::string_view{name}.starts_with("@kphp") && type == php_doc_tag::unknown) {
        kphp_error(0, fmt_format("unrecognized kphp tag: {}", name));
      }

      result.back().name = std::move(name);
      result.back().type = type;

      if (pos != string::npos) {
        int ltrim_pos = pos + 1;
        while (lines[i][ltrim_pos] == ' ') {
          ltrim_pos++;
        }
        result.back().value = lines[i].substr(ltrim_pos);
      }
    }

    if (line_num_of_function_declaration > 0 && !result.empty()) {
      int new_line_num = line_num_of_function_declaration - (static_cast<int>(lines.size()) - i);
      // We have one line with closing php-doc before function declaration
      // e.g. ....
      //      * @param int $a
      //      */
      //      function f() {}
      result.back().line_num = std::min(new_line_num, line_num_of_function_declaration - 2);
    }
  }
//  for (int i = 0; i < result.size(); i++) {
//    fprintf(stderr, "|%s| : |%s|\n", result[i].name.c_str(), result[i].value.c_str());
//  }
  return result;
}

VertexPtr PhpDocTypeRuleParser::parse_classname(const std::string &phpdoc_class_name) {
  const std::string &class_name = resolve_uses(current_function, phpdoc_class_name, '\\');
  ClassPtr klass = G->get_class(class_name);
  if (!klass) {
    unknown_classes_list.push_back(class_name);
  }
  if (klass && klass->is_trait()) {
    throw std::runtime_error("You may not use trait as a type-hint");
  }

  cur_tok++;
  return GenTree::create_type_help_class_vertex(klass);
}

VertexPtr PhpDocTypeRuleParser::parse_simple_type() {
  TokenType cur_type = cur_tok->type();
  // some type names inside phpdoc are not keywords/tokens, but they should be interpreted as such
  if (cur_type == tok_func_name) {
    if (cur_tok->str_val == "integer") {
      cur_type = tok_int;
    } else if (cur_tok->str_val == "double") {
      cur_type = tok_float;
    } else if (cur_tok->str_val == "boolean") {
      cur_type = tok_bool;
    } else if (cur_tok->str_val == "\\tuple") {
      cur_type = tok_tuple;
    } else if (cur_tok->str_val == "\\shape") {
      cur_type = tok_shape;
    }
  }

  switch (cur_type) {
    case tok_end:
      throw std::runtime_error("unexpected end");
    case tok_oppar: {
      cur_tok++;
      VertexPtr v = parse_type_expression();
      if (cur_tok->type() != tok_clpar) {
        throw std::runtime_error("unmatching ()");
      }
      cur_tok++;
      return v;
    }
    case tok_int:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_int);
    case tok_bool:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_bool);
    case tok_float:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_float);
    case tok_string:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_string);
    case tok_false:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_False);
    case tok_true:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_bool);
    case tok_null:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_Null);
    case tok_mixed:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_mixed);
    case tok_void:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_void);
    case tok_tuple:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_tuple, parse_nested_type_rules());
    case tok_shape:
      cur_tok++;
      return parse_shape_type();
    case tok_callable:
      cur_tok++;
      if (cur_tok->type() == tok_oppar) {    // callable(...) : ... — typed callable
        return parse_typed_callable();
      }
      return VertexAdaptor<op_type_expr_callable>::create();
    case tok_array:
      cur_tok++;
      if (vk::any_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {   // array<...>
        return GenTree::create_type_help_vertex(tp_array, {parse_nested_one_type_rule()});
      }
      return GenTree::create_type_help_vertex(tp_array, {GenTree::create_type_help_vertex(tp_Unknown)});
    case tok_at: {      // @tl\...
      cur_tok++;
      if (!cur_tok->str_val.starts_with("tl\\")) {
        throw std::runtime_error("Invalid magic namespace after '@'");
      }
      return parse_classname(G->settings().tl_namespace_prefix.get() + std::string(cur_tok->str_val).substr(3));
    }
    case tok_xor:       // ^1, ^2[*] (for functions.txt)
      cur_tok++;
      return parse_arg_ref();
    case tok_question:  // ?string == string|null; ?string[] == string[]|null
      cur_tok++;
      return VertexAdaptor<op_type_expr_lca>::create(parse_type_expression(), GenTree::create_type_help_vertex(tp_Null));
    case tok_object:
      cur_tok++;
      return GenTree::create_type_help_vertex(tp_Any);

    case tok_static:
    case tok_func_name:
      // tok_future doesn't exist, it's a string
      if (vk::any_of_equal(cur_tok->str_val, "future", "\\future")) {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_future, {parse_nested_one_type_rule()});
      }
      // same for the future_queue
      if (cur_tok->str_val == "future_queue") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_future_queue, {parse_nested_one_type_rule()});
      }
      // plus some extra types that are not tokens as well, but they make sense inside the phpdoc/functions.txt
      if (cur_tok->str_val == "any") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_Any);
      }
      if (cur_tok->str_val == "regexp") {
        cur_tok++;
        return GenTree::create_type_help_vertex(tp_regexp);
      }
      // force(T) = T (for PhpStorm)
      if (cur_tok->str_val == "force") {
        cur_tok++;
        return parse_nested_one_type_rule();
      }
      // (for functions.txt) instance<^2>
      if (cur_tok->str_val == "instance") {
        cur_tok++;
        return VertexAdaptor<op_type_expr_instance>::create(parse_nested_one_type_rule());
      }
      // (for functions.txt) DropFalse<^1>
      if (cur_tok->str_val == "DropFalse") {
        cur_tok++;
        return VertexAdaptor<op_type_expr_drop_false>::create(parse_nested_one_type_rule());
      }
      // (for functions.txt) DropNull<^1>
      if (cur_tok->str_val == "DropNull") {
        cur_tok++;
        return VertexAdaptor<op_type_expr_drop_null>::create(parse_nested_one_type_rule());
      }
      // otherwise interpreted as a class name (including the lowercase names);
      // it works with the absolute and relative names as well as for a special names like 'self';
      // file 'use' directives are taken into the account
      return parse_classname(std::string(cur_tok->str_val));

    default:
      throw std::runtime_error(fmt_format("can't parse '{}'", cur_tok->str_val));
  }
}

VertexPtr PhpDocTypeRuleParser::parse_arg_ref() {   // ^1, ^2[*]
  if (cur_tok->type() != tok_int_const) {
    throw std::runtime_error("Invalid number after ^");
  }
  auto v = VertexAdaptor<op_type_expr_arg_ref>::create();
  v->int_val = std::stoi(std::string(cur_tok->str_val));

  VertexPtr res = v;
  cur_tok++;
  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_times && (cur_tok + 2)->type() == tok_clbrk) {
    res = VertexAdaptor<op_index>::create(res);
    cur_tok += 3;
  }
  while (cur_tok->type() == tok_oppar && (cur_tok + 1)->type() == tok_clpar) {
    res = VertexAdaptor<op_type_expr_callback_call>::create(res);
    cur_tok += 2;
  }

  return res;
}

VertexPtr PhpDocTypeRuleParser::parse_type_array() {
  VertexPtr res = parse_simple_type();

  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_clbrk) {
    res = GenTree::create_type_help_vertex(tp_array, {res});
    cur_tok += 2;
  }
  if (cur_tok->type() == tok_varg) {      // "int ...$args" is identical to "int [] $args"; can also handle "(int|false) ..."
    cur_tok++;
    res = GenTree::create_type_help_vertex(tp_array, {res});
  }

  return res;
}

std::vector<VertexPtr> PhpDocTypeRuleParser::parse_nested_type_rules() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;
  std::vector<VertexPtr> sub_types;
  while (true) {
    sub_types.emplace_back(parse_type_expression());

    if (vk::any_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
      cur_tok++;
      break;
    } else if (cur_tok->type() == tok_comma) {
      cur_tok++;
    } else {
      throw std::runtime_error("expected ')' or ','");
    }
  }
  return sub_types;
}

VertexPtr PhpDocTypeRuleParser::parse_nested_one_type_rule() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;

  VertexPtr sub_type = parse_type_expression();
  if (vk::none_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
    throw std::runtime_error("expected ')'");
  }
  cur_tok++;

  return sub_type;
}

namespace {
bool has_compound_type_inside(VertexPtr type) {
  if (auto lca_rule = type.try_as<op_type_expr_lca>()) {
    VertexPtr lhs = lca_rule->args()[0];
    VertexPtr rhs = lca_rule->args()[1];
    if (vk::any_of_equal(lhs->type_help, tp_False, tp_Null)) {
      return has_compound_type_inside(rhs);
    } else if (vk::any_of_equal(rhs->type_help, tp_False, tp_Null)) {
      return has_compound_type_inside(lhs);
    } else {
      return true;
    }
  }

  auto get_args = [type]() { return type.as<op_type_expr_type>()->args(); };
  auto check_shape_arg = [](VertexPtr v) { return has_compound_type_inside(v.as<op_double_arrow>()->rhs()); };
  switch (type->type_help) {
    case tp_array: return has_compound_type_inside(get_args()[0]);
    case tp_tuple: return vk::any_of(get_args(), has_compound_type_inside);
    case tp_shape: return vk::any_of(get_args(), check_shape_arg);
    default:       return false;
  }
};
} // namespace

VertexPtr PhpDocTypeRuleParser::parse_typed_callable() {  // callable(int, int):int, callable(int), callable():void
  if (cur_tok->type() != tok_oppar) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;

  std::vector<VertexPtr> arg_types;
  while (true) {
    if (cur_tok->type() == tok_clpar) {
      cur_tok++;
      break;
    }
    arg_types.emplace_back(parse_type_expression());

    if (cur_tok->type() == tok_comma) {
      cur_tok++;
    } else if (cur_tok->type() != tok_clpar) {
      throw std::runtime_error("expected ')' or ','");
    }
  }

  VertexPtr return_type;
  if (cur_tok->type() == tok_colon) {
    cur_tok++;
    return_type = parse_type_expression();
  } else {
    return_type = GenTree::create_type_help_vertex(tp_void);
  }

  // To generate a consistent name of lambda's interface we need arguments' types and return type
  // At this stage it's not obvious how to calculate the lca of types such as (float[]|int[]), (string|int), (mixed|int[])
  // but at the same time |null & |false are allowed; example of valid type: callable(?int, float[]|false, tuple(int)):shape(x:int)
  kphp_error(!vk::any_of(arg_types, has_compound_type_inside) && !has_compound_type_inside(return_type), "Callable type may not contain compound types (int|string...)");

  return VertexAdaptor<op_type_expr_callable>::create(VertexAdaptor<op_func_param_list>::create(arg_types), return_type);
}

VertexPtr PhpDocTypeRuleParser::parse_shape_type() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;

  std::vector<VertexAdaptor<op_double_arrow>> shape_rules;
  bool has_varg = false;
  // example: shape(x:int, y?:\A, z:tuple(...))
  // elements are [ op_double_arrow ( op_func_name "x", type_rule "int" ), ... ]
  // may end with tok_varg: shape(x:int, ...)
  while (true) {
    auto elem_name_v = VertexAdaptor<op_func_name>::create();
    elem_name_v->str_val = static_cast<std::string>(cur_tok->str_val);
    cur_tok++;

    bool is_nullable = false;
    if (cur_tok->type() == tok_question) {
      is_nullable = true;
      cur_tok++;
    }
    if (cur_tok->type() != tok_colon) {
      throw std::runtime_error("expected ':'");
    }
    cur_tok++;

    VertexPtr elem_type_rule = parse_type_expression();
    if (is_nullable) {
      elem_type_rule = VertexAdaptor<op_type_expr_lca>::create(elem_type_rule, GenTree::create_type_help_vertex(tp_Null));
    }

    shape_rules.emplace_back(VertexAdaptor<op_double_arrow>::create(elem_name_v, elem_type_rule));

    if (vk::any_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
      cur_tok++;
      break;
    } else if (cur_tok->type() == tok_comma) {
      cur_tok++;
      if (cur_tok->type() == tok_varg) {
        has_varg = true;
        cur_tok++;
        if (vk::any_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
          cur_tok++;
          break;
        } else {
          throw std::runtime_error("expected ')' after ...");
        }
      }
    } else {
      throw std::runtime_error("expected ')' or ','");
    }
  }

  VertexPtr shape_type_help = GenTree::create_type_help_vertex(tp_shape, shape_rules);
  if (has_varg) {
    shape_type_help->extra_type = op_ex_shape_has_varg;
  }
  return shape_type_help;
}

VertexPtr PhpDocTypeRuleParser::parse_type_expression() {
  VertexPtr result = parse_type_array();
  bool is_raw_bool = result->type() == op_type_expr_type && result->type_help == tp_bool && (cur_tok - 1)->type() == tok_bool;
  bool is_raw_null = result->type() == op_type_expr_type && result->type_help == tp_Null && (cur_tok - 1)->type() == tok_null;
  while (cur_tok->type() == tok_or) {
    cur_tok++;
    // lhs|rhs => lca(lhs,rhs)
    VertexPtr rhs = parse_type_array();
    result = VertexAdaptor<op_type_expr_lca>::create(result, rhs);

    is_raw_bool |= rhs->type() == op_type_expr_type && rhs->type_help == tp_bool && (cur_tok - 1)->type() == tok_bool;
    is_raw_null |= rhs->type() == op_type_expr_type && rhs->type_help == tp_Null && (cur_tok - 1)->type() == tok_null;
    if (is_raw_bool && !is_raw_null) {
      throw std::runtime_error("Do not use |bool in phpdoc, use |false instead\n(if you really need bool, specify |boolean)");
    }
  }
  return result;
}

VertexPtr PhpDocTypeRuleParser::parse_from_tokens(std::vector<Token>::const_iterator &tok_iter) {
  cur_tok = tok_iter;
  VertexPtr v = parse_type_expression();      // could throw an exception
  tok_iter = cur_tok;
  return v;                                   // not null if an exception was not thrown
}

VertexPtr PhpDocTypeRuleParser::parse_from_tokens_silent(std::vector<Token>::const_iterator &tok_iter) noexcept {
  try {
    return parse_from_tokens(tok_iter);
  } catch (std::runtime_error &) {
    return {};
  }
}

/*
 * With a phpdoc string after a @param/@return/@var like "int|false $a maybe comment"
 * or "$var tuple(int, string) maybe comment" or "A[] maybe comment"
 * parse a type (turn it into the tree for a type_rule) and a variable name (if present).
 */
PhpDocTagParseResult phpdoc_parse_type_and_var_name(vk::string_view phpdoc_tag_str, FunctionPtr current_function) {
  std::vector<Token> tokens = phpdoc_to_tokens(phpdoc_tag_str);
  std::vector<Token>::const_iterator tok_iter = tokens.begin();
  std::string var_name;

  // $var_name phpdoc|type maybe comment
  if (tokens.front().type() == tok_var_name) {
    var_name = std::string(tokens.front().str_val);
    tok_iter++;
    if (tokens.size() <= 2) {     // only tok_end is left
      return {VertexPtr{}, std::move(var_name)};
    }
  }

  PhpDocTypeRuleParser parser(current_function);
  VertexPtr doc_type;
  try {
    doc_type = parser.parse_from_tokens(tok_iter);
  } catch (std::runtime_error &ex) {
    stage::set_location(current_function->root->location);
    kphp_error(0, fmt_format("{}: {}\n{}",
                             TermStringFormat::paint_red(TermStringFormat::add_text_attribute("Could not parse phpdoc tag", TermStringFormat::bold)),
                             TermStringFormat::add_text_attribute(std::string(phpdoc_tag_str), TermStringFormat::underline),
                             ex.what()));
  }

  if (!parser.get_unknown_classes().empty()) {
    stage::set_location(current_function->root->location);
    kphp_error(0, fmt_format("{}: {}",
                             TermStringFormat::paint_red(TermStringFormat::add_text_attribute("Could not find class in phpdoc", TermStringFormat::bold)),
                             TermStringFormat::add_text_attribute(*parser.get_unknown_classes().begin(), TermStringFormat::underline)));
    return {VertexPtr{}, std::move(var_name)};
  }

  if (var_name.empty()) {
    kphp_assert(tok_iter != tokens.end());
    // phpdoc|type $var_name maybe comment
    if (tok_iter->type() == tok_var_name) {   // tok_iter — right after the parsing is finished
      var_name = std::string(tok_iter->str_val);
    }
  }

  return {doc_type, std::move(var_name)};
}

/*
 * With a full phpdoc string / ** ... * / with various tags inside,
 * find the first @tag and parse everything on its right side.
 * Returns result which has a bool() operator (reports whether this tag was found or not).
 */
PhpDocTagParseResult phpdoc_find_tag(vk::string_view phpdoc, php_doc_tag::doc_type tag_type, FunctionPtr current_function) {
  if (auto found_tag = phpdoc_find_tag_as_string(phpdoc, tag_type)) {
    return phpdoc_parse_type_and_var_name(*found_tag, current_function);
  }
  return {VertexPtr{}, std::string()};
}

/*
 * With a full phpdoc string / ** ... * /,
 * find all @tag and parse everything on their right side.
 * Useful for @param tags.
 */
std::vector<PhpDocTagParseResult> phpdoc_find_tag_multi(vk::string_view phpdoc, php_doc_tag::doc_type tag_type, FunctionPtr current_function) {
  std::vector<PhpDocTagParseResult> result;
  for (const auto &tag : parse_php_doc(phpdoc)) {
    if (tag.type == tag_type) {
      if (auto parsed = phpdoc_parse_type_and_var_name(tag.value, current_function)) {
        result.emplace_back(std::move(parsed));
      }
    }
  }
  return result;
}

/*
 * With a full phpdoc string / ** ... * /,
 * find the first @tag and return everything on its right side as a string.
 */
vk::optional<std::string> phpdoc_find_tag_as_string(vk::string_view phpdoc, php_doc_tag::doc_type tag_type) {
  for (const auto &tag : parse_php_doc(phpdoc)) {
    if (tag.type == tag_type) {
      return tag.value;
    }
  }
  return {};
}

/*
 * With a full phpdoc string / ** ... * /,
 * find all @tag and return everything on their right side as a string.
 * Useful for @kphp-template tags.
 */
std::vector<std::string> phpdoc_find_tag_as_string_multi(vk::string_view phpdoc, php_doc_tag::doc_type tag_type) {
  std::vector<std::string> result;
  for (const auto &tag : parse_php_doc(phpdoc)) {
    if (tag.type == tag_type) {
      result.emplace_back(tag.value);
    }
  }
  return result;
}

bool phpdoc_tag_exists(vk::string_view phpdoc, php_doc_tag::doc_type tag_type) {
  return static_cast<bool>(phpdoc_find_tag_as_string(phpdoc, tag_type));
}
