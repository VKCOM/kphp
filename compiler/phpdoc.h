// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <forward_list>
#include <vector>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"


enum class PhpDocType {
  unknown,
  kphp_inline,
  kphp_infer,
  kphp_disable_warnings,
  kphp_extern_func_info,
  kphp_pure_function,
  param,
  returns,
  var,
  kphp_required,
  kphp_lib_export,
  kphp_sync,
  kphp_should_not_throw,
  kphp_throws,
  kphp_template,
  kphp_generic,
  kphp_tracing,
  kphp_param,
  kphp_return,
  kphp_memcache_class,
  kphp_immutable_class,
  kphp_tl_class,
  kphp_const,
  kphp_noreturn,
  kphp_warn_unused_result,
  kphp_warn_performance,
  kphp_analyze_performance,
  kphp_flatten,
  kphp_serializable,
  kphp_reserved_fields,
  kphp_serialized_field,
  kphp_serialized_float32,
  kphp_json,
  kphp_profile,
  kphp_profile_allow_inline,
  kphp_strict_types_enable, // TODO: remove when strict_types=1 are enabled by default
  kphp_color,
  kphp_internal_result_indexing,
  kphp_internal_result_array2tuple,
  kphp_internal_param_readonly,
};

class GenericsDeclarationMixin;

// one phpdoc contains of several doc tags
// every doc tag has a type (enum mapping @tag-name) and a value
struct PhpDocTag {
  // if a tag value is like "int|false $var_name" (for @param / @var / etc.),
  // it's represented as this structure
  struct TypeAndVarName {
    const TypeHint *type_hint{nullptr};
    vk::string_view var_name;     // stored without leading "$"; could be empty if omitted in phpdoc

    operator bool() const noexcept { return static_cast<bool>(type_hint); }
  };


  PhpDocType type;
  vk::string_view value;

  PhpDocTag(PhpDocType type, vk::string_view value) : type(type), value(value) {}

  std::string get_tag_name() const noexcept;

  std::string value_as_string() const noexcept { return std::string(value); }
  TypeAndVarName value_as_type_and_var_name(FunctionPtr current_function, const GenericsDeclarationMixin *genericTs) const;
};

// a class representing whole php doc comment
// a whole comment is parsed to doc tags immediately in gentree,
// but the values of exact tags are analyzed later
class PhpDocComment {
public:
  std::forward_list<PhpDocTag> tags;

  explicit PhpDocComment(vk::string_view phpdoc_str);

  bool has_tag(PhpDocType type) const noexcept {
    for (const PhpDocTag &tag: tags) {
      if (tag.type == type) {
        return true;
      }
    }
    return false;
  }

  bool has_tag(PhpDocType type, PhpDocType or_type2) const noexcept {
    for (const PhpDocTag &tag: tags) {
      if (tag.type == type || tag.type == or_type2) {
        return true;
      }
    }
    return false;
  }

  const PhpDocTag *find_tag(PhpDocType type) const noexcept {
    for (const PhpDocTag &tag: tags) {
      if (tag.type == type) {
        return &tag;
      }
    }
    return nullptr;
  }
};

// parse "int|false", "A[]" and other types from phpdoc
// such parsing could be done only based on current_function, when we know all classes and function's properties
class PhpDocTypeHintParser {
public:
  explicit PhpDocTypeHintParser(FunctionPtr current_function, const GenericsDeclarationMixin *genericTs)
    : current_function(current_function)
    , genericTs(genericTs) {}

  const TypeHint *parse_from_tokens(std::vector<Token>::const_iterator &tok_iter);
  const TypeHint *parse_from_tokens_silent(std::vector<Token>::const_iterator &tok_iter) noexcept;

private:
  FunctionPtr current_function;
  const GenericsDeclarationMixin *genericTs;
  std::vector<Token>::const_iterator cur_tok;

  const TypeHint *parse_ffi_scope();
  const TypeHint *parse_ffi_cdata();
  const TypeHint *parse_classname(const std::string &phpdoc_class_name);
  const TypeHint *parse_simple_type();
  const TypeHint *parse_arg_ref();
  const TypeHint *parse_type_array();
  std::vector<const TypeHint *> parse_nested_type_hints();
  const TypeHint *parse_shape_type();
  const TypeHint *parse_nested_one_type_hint();
  const TypeHint *parse_typed_callable();
  const TypeHint *parse_type_expression();
};

const TypeHint *phpdoc_finalize_type_hint_and_resolve(const TypeHint *type_hint, FunctionPtr resolve_context);
const TypeHint *phpdoc_replace_genericTs_with_reified(const TypeHint *type_hint, const GenericsInstantiationMixin *reifiedTs);
const TypeHint *phpdoc_convert_default_value_to_type_hint(VertexPtr init_val);
