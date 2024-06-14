// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/phpdoc.h"

#include <cstdio>
#include <utility>

#include "common/termformat/termformat.h"
#include "common/php-functions.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/generics-mixins.h"
#include "compiler/lexer.h"
#include "compiler/modulite-check-rules.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"
#include "compiler/type-hint.h"
#include "compiler/utils/string-utils.h"
#include "compiler/ffi/ffi_parser.h"
#include "compiler/vertex-util.h"


static constexpr unsigned int calcHashOfTagName(const char *start, const char *end) {
  unsigned int hash = 5381;
  for (const char *c = start; c != end; ++c) {
    hash = (hash << 5) + hash + *c;
  }
  return hash;
}

struct KnownPhpDocTag {
  unsigned int hash;
  PhpDocType type;
  const char *tag_name;

  constexpr KnownPhpDocTag(const char *tag_name, PhpDocType type)
    : hash(calcHashOfTagName(tag_name, tag_name + __builtin_strlen(tag_name))), type(type), tag_name(tag_name) {}
};

class AllDocTags {
  static constexpr int N_TAGS = 41;
  static const KnownPhpDocTag ALL_TAGS[N_TAGS];

public:
  [[gnu::always_inline]] static PhpDocType name2type(const char *start, const char *end) {
    unsigned int hash = calcHashOfTagName(start, end);

    for (const KnownPhpDocTag &tag: ALL_TAGS) {
      if (tag.hash == hash) {
        return tag.type;
      }
    }
    return PhpDocType::unknown;
  }

  static const char *type2name(PhpDocType type) {
    for (const KnownPhpDocTag &tag: ALL_TAGS) {
      if (tag.type == type) {
        return tag.tag_name;
      }
    }
    return "@tag";
  }
};

const KnownPhpDocTag AllDocTags::ALL_TAGS[] = {
  KnownPhpDocTag("@param", PhpDocType::param),
  KnownPhpDocTag("@var", PhpDocType::var),
  KnownPhpDocTag("@return", PhpDocType::returns),
  KnownPhpDocTag("@type", PhpDocType::var),
  KnownPhpDocTag("@returns", PhpDocType::returns),
  KnownPhpDocTag("@kphp-inline", PhpDocType::kphp_inline),
  KnownPhpDocTag("@kphp-flatten", PhpDocType::kphp_flatten),
  KnownPhpDocTag("@kphp-infer", PhpDocType::kphp_infer),
  KnownPhpDocTag("@kphp-required", PhpDocType::kphp_required),
  KnownPhpDocTag("@kphp-lib-export", PhpDocType::kphp_lib_export),
  KnownPhpDocTag("@kphp-sync", PhpDocType::kphp_sync),
  KnownPhpDocTag("@kphp-should-not-throw", PhpDocType::kphp_should_not_throw),
  KnownPhpDocTag("@kphp-throws", PhpDocType::kphp_throws),
  KnownPhpDocTag("@kphp-disable-warnings", PhpDocType::kphp_disable_warnings),
  KnownPhpDocTag("@kphp-extern-func-info", PhpDocType::kphp_extern_func_info),
  KnownPhpDocTag("@kphp-pure-function", PhpDocType::kphp_pure_function),
  KnownPhpDocTag("@kphp-template", PhpDocType::kphp_template),
  KnownPhpDocTag("@kphp-generic", PhpDocType::kphp_generic),
  KnownPhpDocTag("@kphp-tracing", PhpDocType::kphp_tracing),
  KnownPhpDocTag("@kphp-param", PhpDocType::kphp_param),
  KnownPhpDocTag("@kphp-return", PhpDocType::kphp_return),
  KnownPhpDocTag("@kphp-memcache-class", PhpDocType::kphp_memcache_class),
  KnownPhpDocTag("@kphp-immutable-class", PhpDocType::kphp_immutable_class),
  KnownPhpDocTag("@kphp-tl-class", PhpDocType::kphp_tl_class),
  KnownPhpDocTag("@kphp-const", PhpDocType::kphp_const),
  KnownPhpDocTag("@kphp-no-return", PhpDocType::kphp_noreturn),
  KnownPhpDocTag("@kphp-warn-unused-result", PhpDocType::kphp_warn_unused_result),
  KnownPhpDocTag("@kphp-warn-performance", PhpDocType::kphp_warn_performance),
  KnownPhpDocTag("@kphp-analyze-performance", PhpDocType::kphp_analyze_performance),
  KnownPhpDocTag("@kphp-serializable", PhpDocType::kphp_serializable),
  KnownPhpDocTag("@kphp-reserved-fields", PhpDocType::kphp_reserved_fields),
  KnownPhpDocTag("@kphp-serialized-field", PhpDocType::kphp_serialized_field),
  KnownPhpDocTag("@kphp-serialized-float32", PhpDocType::kphp_serialized_float32),
  KnownPhpDocTag("@kphp-json", PhpDocType::kphp_json),
  KnownPhpDocTag("@kphp-profile", PhpDocType::kphp_profile),
  KnownPhpDocTag("@kphp-profile-allow-inline", PhpDocType::kphp_profile_allow_inline),
  KnownPhpDocTag("@kphp-strict-types-enable", PhpDocType::kphp_strict_types_enable),
  KnownPhpDocTag("@kphp-color", PhpDocType::kphp_color),
  KnownPhpDocTag("@kphp-internal-result-indexing", PhpDocType::kphp_internal_result_indexing),
  KnownPhpDocTag("@kphp-internal-result-array2tuple", PhpDocType::kphp_internal_result_array2tuple),
  KnownPhpDocTag("@kphp-internal-param-readonly", PhpDocType::kphp_internal_param_readonly),
};


