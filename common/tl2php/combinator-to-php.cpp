// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tl2php/combinator-to-php.h"

#include <algorithm>
#include <unordered_set>

#include "common/tl2php/tl-to-php-classes-converter.h"

namespace vk {
namespace tl {

class AutoPushPop {
public:
  AutoPushPop(CombinatorToPhp &this_converter, php_field_type pushed_type) :
    this_converter_(this_converter),
    pushed_type_(pushed_type) {
    this_converter_.type_stack_.push_back(pushed_type_);
  }

  ~AutoPushPop() {
    assert(!this_converter_.type_stack_.empty());
    assert(this_converter_.type_stack_.back() == pushed_type_);
    this_converter_.type_stack_.pop_back();
    this_converter_.last_processed_type_ = pushed_type_;
  }
private:
  CombinatorToPhp &this_converter_;
  const php_field_type pushed_type_;
};

CombinatorToPhp::CombinatorToPhp(TlToPhpClassesConverter &tl_to_php, const tlo_parsing::combinator &tl_combinator) :
  tl_to_php_(tl_to_php),
  tl_combinator_(tl_combinator) {
}

std::vector<PhpClassField> CombinatorToPhp::args_to_php_class_fields(const std::vector<std::unique_ptr<tlo_parsing::arg>> &args) {
  std::vector<PhpClassField> fields;
  fields.reserve(args.size());
  for (const auto &tl_arg : args) {
    if (!(tl_arg->is_optional())) {
      fields.emplace_back(arg_type_to_php_field(*tl_arg));
    }
  }
  return fields;
}

void CombinatorToPhp::apply_state(const CombinatorToPhp &other) {
  assert(other.last_processed_type_ != php_field_type::t_unknown);
  php_doc_.append(other.php_doc_);
  last_processed_type_ = other.last_processed_type_;
  has_class_inside_ = has_class_inside_ || other.has_class_inside_;
}

PhpClassField CombinatorToPhp::arg_type_to_php_field(const tlo_parsing::arg &combinator_arg) {
  assert(php_doc_.empty());
  assert(type_stack_.empty());
  has_class_inside_ = false;
  last_processed_type_ = php_field_type::t_unknown;
  combinator_arg.type_expr->visit(*this);
  assert(type_stack_.empty());
  assert(last_processed_type_ != php_field_type::t_unknown);
  std::string field_mask_name;
  if ((combinator_arg.is_fields_mask_optional())) {
    const auto *filed_mask_arg = tl_combinator_.get_var_num_arg(combinator_arg.exist_var_num);
    assert(filed_mask_arg);
    field_mask_name = filed_mask_arg->name;
  }

  assert(combinator_arg.idx >= 0);
  std::string field_name = combinator_arg.name.empty()
                           ? "arg" + std::to_string(combinator_arg.idx)
                           : combinator_arg.name;

  return PhpClassField{std::move(field_name), std::move(php_doc_),
                       std::move(field_mask_name), combinator_arg.exist_var_bit,
                       last_processed_type_, has_class_inside_};
}

bool CombinatorToPhp::try_as_primitive_builtin_type(const vk::string_view &tl_type_name) {
  struct PhpTypeName {
    vk::string_view name;
    php_field_type type;
  };
  static const std::unordered_map<vk::string_view, PhpTypeName> builtin_types{
    {"String", PhpTypeName{"string", php_field_type::t_string}},
    {"Int",    PhpTypeName{"int", php_field_type::t_int}},
    {"#",      PhpTypeName{"int", php_field_type::t_int}},
    {"Long",   PhpTypeName{"int", php_field_type::t_int}},
    {"Double", PhpTypeName{"float", php_field_type::t_double}},
    {"Float",  PhpTypeName{"float", php_field_type::t_double}},
    {"Bool",   PhpTypeName{"boolean", php_field_type::t_bool}},
    {"False",  PhpTypeName{"boolean", php_field_type::t_bool}},
    {"True",   PhpTypeName{"boolean", php_field_type::t_bool_true}}
  };

  const auto it = builtin_types.find(tl_type_name);
  const bool found = it != builtin_types.end();
  if (found) {
    last_processed_type_ = it->second.type;
    php_doc_.append(it->second.name.begin(), it->second.name.end());
  }
  return found;
}

bool CombinatorToPhp::try_as_array_type(const tlo_parsing::type_expr &tl_type_expr, const vk::string_view &tl_type_name) {
  static const std::unordered_set<vk::string_view> array_types{
    "Vector", "Tuple", "Dictionary", "IntKeyDictionary", "LongKeyDictionary"
  };
  if (array_types.count(tl_type_name) == 0) {
    return false;
  }
  assert(tl_type_expr.children.size() >= 1);
  as_array_type(*tl_type_expr.children.front());
  return true;
}

void CombinatorToPhp::as_array_type(const tlo_parsing::expr_base &array_item) {
  if (is_template_instantiating()) {
    php_doc_.append("array_");
  }
  {
    AutoPushPop auto_push_pop{*this, php_field_type::t_array};
    array_item.visit(*this);
  }
  if (!is_template_instantiating()) {
    php_doc_.append("[]");
  }
}

bool CombinatorToPhp::try_as_maybe_type(const tlo_parsing::type_expr &tl_type_expr, const vk::string_view &tl_type_name) {
  if (tl_type_name != "Maybe") {
    return false;
  }
  assert(tl_type_expr.children.size() == 1);
  const size_t maybe_prefix_start = php_doc_.size();
  if (is_template_instantiating()) {
    php_doc_.append("maybe_");
  } else if (is_array_instantiating()) {
    php_doc_.append("(");
  }
  const size_t maybe_prefix_len = php_doc_.size() - maybe_prefix_start;

  php_field_type processed_type_in_maybe = php_field_type::t_unknown;
  {
    AutoPushPop auto_push_pop{*this, php_field_type::t_maybe};
    tl_type_expr.children.front()->visit(*this);
    processed_type_in_maybe = last_processed_type_;
  }
  if (!is_or_null_possible(processed_type_in_maybe)) {
    php_doc_.erase(maybe_prefix_start, maybe_prefix_len);
  } else if (!is_template_instantiating()) {
    php_doc_.append("|null");
    if (is_array_instantiating()) {
      php_doc_.append(")");
    }
  }

  return true;
}

void CombinatorToPhp::as_complex_type(const tlo_parsing::type_expr &tl_type_expr, const tlo_parsing::type &tl_type) {
  assert(!tl_type.constructors.empty());
  const bool is_template = is_template_instantiating();
  auto php_doc_type = is_template || tl_type.is_polymorphic()
                      ? tl_type.name
                      : tl_type.constructors.front()->name;

  has_class_inside_ = true;
  if (is_template) {
    std::replace(php_doc_type.begin(), php_doc_type.end(), '.', '_');
  } else {
    std::string engine_namespace{"\\"};
    size_t dot_pos = php_doc_type.find('.');
    if (dot_pos != std::string::npos) {
      engine_namespace.append(php_doc_type, 0, dot_pos);
      php_doc_type[dot_pos] = '_';
    } else {
      engine_namespace.append(PhpClasses::common_engine_namespace());
    }
    php_doc_type = PhpClasses::tl_namespace() + engine_namespace + '\\' + PhpClasses::types_namespace() + '\\' + php_doc_type;
  }

  const size_t type_start = php_doc_.size();
  php_doc_.append(php_doc_type);
  const size_t type_postfix_start = php_doc_.size();
  size_t type_postfix_end = type_postfix_start;
  {
    AutoPushPop auto_push_pop{*this, php_field_type::t_class};
    for (const auto &child : tl_type_expr.children) {
      child->visit(*this);
      if (type_postfix_end != php_doc_.size()) {
        php_doc_.insert(type_postfix_end, "__");
        type_postfix_end = php_doc_.size();
      }
    }
  }
  std::string type_postfix;
  bool is_builtin = false;
  if (tl_to_php_.need_rpc_response_type_hack(php_doc_)) {
    is_builtin = true;
    php_doc_.erase(type_start);
    php_doc_.append(is_template ? PhpClasses::rpc_response_type() : PhpClasses::rpc_response_type_with_tl_namespace());
  } else {
    type_postfix = php_doc_.substr(type_postfix_start, type_postfix_end - type_postfix_start);
  }
  tl_to_php_.register_type(tl_type, std::move(type_postfix), *this, tl_type_expr, is_builtin);
}

void CombinatorToPhp::apply(const tlo_parsing::type_expr &tl_type_expr) {
  auto tl_type_it = tl_to_php_.get_sheme().types.find(tl_type_expr.type_id);
  assert(tl_type_it != tl_to_php_.get_sheme().types.end());

  const vk::string_view tl_type_name{tl_type_it->second->name};
  if (try_as_primitive_builtin_type(tl_type_name)) {
    return;
  }
  if (try_as_array_type(tl_type_expr, tl_type_name)) {
    return;
  }
  if (try_as_maybe_type(tl_type_expr, tl_type_name)) {
    return;
  }
  as_complex_type(tl_type_expr, *tl_type_it->second);
}

void CombinatorToPhp::apply(const tlo_parsing::type_array &tl_type_array) {
  // expect anonymous args replacing before (check replace_anonymous_args)
  assert(tl_type_array.args.size() == 1);
  as_array_type(*tl_type_array.args.front()->type_expr);
}

bool CombinatorToPhp::is_template_instantiating() const {
  return std::find(type_stack_.begin(), type_stack_.end(), php_field_type::t_class) != type_stack_.end();
}

bool CombinatorToPhp::is_array_instantiating() const {
  return std::find(type_stack_.begin(), type_stack_.end(), php_field_type::t_array) != type_stack_.end();
}

} // namespace tl
} // namespace vk
