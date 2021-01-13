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
#include "compiler/type-hint.h"
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
  {"@kphp-throws",               kphp_throws},
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
  {"@kphp-warn-performance",     kphp_warn_performance},
  {"@kphp-analyze-performance",  kphp_analyze_performance},
  {"@kphp-serializable",         kphp_serializable},
  {"@kphp-reserved-fields",      kphp_reserved_fields},
  {"@kphp-serialized-field",     kphp_serialized_field},
  {"@kphp-serialized-float32",   kphp_serialized_float32},
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
      bool is_singleline = lines.size() < 2;  // if /** ... */ as a single line, otherwise assume */ to be on a separate line
      result.back().line_num = std::min(new_line_num, line_num_of_function_declaration - (is_singleline ? 1 : 2));
    }
  }
//  for (int i = 0; i < result.size(); i++) {
//    fprintf(stderr, "|%s| : |%s|\n", result[i].name.c_str(), result[i].value.c_str());
//  }
  return result;
}

const TypeHint *PhpDocTypeRuleParser::parse_classname(const std::string &phpdoc_class_name) {
  cur_tok++;
  // here we have a relative class name, that can be resolved into full unless self/static/parent
  // if self/static/parent, it will be resolved at context, see phpdoc_finalize_type_hint_and_resolve()
  return is_string_self_static_parent(phpdoc_class_name)
         ? TypeHintInstance::create(phpdoc_class_name)
         : TypeHintInstance::create(resolve_uses(current_function, phpdoc_class_name, '\\'));
}

const TypeHint *PhpDocTypeRuleParser::parse_simple_type() {
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
      const TypeHint *type_hint = parse_type_expression();
      if (cur_tok->type() != tok_clpar) {
        throw std::runtime_error("unmatching ()");
      }
      cur_tok++;
      return type_hint;
    }
    case tok_int:
      cur_tok++;
      return TypeHintPrimitive::create(tp_int);
    case tok_bool:
      cur_tok++;
      return TypeHintPrimitive::create(tp_bool);
    case tok_float:
      cur_tok++;
      return TypeHintPrimitive::create(tp_float);
    case tok_string:
      cur_tok++;
      return TypeHintPrimitive::create(tp_string);
    case tok_false:
      cur_tok++;
      return TypeHintPrimitive::create(tp_False);
    case tok_true:
      cur_tok++;
      return TypeHintPrimitive::create(tp_bool);
    case tok_null:
      cur_tok++;
      return TypeHintPrimitive::create(tp_Null);
    case tok_mixed:
      cur_tok++;
      return TypeHintPrimitive::create(tp_mixed);
    case tok_void:
      cur_tok++;
      return TypeHintPrimitive::create(tp_void);
    case tok_tuple:
      cur_tok++;
      return TypeHintTuple::create(parse_nested_type_hints());
    case tok_shape:
      cur_tok++;
      return parse_shape_type();
    case tok_callable:
      cur_tok++;
      if (cur_tok->type() == tok_oppar) {    // callable(...) : ... — typed callable
        return parse_typed_callable();
      }
      return TypeHintCallable::create_untyped_callable();
    case tok_array:
      cur_tok++;
      if (vk::any_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {   // array<...>
        return TypeHintArray::create(parse_nested_one_type_hint());
      }
      return TypeHintArray::create_array_of_any();
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
      return TypeHintOptional::create(parse_type_expression(), true, false);
    case tok_object:
      cur_tok++;
      return TypeHintPrimitive::create(tp_any);

    case tok_static:
    case tok_func_name:
      // tok_future doesn't exist, it's a string
      if (vk::any_of_equal(cur_tok->str_val, "future", "\\future")) {
        cur_tok++;
        return TypeHintFuture::create(tp_future, parse_nested_one_type_hint());
      }
      // same for the future_queue
      if (cur_tok->str_val == "future_queue") {
        cur_tok++;
        return TypeHintFuture::create(tp_future_queue, parse_nested_one_type_hint());
      }
      // plus some extra types that are not tokens as well, but they make sense inside the phpdoc/functions.txt
      if (cur_tok->str_val == "any") {
        cur_tok++;
        return TypeHintPrimitive::create(tp_any);
      }
      if (cur_tok->str_val == "regexp") {
        cur_tok++;
        return TypeHintPrimitive::create(tp_regexp);
      }
      // force(T) = T (for PhpStorm)
      if (cur_tok->str_val == "force") {
        cur_tok++;
        return parse_nested_one_type_hint();
      }
      // (for functions.txt) instance<^2>
      if (cur_tok->str_val == "instance") {
        cur_tok++;
        const auto *nested = dynamic_cast<const TypeHintArgRef *>(parse_nested_one_type_hint());
        kphp_assert(nested);
        return TypeHintArgRefInstance::create(nested->arg_num);
      }
      // otherwise interpreted as a class name (including the lowercase names);
      // it works with the absolute and relative names as well as for a special names like 'self';
      // file 'use' directives are taken into the account
      return parse_classname(std::string(cur_tok->str_val));

    default:
      throw std::runtime_error(fmt_format("can't parse '{}'", cur_tok->str_val));
  }
}