PhpDocComment::PhpDocComment(vk::string_view phpdoc_str) {
  const char *c = phpdoc_str.data();
  const char *end = phpdoc_str.end();
  auto iter_next = tags.before_begin();

  while (c != end) {
    // we are at line start, waiting for '*' after spaces
    if (*c == ' ' || *c == '\t' || *c == '\n') {
      ++c;
      continue;
    }
    // if not '*', skip this line
    if (*c != '*') {
      while (c != end && *c != '\n') {
        ++c;
      }
      continue;
    }
    // wait for '@' after spaces
    ++c;
    while (c != end && *c == ' ') {
      ++c;
    }
    // if not @ after *, skip this line
    if (*c != '@') {
      while (c != end && *c != '\n') {
        ++c;
      }
      continue;
    }

    // c points to '@', read tag name until space
    const char *start = c;
    while (c != end && *c != ' ' && *c != '\n' && *c != '\t') {
      ++c;
    }

    // convert @tag-name to enum; linear search is better, since most common cases are placed at the top
    PhpDocType type = AllDocTags::name2type(start, c);
    if (type == PhpDocType::unknown && vk::string_view{start, 5}.starts_with("@kphp")) {
      kphp_error(0, fmt_format("unrecognized @kphp tag: '{}'", std::string(start, c)));
    }

    // after @tag-name, there are spaces and a tag value until end of line
    while (c != end && *c == ' ') {
      ++c;
    }
    start = c;
    while (c != end && *c != '\n') {
      ++c;
    }

    // append tag to the end of forward list
    iter_next = tags.emplace_after(iter_next, type, vk::string_view(start, c));
  }
}

std::string PhpDocTag::get_tag_name() const noexcept {
  return AllDocTags::type2name(type);
}

/*
 * With a phpdoc string after a @param/@return/@var like "int|false $a maybe comment"
 * or "$var tuple(int, string) maybe comment" or "A[] maybe comment"
 * parse a type (turn it into the TypeHint tree representation) and a variable name (if present).
 */
PhpDocTag::TypeAndVarName PhpDocTag::value_as_type_and_var_name(FunctionPtr current_function, const GenericsDeclarationMixin *genericTs) const {
  std::vector<Token> tokens = phpdoc_to_tokens(value);
  auto tok_iter = tokens.cbegin();
  vk::string_view var_name;

  // $var_name phpdoc|type maybe comment
  if (tokens.front().type() == tok_var_name) {
    var_name = tokens.front().str_val;
    tok_iter++;
    if (tokens.size() <= 2) {     // only tok_end is left
      return {nullptr, var_name};
    }
  }

  PhpDocTypeHintParser parser(current_function, genericTs);
  const TypeHint *type_hint{nullptr};
  try {
    type_hint = parser.parse_from_tokens(tok_iter);
  } catch (std::runtime_error &ex) {
    kphp_error(0, fmt_format("{}: {}\n{}",
                             TermStringFormat::paint_red(TermStringFormat::add_text_attribute("Could not parse " + get_tag_name(), TermStringFormat::bold)),
                             TermStringFormat::add_text_attribute(value_as_string(), TermStringFormat::underline),
                             ex.what()));
  }

  if (!type_hint) {
    return {nullptr, var_name};
  }

  if (var_name.empty()) {
    kphp_assert(tok_iter != tokens.end());
    // phpdoc|type $var_name maybe comment
    if (tok_iter->type() == tok_var_name) {   // tok_iter — right after the parsing is finished
      var_name = tok_iter->str_val;
    }
  }

  return {type_hint, var_name};
}


const TypeHint *PhpDocTypeHintParser::parse_classname(const std::string &phpdoc_class_name) {
  cur_tok++;
  // here we have a relative class name, that can be resolved into full unless self/static/parent
  // if self/static/parent, it will be resolved at context, see phpdoc_finalize_type_hint_and_resolve()
  if (is_string_self_static_parent(phpdoc_class_name)) {
    return TypeHintInstance::create(phpdoc_class_name);
  }
  if (genericTs != nullptr && genericTs->has_nameT(phpdoc_class_name)) {
    return TypeHintGenericT::create(phpdoc_class_name);
  }
  return TypeHintInstance::create(resolve_uses(current_function, phpdoc_class_name));
}

const TypeHint *PhpDocTypeHintParser::parse_ffi_cdata() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '<' or '('");
  }
  cur_tok++;
  if (cur_tok->type() != tok_func_name) {
    throw std::runtime_error("expected scope name identifier as 1st CData type param");
  }
  std::string scope_name = static_cast<std::string>(cur_tok->str_val);
  cur_tok++;
  if (cur_tok->type() != tok_comma) {
    throw std::runtime_error("expected ','");
  }
  cur_tok++;

  // this section is weird: we need to construct a complete C declaration string here,
  // but our tokenizer doesn't give an easy way to do so;
  // this is why we're reconstructing a string manually here to re-parse it with C parser
  std::string cdef;
  cdef.reserve(32); // long enough to store something like "struct SomeImportantType**"
  while (true) {
    auto tok = cur_tok->type();
    if (vk::any_of_equal(tok, tok_gt, tok_clpar)) {
      break;
    }
    bool need_space = false;
    if (tok == tok_func_name) {
      cdef.append(cur_tok->str_val.begin(), cur_tok->str_val.end());
      need_space = true;
    } else if (tok == tok_int_const) {
      cdef.append(cur_tok->str_val.begin(), cur_tok->str_val.end());
    } else if (tok == tok_void) {
      cdef.append("void");
      need_space = true;
    } else if (tok == tok_int) {
      cdef.append("int");
      need_space = true;
    } else if (tok == tok_float) {
      cdef.append("float");
      need_space = true;
    } else if (tok == tok_double) {
      cdef.append("double");
      need_space = true;
    } else if (tok == tok_const) {
      cdef.append("const");
      need_space = true;
    } else if (tok == tok_times) {
      cdef.push_back('*');
    }  else if (tok == tok_opbrk) {
      cdef.push_back('[');
    } else if (tok == tok_clbrk) {
      cdef.push_back(']');
    } else {
      throw std::runtime_error("unexpected token, expected C type, '>' or ')'");
    }
    if (need_space) {
      cdef.push_back(' ');
    }
    cur_tok++;
  }
  cur_tok++; // consume '>' or ')'

  FFITypedefs typedefs; // will remain empty
  auto [type, err] = ffi_parse_type(cdef, typedefs);
  if (!err.message.empty()) {
    throw std::runtime_error(fmt_format("parse ffi_cdata<{}, {}>: {}", scope_name, cdef, err.message));
  }

  // since we don't know whether create() will actually use newly allocated FFI types,
  // we transfer the ownership: if it will reuse previously created type hint,
  // our FFI type will be deallocated
  bool transfer_ownership = true;
  return TypeHintFFIType::create(scope_name, type, transfer_ownership);
}