const TypeHint *PhpDocTypeRuleParser::parse_arg_ref() {   // ^1, ^2[*][*], ^3()
  if (cur_tok->type() != tok_int_const) {
    throw std::runtime_error("Invalid number after ^");
  }
  int arg_num = std::stoi(std::string(cur_tok->str_val));
  cur_tok++;

  if (cur_tok->type() == tok_oppar && (cur_tok + 1)->type() == tok_clpar) {
    cur_tok += 2;
    return TypeHintArgRefCallbackCall::create(arg_num);
  }

  const TypeHint *res = TypeHintArgRef::create(arg_num);
  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_times && (cur_tok + 2)->type() == tok_clbrk) {
    res = TypeHintArgSubkeyGet::create(res);
    cur_tok += 3;
  }

  return res;
}

const TypeHint *PhpDocTypeRuleParser::parse_type_array() {
  const TypeHint *inner = parse_simple_type();

  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_clbrk) {
    inner = TypeHintArray::create(inner);
    cur_tok += 2;
  }

  return inner;
}

std::vector<const TypeHint *> PhpDocTypeRuleParser::parse_nested_type_hints() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;
  std::vector<const TypeHint *> sub_types;
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

const TypeHint *PhpDocTypeRuleParser::parse_nested_one_type_hint() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;

  const TypeHint *sub_type = parse_type_expression();
  if (vk::none_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
    throw std::runtime_error("expected ')'");
  }
  cur_tok++;

  return sub_type;
}

const TypeHint *PhpDocTypeRuleParser::parse_typed_callable() {  // callable(int, int):?string, callable(int), callable():void
  if (cur_tok->type() != tok_oppar) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;

  std::vector<const TypeHint *> arg_types;
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

  const TypeHint *return_type;
  if (cur_tok->type() == tok_colon) {
    cur_tok++;
    return_type = parse_type_expression();
  } else {
    return_type = TypeHintPrimitive::create(tp_void);
  }

  return TypeHintCallable::create(std::move(arg_types), return_type);
}

const TypeHint *PhpDocTypeRuleParser::parse_shape_type() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '('");
  }
  cur_tok++;

  std::vector<std::pair<std::string, const TypeHint *>> shape_items;
  bool has_varg = false;
  // example: shape(x:int, y?:\A, z:tuple(...))
  // elements are [ op_double_arrow ( op_func_name "x", TypeHint "int" ), ... ]
  // may end with tok_varg: shape(x:int, ...)
  while (true) {
    std::string elem_name = static_cast<std::string>(cur_tok->str_val);
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

    const TypeHint *elem_type_hint = parse_type_expression();
    if (is_nullable) {
      elem_type_hint = TypeHintOptional::create(elem_type_hint, true, false);
    }

    shape_items.emplace_back(std::make_pair(elem_name, elem_type_hint));

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

  return TypeHintShape::create(std::move(shape_items), has_varg);
}

const TypeHint *PhpDocTypeRuleParser::parse_type_expression() {
  const TypeHint *result = parse_type_array();
  if (cur_tok->type() != tok_or) {
    return result;
  }

  std::vector<const TypeHint *> items;
  bool was_raw_bool = false;
  bool was_false = false;
  bool was_null = false;
  
  auto on_each_item = [&](const TypeHint *item) {
    items.emplace_back(item);
    if (const auto *as_primitive = item->try_as<TypeHintPrimitive>()) {
      was_raw_bool |= as_primitive->ptype == tp_bool && (cur_tok - 1)->type() == tok_bool;
      was_false |= as_primitive->ptype == tp_False;
      was_null |= as_primitive->ptype == tp_Null;
    }
  };

  on_each_item(result);
  while (cur_tok->type() == tok_or) {
    cur_tok++;
    on_each_item(parse_type_array());

    if (was_raw_bool && !was_null) {
      throw std::runtime_error("Do not use |bool in phpdoc, use |false instead\n(if you really need bool, specify |boolean)");
    }
  }

  // try to simplify: T|false and similar as an optional<T>, not as a vector
  bool can_be_simplified_as_optional = 1 == (items.size() - was_false - was_null);
  if (can_be_simplified_as_optional) {
    for (const TypeHint *item : items) {
      const auto *as_primitive = item->try_as<TypeHintPrimitive>();
      if (!as_primitive || vk::none_of_equal(as_primitive->ptype, tp_Null, tp_False)) {
        return TypeHintOptional::create(item, was_null, was_false);
      }
    }
  }

  return TypeHintPipe::create(std::move(items));
}

const TypeHint *PhpDocTypeRuleParser::parse_from_tokens(std::vector<Token>::const_iterator &tok_iter) {
  cur_tok = tok_iter;
  const TypeHint *v = parse_type_expression();      // could throw an exception

  // "?int ...$args" == "(?int)[] $args", "int|false ...$a" == "(int|false)[] $a"; handle "..." after all has been parsed
  if (cur_tok->type() == tok_varg) {
    cur_tok++;
    v = TypeHintArray::create(v);
  }

  tok_iter = cur_tok;
  return v;                                   // not null if an exception was not thrown
}