const TypeHint *PhpDocTypeHintParser::parse_ffi_scope() {
  if (vk::none_of_equal(cur_tok->type(), tok_lt, tok_oppar)) {
    throw std::runtime_error("expected '<' or '('");
  }
  cur_tok++;
  vk::string_view scope_name;
  int arg_num = 0;
  if (cur_tok->type() == tok_func_name) {
    scope_name = cur_tok->str_val;
  } else if (cur_tok->type() == tok_xor) {
    cur_tok++;
    if (cur_tok->type() != tok_int_const) {
      throw std::runtime_error("Invalid number after ^");
    }
    arg_num = std::stoi(std::string(cur_tok->str_val));
  } else {
    throw std::runtime_error("expected scope name identifier");
  }
  cur_tok++;
  if (vk::none_of_equal(cur_tok->type(), tok_gt, tok_clpar)) {
    throw std::runtime_error("expected '>' or ')'");
  }
  cur_tok++;
  if (scope_name.empty()) {
    return TypeHintFFIScopeArgRef::create(arg_num);
  }
  return TypeHintFFIScope::create(std::string(scope_name));
}

const TypeHint *PhpDocTypeHintParser::parse_simple_type() {
  TokenType cur_type = cur_tok->type();
  // some type names inside phpdoc are not keywords/tokens, but they should be interpreted as such
  if (cur_type == tok_func_name) {
    if (cur_tok->str_val == "\\tuple") {
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
      [[fallthrough]];
    case tok_boolean:
      cur_tok++;
      return TypeHintPrimitive::create(tp_bool);
    case tok_float:
    case tok_double:
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
        const auto &type_hints = parse_nested_type_hints();
        if (type_hints.empty() || type_hints.size() > 2) {
          throw std::runtime_error{"array<...> has to be parameterized with one type or two types at most"};
        }
        // just ignore array's key type if there is any
        return TypeHintArray::create(type_hints.back());
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
      return TypeHintObject::create();
    case tok_class:
      if ((cur_tok + 1)->type() == tok_minus && (cur_tok + 2)->type() == tok_string) {  // class-string<T>
        cur_tok += 3;
        const auto *nested = parse_nested_one_type_hint();
        if (!nested->try_as<TypeHintGenericT>()) {
          throw std::runtime_error("class-string<> should contain generic T inside");
        }
        return TypeHintClassString::create(nested);
      }
      break;

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
      if (cur_tok->str_val == "ffi_cdata") {
        cur_tok++;
        return parse_ffi_cdata();
      }
      if (cur_tok->str_val == "ffi_scope") {
        cur_tok++;
        return parse_ffi_scope();
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
      if (cur_tok->str_val == "_tmp_string") {
        cur_tok++;
        if (!current_function->is_extern()) {
          throw std::runtime_error("_tmp_string type can only be used for KPHP builtin functions");
        }
        return TypeHintPrimitive::create(tp_tmp_string);
      }
      if (cur_tok->str_val == "not_null") {
        cur_tok++;
        return TypeHintNotNull::create(parse_nested_one_type_hint(), true, false);
      }
      if (cur_tok->str_val == "not_false") {
        cur_tok++;
        return TypeHintNotNull::create(parse_nested_one_type_hint(), false, true);
      }
      // otherwise interpreted as a class name (including the lowercase names);
      // it works with the absolute and relative names as well as for a special names like 'self';
      // file 'use' directives are taken into the account
      return parse_classname(std::string(cur_tok->str_val));

    default:
      break;
  }

  throw std::runtime_error(fmt_format("can't parse '{}'", cur_tok->str_val));
}

const TypeHint *PhpDocTypeHintParser::parse_arg_ref() {   // ^1, ^2[*][*], ^3()
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

const TypeHint *PhpDocTypeHintParser::parse_type_array() {
  const TypeHint *inner = parse_simple_type();

  if (cur_tok->type() == tok_double_colon) {      // T::methodName(), T::fieldName
    cur_tok++;
    if (cur_tok->type() == tok_func_name && (cur_tok + 1)->type() == tok_oppar && (cur_tok + 2)->type() == tok_clpar) {
      inner = TypeHintRefToMethod::create(inner, std::string(cur_tok->str_val));
      cur_tok += 3;
    } else if (cur_tok->type() == tok_func_name) {
      inner = TypeHintRefToField::create(inner, std::string(cur_tok->str_val));
      cur_tok += 1;
    } else {
      throw std::runtime_error("Invalid syntax after ::");
    }
  }

  while (cur_tok->type() == tok_opbrk && (cur_tok + 1)->type() == tok_clbrk) {
    inner = TypeHintArray::create(inner);
    cur_tok += 2;
  }

  return inner;
}

std::vector<const TypeHint *> PhpDocTypeHintParser::parse_nested_type_hints() {
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

const TypeHint *PhpDocTypeHintParser::parse_nested_one_type_hint() {
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

const TypeHint *PhpDocTypeHintParser::parse_typed_callable() {  // callable(int, int):?string, callable(int $x), callable():void
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

    if (cur_tok->type() == tok_var_name) {  // callable(int $x) — var name is like a comment in declaration
      cur_tok++;
    }
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

const TypeHint *PhpDocTypeHintParser::parse_shape_type() {
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
    const auto elem_name = static_cast<std::string>(cur_tok->str_val);
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
      const std::int64_t hash = string_hash(elem_name.data(), elem_name.size());
      // nullable shape's key that is never used in code will not be present in shapes key storage,
      // so we will get an error while trying to print it via to_array_debug(), thus it added here
      TypeHintShape::register_known_key(hash, elem_name);
    }

    shape_items.emplace_back(elem_name, elem_type_hint);

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

const TypeHint *PhpDocTypeHintParser::parse_type_expression() {
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

const TypeHint *PhpDocTypeHintParser::parse_from_tokens(std::vector<Token>::const_iterator &tok_iter) {
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

const TypeHint *PhpDocTypeHintParser::parse_from_tokens_silent(std::vector<Token>::const_iterator &tok_iter) noexcept {
  try {
    return parse_from_tokens(tok_iter);
  } catch (std::runtime_error &) {
    return {};
  }
}

/*
 * When phpdoc/type hint has just been parsed and inherited, it's ready be to saved as a function/field property.
 * Here we do final checks before saving it as immutable:
 * 1) Check that all classes inside are resolved
 * 2) Replace self/static/parent with actual context
 * 3) Generate interfaces for typed callables
 * If it contains an error, it is printed with current location context is supposed to also be printed on the caller side.
 */
const TypeHint *phpdoc_finalize_type_hint_and_resolve(const TypeHint *type_hint, FunctionPtr resolve_context) {
  if (!type_hint || resolve_context->file_id->is_builtin()) {
    return type_hint;
  }

  bool all_resolved = true;

  if (type_hint->has_self_static_parent_inside()) {
    type_hint = type_hint->replace_self_static_parent(resolve_context);
    kphp_assert(!type_hint->has_self_static_parent_inside());
  }

  if (type_hint->has_instances_inside()) {
    type_hint->traverse([&all_resolved, resolve_context](const TypeHint *child) {
      if (const auto *as_instance = child->try_as<TypeHintInstance>()) {
        ClassPtr klass = as_instance->resolve();
        if (!klass) {
          all_resolved = false;
          kphp_error_return(0, fmt_format("Could not find class {}", TermStringFormat::paint_red(as_instance->full_class_name)));
        } else {
          kphp_error(!klass->is_trait(), fmt_format("Using trait {} as a type is invalid (traits are not types)", as_instance->full_class_name));
        }

        if (resolve_context->modulite != klass->modulite) {
          modulite_check_when_use_class(resolve_context, klass);
        }

      } else if (const auto *as_scope = child->try_as<TypeHintFFIScope>()) {
        ClassPtr klass = G->get_class(FFIRoot::scope_class_name(as_scope->scope_name));
        kphp_error_return(klass, fmt_format("Could not find ffi_scope<{}>", as_scope->scope_name));

      } else if (const auto *as_ffi = child->try_as<TypeHintFFIType>()) {
        if (vk::any_of_equal(as_ffi->type->kind, FFITypeKind::Struct, FFITypeKind::Union)) {
          ClassPtr klass = G->get_class(FFIRoot::cdata_class_name(as_ffi->scope_name, as_ffi->type->str));
          kphp_error_return(klass, fmt_format("Could not find ffi_cdata<{}, {}>", as_ffi->scope_name, as_ffi->type->str));
          bool tags_ok = (as_ffi->type->kind == FFITypeKind::Struct && klass->ffi_class_mixin->ffi_type->kind == FFITypeKind::StructDef) ||
                         (as_ffi->type->kind == FFITypeKind::Union && klass->ffi_class_mixin->ffi_type->kind == FFITypeKind::UnionDef);
          kphp_error_return(tags_ok, fmt_format("Mismatched union/struct tag for ffi_cdata<{}, {}>", as_ffi->scope_name, as_ffi->type->str));
        }
      }
    });
  }

  if (type_hint->has_autogeneric_inside()) {
    // 'callable' and 'object' are allowed only standalone; fire an error for 'callable[]' and other nested
    kphp_error(type_hint->try_as<TypeHintObject>() ||
               (type_hint->try_as<TypeHintCallable>() && type_hint->try_as<TypeHintCallable>()->is_untyped_callable()),
               "Keywords 'callable' and 'object' have special treatment, they are not real types.\nThey can be used for a parameter, implicitly converting a function into generic.\nThey can NOT be used inside arrays, tuples, etc. — only as a standalone keyword.\nConsider using typed callables instead of 'callable', and generic functions/classes instead of 'object'.");
  }

  if (type_hint->has_callables_inside()) {
    type_hint->traverse([](const TypeHint *child) {
      if (const auto *as_callable = child->try_as<TypeHintCallable>()) {

        if (as_callable->is_typed_callable() && !as_callable->has_genericT_inside()) {
          as_callable->get_interface();
        }

      }
    });
  }

  return all_resolved ? type_hint : nullptr;
}

/*
 * When we have a generic function <T> and an instantiation T=User, replace all T's inside a type hint.
 * We also handle @return here, that can ref to a field T::fieldName, replacing it after T is instantiated.
 * (note, that we don't allow ClassName::fieldName anywhere but in generics; if it's written, it will fail on type inferring)
 */
const TypeHint *phpdoc_replace_genericTs_with_reified(const TypeHint *type_hint, const GenericsInstantiationMixin *reifiedTs) {
  if (!type_hint->has_genericT_inside()) {
    return type_hint;
  }

  return type_hint->replace_children_custom([reifiedTs](const TypeHint *child) {

    if (const auto *as_genericT = child->try_as<TypeHintGenericT>()) {
      const TypeHint *replacement = reifiedTs->find(as_genericT->nameT);
      return replacement ?: child;
    }
    if (const auto *as_field_ref = child->try_as<TypeHintRefToField>()) {
      const TypeHint *field_type_hint = as_field_ref->resolve_field_type_hint();
      kphp_error(field_type_hint, "Could not detect a field that :: points to in phpdoc white instantiating generics");
      return field_type_hint ?: TypeHintPrimitive::create(tp_any);
    }
    if (const auto *as_field_ref = child->try_as<TypeHintRefToMethod>()) {
      FunctionPtr method = as_field_ref->resolve_method();
      kphp_error(method, "Could not detect a method that :: points to in phpdoc white instantiating generics");
      return method && method->return_typehint ? method->return_typehint : TypeHintPrimitive::create(tp_any);
    }

    return child;
  });
}

// when a field has neither @var not type hint, we use the default value initializer like a type guard
// here we convert this value initializer (init_val) to a TypeHint, like a phpdoc was actually written
const TypeHint *phpdoc_convert_default_value_to_type_hint(VertexPtr init_val) {
  switch (init_val->type()) {
    case op_define_val:
      return phpdoc_convert_default_value_to_type_hint(init_val.as<op_define_val>()->value());
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