const TypeHint *PhpDocTypeRuleParser::parse_from_tokens_silent(std::vector<Token>::const_iterator &tok_iter) noexcept {
  try {
    return parse_from_tokens(tok_iter);
  } catch (std::runtime_error &) {
    return {};
  }
}

/*
 * With a phpdoc string after a @param/@return/@var like "int|false $a maybe comment"
 * or "$var tuple(int, string) maybe comment" or "A[] maybe comment"
 * parse a type (turn it into the TypeHint tree representation) and a variable name (if present).
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
      return {nullptr, std::move(var_name)};
    }
  }

  PhpDocTypeRuleParser parser(current_function);
  const TypeHint *type_hint{nullptr};
  try {
    type_hint = parser.parse_from_tokens(tok_iter);
  } catch (std::runtime_error &ex) {
    stage::set_location(current_function->root->location);
    kphp_error(0, fmt_format("{}: {}\n{}",
                             TermStringFormat::paint_red(TermStringFormat::add_text_attribute("Could not parse phpdoc tag", TermStringFormat::bold)),
                             TermStringFormat::add_text_attribute(std::string(phpdoc_tag_str), TermStringFormat::underline),
                             ex.what()));
  }

  if (!type_hint) {
    return {nullptr, std::move(var_name)};
  }

  if (var_name.empty()) {
    kphp_assert(tok_iter != tokens.end());
    // phpdoc|type $var_name maybe comment
    if (tok_iter->type() == tok_var_name) {   // tok_iter — right after the parsing is finished
      var_name = std::string(tok_iter->str_val);
    }
  }

  return {type_hint, std::move(var_name)};
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
  return {nullptr, std::string()};
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

/*
 * When phpdoc/type hint has just been parsed and inherited, it's ready be to saved as a function/field property.
 * Here we do final checks before saving it as immutable:
 * 1) Check that all classes inside are resolved
 * 2) Replace self/static/parent with actual context
 * If it contains an error, it is printed with current location context is supposed to also be printed on the caller side.
 */
const TypeHint *phpdoc_finalize_type_hint_and_resolve(const TypeHint *type_hint, FunctionPtr resolve_context) {
  if (!type_hint || !type_hint->has_instances_inside()) {
    return type_hint;
  }

  if (type_hint->has_self_static_parent_inside()) {
    type_hint = type_hint->replace_self_static_parent(resolve_context);
    kphp_assert(!type_hint->has_self_static_parent_inside());
  }

  bool all_resolved = true;

  type_hint->traverse([&all_resolved](const TypeHint *child) {
    if (const auto *as_instance = child->try_as<TypeHintInstance>()) {

      ClassPtr klass = G->get_class(as_instance->full_class_name);
      if (!klass) {
        all_resolved = false;
        kphp_error(0, fmt_format("Could not find class {}", TermStringFormat::paint_red(as_instance->full_class_name)));
      } else {
        kphp_error(!klass->is_trait(), "You may not use trait as a type-hint");
      }

    }
  });

  return all_resolved ? type_hint : nullptr;
}

// when a field has neither @var not type hint, we use the default value initializer like a type guard
// here we convert this value initializer (init_val) to a TypeHint, like a phpdoc was actually written
const TypeHint *phpdoc_convert_default_value_to_type_hint(VertexPtr init_val) {
  switch (init_val->type()) {
    case op_int_const:
    case op_conv_int:
      return TypeHintPrimitive::create(tp_int);
    case op_string:
    case op_string_build:
    case op_concat:
      return TypeHintPrimitive::create(tp_string);
    case op_float_const:
    case op_conv_float:
      return TypeHintPrimitive::create(tp_float);
    case op_array:
    case op_conv_array:
      // an array as a default => like "array" as a type hint, meaning "array of any", regardless of elements values
      return TypeHintArray::create_array_of_any();
    case op_true:
    case op_false:
      return TypeHintPrimitive::create(tp_bool);
    case op_mul:
    case op_sub:
    case op_plus: {
      const auto *lhs_type_hint = phpdoc_convert_default_value_to_type_hint(init_val.as<meta_op_binary>()->lhs())->try_as<TypeHintPrimitive>();
      const auto *rhs_type_hint = phpdoc_convert_default_value_to_type_hint(init_val.as<meta_op_binary>()->rhs())->try_as<TypeHintPrimitive>();
      if (lhs_type_hint && rhs_type_hint) {
        if (lhs_type_hint->ptype == tp_float || rhs_type_hint->ptype == tp_float) {
          return TypeHintPrimitive::create(tp_float);
        } else if (lhs_type_hint->ptype == tp_int && rhs_type_hint->ptype == tp_int) {
          return TypeHintPrimitive::create(tp_int);
        }
      }
      return {};
    }
    case op_div:
      return TypeHintPrimitive::create(tp_float);
      
    default:
      // 'null' or something strange as a default — can't detect, will report "specify @var manually"
      return {};
  }
}
