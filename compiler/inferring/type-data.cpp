#include "compiler/inferring/type-data.h"

#include <regex>
#include <string>
#include <vector>

#include "common/algorithms/compare.h"
#include "common/algorithms/contains.h"
#include "common/termformat/termformat.h"

#include "common-php-functions.h"
#include "compiler/code-gen/common.h"
#include "compiler/data/class-data.h"
#include "compiler/pipes/collect-main-edges.h"
#include "compiler/stage.h"
#include "compiler/threading/hash-table.h"
#include "compiler/utils/string-utils.h"

/*** TypeData::SubkeysValues ***/

TypeData *TypeData::SubkeysValues::create_if_empty(const Key &key, TypeData *parent) {
  if (auto existed_type_data = find(key)) {
    return existed_type_data;
  }

  TypeData *value = get_type(tp_Unknown)->clone();
  value->parent_ = parent;

  add(key, value);
  return value;
}

TypeData *TypeData::SubkeysValues::find(const Key &key) const {
  auto it = std::find_if(values_pairs_.begin(), values_pairs_.end(), [&](auto &kv) { return kv.first == key; });
  if (it != values_pairs_.end()) {
    return it->second;
  }
  return nullptr;
}


/*** TypeData ***/

static std::vector<const TypeData *> primitive_types;
static std::vector<const TypeData *> array_types;

void TypeData::init_static() {
  if (!primitive_types.empty()) {
    return;
  }
  primitive_types.resize(ptype_size);
  array_types.resize(ptype_size);

  for (int tp = 0; tp < ptype_size; tp++) {
    primitive_types[tp] = new TypeData((PrimitiveType)tp);
  }

  for (int tp = 0; tp < ptype_size; tp++) {
    array_types[tp] = create_type_data(primitive_types[tp == tp_Any ? tp_CreateAny : tp]);
  }
}

const TypeData *TypeData::get_type(PrimitiveType type) {
  return primitive_types[type];
}

const TypeData *TypeData::get_type(PrimitiveType array, PrimitiveType type) {
  if (array != tp_array) {
    return get_type(array);
  }
  return array_types[type];
}

TypeData::TypeData(PrimitiveType ptype) :
  ptype_(ptype),
  generation_(current_generation()) {
  if (ptype_ == tp_Null) {
    set_or_null_flag();
    ptype_ = tp_Unknown;
  }
  if (ptype_ == tp_False) {
    set_or_false_flag();
    ptype_ = tp_Unknown;
  }
}

TypeData::TypeData(const TypeData &from) :
  ptype_(from.ptype_),
  flags_(from.flags_),
  generation_(from.generation_),
  class_type_(from.class_type_),
  subkeys_values(from.subkeys_values) {
  if (from.anykey_value != nullptr) {
    anykey_value = from.anykey_value->clone();
    anykey_value->parent_ = this;
  }
  for (auto &subkey : subkeys_values) {
    TypeData *ptr = subkey.second;
    assert (ptr != nullptr);
    subkey.second = ptr->clone();
    subkey.second->parent_ = this;
  }
}

TypeData::~TypeData() {
  assert (parent_ == nullptr);

  if (anykey_value != nullptr) {
    anykey_value->parent_ = nullptr;
    delete anykey_value;
  }
  for (auto &subkey : subkeys_values) {
    TypeData *ptr = subkey.second;
    ptr->parent_ = nullptr;
    delete ptr;
  }
}

TypeData *TypeData::at(const Key &key) const {
  kphp_assert_msg (structured(), "bug in TypeData");

  return key.is_any_key() ? anykey_value : subkeys_values.find(key);  // любое может быть nullptr
}

TypeData *TypeData::at_force(const Key &key) {
  kphp_assert_msg (structured(), "bug in TypeData");

  TypeData *res = at(key);
  if (res != nullptr) {
    return res;
  }

  TypeData *value = get_type(tp_Unknown)->clone();
  value->parent_ = this;
  value->on_changed();

  if (key.is_any_key()) {
    anykey_value = value;
  } else {
    subkeys_values.add(key, value);
  }

  return value;
}

PrimitiveType TypeData::get_real_ptype() const {
  const PrimitiveType p = ptype();
  if (p == tp_Unknown && (or_null_flag() || or_false_flag())) {
    return tp_bool;
  }
  return p;
}

void TypeData::set_ptype(PrimitiveType new_ptype) {
  if (new_ptype != ptype_) {
    ptype_ = new_ptype;
    if (new_ptype == tp_Error) {
      set_error_flag();
    }
    on_changed();
  }
}

const std::forward_list<ClassPtr> &TypeData::class_types() const {
  return class_type_;
}

ClassPtr TypeData::class_type() const {
  if (class_type_.empty()) return {};
  kphp_assert(std::distance(class_type_.begin(), class_type_.end()) == 1);
  return class_type_.front();
}

void TypeData::set_class_type(const std::forward_list<ClassPtr> &new_class_type) {
  if (new_class_type.empty()) {
    return;
  }

  if (class_type_.empty()) {
    class_type_ = new_class_type;
    on_changed();
  } else if (!vk::all_of(class_type_, [&](ClassPtr c) { return vk::contains(new_class_type, c); })) {
    std::unordered_set<ClassPtr> result_type;
    for (const auto &possible_class : class_type_) {
      for (const auto &new_class : new_class_type) {
        auto common_interfaces = possible_class->get_common_base_or_interface(new_class);
        result_type.insert(common_interfaces.begin(), common_interfaces.end());
      }
    }

    if (result_type.empty()) {
      // нельзя в одной переменной/массиве смешивать инстансы разных классов
      set_ptype(tp_Error);
    } else if (!vk::all_of(class_type_, [&](ClassPtr c) { return vk::contains(result_type, c); })) {
      class_type_ = {result_type.begin(), result_type.end()};
      on_changed();
    }
  }
}

template<typename F>
bool TypeData::for_each_deep(const F &visitor) const {
  if (visitor(*this)) {
    return true;
  }
  if (anykey_value && anykey_value->for_each_deep(visitor)) {
    return true;
  }
  for (const auto &sub_key: subkeys_values) {
    if (sub_key.second->for_each_deep(visitor)) {
      return true;
    }
  }
  return false;
}

/**
 * Быстрый аналог !get_all_class_types_inside().empty()
 */
bool TypeData::has_class_type_inside() const {
  return for_each_deep([](const TypeData &data) { return !data.class_type_.empty(); });
}

void TypeData::mark_classes_used() const {
  for_each_deep([](const TypeData &this_) {
    if (this_.ptype() == tp_Class) {
      for (const auto &klass : this_.class_type_) {
        klass->mark_as_used();
      }
    }
    return false;
  });
}

void TypeData::get_all_class_types_inside(std::unordered_set<ClassPtr> &out) const {
  for_each_deep([&out](const TypeData &this_) {
    out.insert(this_.class_type_.begin(), this_.class_type_.end());
    return false;
  });
}

ClassPtr TypeData::get_first_class_type_inside() const {
  ClassPtr first_class;
  for_each_deep([&first_class](const TypeData &this_) {
    first_class = this_.class_type();
    return first_class;
  });
  return first_class;
}

bool TypeData::is_primitive_type() const {
  return vk::any_of_equal(get_real_ptype(), tp_int, tp_bool, tp_float, tp_future, tp_future_queue);
}

TypeData::flags_t TypeData::flags() const {
  return flags_;
}

void TypeData::set_flags(TypeData::flags_t new_flags) {
  kphp_assert_msg((flags_ & new_flags) == flags_, "It is forbidden to remove flag");
  if (flags_ != new_flags) {
    if (new_flags & error_flag_e) {
      set_error_flag();
    }
    flags_ = new_flags;
    on_changed();
  }
}

bool TypeData::use_or_null() const {
  return !::can_store_null(ptype()) && or_null_flag();
}

bool TypeData::use_optional() const {
  return use_or_false() || use_or_null();
}

bool TypeData::can_store_null() const {
  return ::can_store_null(ptype()) || or_null_flag();
}

bool TypeData::use_or_false() const {
  return !::can_store_false(ptype()) && or_false_flag() && ptype() != tp_Unknown;
}

bool TypeData::can_store_false() const {
  return ::can_store_false(ptype()) || or_false_flag();
}

bool TypeData::structured() const {
  return vk::any_of_equal(ptype(), tp_array, tp_tuple, tp_shape, tp_future, tp_future_queue);
}

TypeData::generation_t TypeData::generation() const {
  return generation_;
}

void TypeData::on_changed() {
  generation_ = current_generation();
  if (parent_ != nullptr) {
    if (parent_->generation_ < current_generation()) {
      parent_->on_changed();
    }
  }
}

TypeData *TypeData::clone() const {
  return new TypeData(*this);
}

void TypeData::convert_Unknown_to_Any() {
  if (get_real_ptype() == tp_Unknown) {
    set_ptype(tp_Any);
  }
  if (anykey_value != nullptr) {
    anykey_value->convert_Unknown_to_Any();
  }
  for (auto &subkey : subkeys_values) {
    subkey.second->convert_Unknown_to_Any();
  }
}

const TypeData *TypeData::const_read_at(const Key &key) const {
  if (ptype() == tp_var) {
    return get_type(tp_var);
  }
  if (ptype() == tp_string) {
    return get_type(tp_string);
  }
  if (!structured()) {
    return get_type(tp_Unknown);
  }
  if (vk::any_of_equal(ptype(), tp_tuple, tp_shape) && key.is_any_key()) {
    return get_type(tp_Error);
  }
  TypeData *res = at(key);
  if (res == nullptr && !key.is_any_key()) {
    res = anykey_value;
  }
  if (res == nullptr) {
    return get_type(tp_Unknown);
  }
  return res;
}

const TypeData *TypeData::const_read_at(const MultiKey &multi_key) const {
  const TypeData *res = this;
  for (Key i : multi_key) {
    res = res->const_read_at(i);
  }
  return res;
}

void TypeData::make_structured() {
  // lvalue $s[idx] делает $s всегда массивом: строки и кортежи только на read-only оставляют типизацию
  if (ptype() < tp_array) {
    PrimitiveType new_ptype = type_lca(ptype(), tp_array);
    set_ptype(new_ptype);
  }
}

TypeData *TypeData::write_at(const Key &key) {
  make_structured();
  if (!structured()) {
    return nullptr;
  }
  TypeData *res = at_force(key);
  res->set_write_flag();
  return res;
}

TypeData *TypeData::lookup_at(const Key &key) const {
  if (!structured()) {
    return nullptr;
  }
  return key.is_any_key() ? anykey_value : subkeys_values.find(key);
}

TypeData::lookup_iterator TypeData::lookup_begin() const {
  return structured() ? subkeys_values.begin() : subkeys_values.end();
}

TypeData::lookup_iterator TypeData::lookup_end() const {
  return subkeys_values.end();
}

void TypeData::set_lca(const TypeData *rhs, bool save_or_false, bool save_or_null) {
  if (rhs == nullptr) {
    return;
  }
  TypeData *lhs = this;

  PrimitiveType new_ptype = type_lca(lhs->ptype(), rhs->ptype());
  if (new_ptype == tp_var) {
    if (lhs->ptype() == tp_array && lhs->anykey_value != nullptr) {
      lhs->anykey_value->set_lca(tp_var);
      if (lhs->ptype() == tp_Error) {
        new_ptype = tp_Error;
      }
    }
    if (rhs->ptype() == tp_array && rhs->anykey_value != nullptr) {
      TypeData tmp(tp_var);
      tmp.set_lca(rhs->anykey_value);
      if (tmp.ptype() == tp_Error) {
        new_ptype = tp_Error;
      }
    }
  }

  lhs->set_ptype(new_ptype);
  TypeData::flags_t new_flags = rhs->flags_;
  if (!save_or_false) {
    new_flags &= ~(or_false_flag_e);
  }
  if (!save_or_null) {
    new_flags &= ~(or_null_flag_e);
  }
  new_flags |= lhs->flags_;

  lhs->set_flags(new_flags);
  if (new_ptype == tp_void && (new_flags & (or_false_flag_e|or_null_flag_e))) {
    lhs->set_ptype(tp_Error);
  }

  if (rhs->ptype() == tp_Class) {
    if (lhs->or_false_flag()) {
      lhs->set_ptype(tp_Error);
    } else {
      lhs->set_class_type(rhs->class_type_);
    }
  }

  if (!lhs->structured()) {
    return;
  }

  if (new_ptype == tp_tuple && rhs->ptype() == tp_tuple) {
    if (!lhs->subkeys_values.empty() && !rhs->subkeys_values.empty() && lhs->subkeys_values.size() != rhs->subkeys_values.size()) {
      lhs->set_ptype(tp_Error);   // совмещение tuple'ов разных размеров
      return;
    }
  }

  if (new_ptype == tp_shape && rhs->ptype() == tp_shape) {
    // shape'ы при расширении образуют более широкий (union) shape, это нормально, tp_Error тут не возникнет
    // а @param с конкретной структурой shape'аs — на restriction'ах проверится
  }

  TypeData *lhs_any_key = lhs->at_force(Key::any_key());
  TypeData *rhs_any_key = rhs->lookup_at(Key::any_key());
  lhs_any_key->set_lca(rhs_any_key);

  if (!rhs->subkeys_values.empty()) {
    for (const auto &rhs_subkey : rhs->subkeys_values) {
      Key rhs_key = rhs_subkey.first;
      TypeData *rhs_value = rhs_subkey.second;
      TypeData *lhs_value = lhs->subkeys_values.create_if_empty(rhs_key, lhs);
      lhs_value->set_lca(rhs_value);
    }
    for (auto &lhs_subkey : lhs->subkeys_values) {
      if (!rhs->subkeys_values.find(lhs_subkey.first)) {
        lhs_subkey.second->set_or_null_flag();
      }
    }
  }
}

void TypeData::set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false, bool save_or_null) {
  TypeData *cur = this;
  auto last = multi_key.rbegin();
  for (auto it = multi_key.begin(); it != multi_key.end(); it++) {
    auto key = *it;
    auto prev = cur;
    cur = cur->write_at(key);
    if (cur == nullptr) {
      if (prev->ptype() == tp_var) {
        TypeData tmp(tp_var);
        tmp.set_lca(rhs);
        if (tmp.ptype() == tp_Error) {
          prev->set_ptype(tp_Error);
          last = std::prev(multi_key.rend(), std::distance(multi_key.begin(), it));
          cur = prev;
          break;
        }
      }
      return;
    }
  }
  cur->set_lca(rhs, save_or_false, save_or_null);
  for (auto it = last; it != multi_key.rend(); it++) {
    cur = cur->parent_;
    if (*it == Key::any_key()) {
      TypeData *any_value = cur->at_force(Key::any_key());
      TypeData *key_value = cur->write_at(*it);
      any_value->set_lca(key_value);
    }
  }
}

void TypeData::fix_inf_array() {
  //hack: used just to make current version stable
  int depth = 0;
  TypeData *cur = this;
  while (cur != nullptr) {
    cur = cur->lookup_at(Key::any_key());
    depth++;
  }
  if (depth > 6) {
    set_lca_at(MultiKey::any_key(6), TypeData::get_type(tp_var));
  }
}

bool TypeData::should_proxy_error_flag_to_parent() const {
  if (vk::any_of_equal(parent_->ptype(), tp_tuple, tp_shape) && parent_->anykey_value == this) {
    return false;   // tp_tuple any key может быть tp_Error (к примеру, tuple(1, new A)), сам tuple от этого не error
  }
  return true;
}

void TypeData::set_lca(PrimitiveType ptype) {
  set_lca(TypeData::get_type(ptype));
}

TLS<TypeData::generation_t> TypeData::current_generation_;

void TypeData::inc_generation() {
  (*current_generation_)++;
}

TypeData::generation_t TypeData::current_generation() {
  return *current_generation_;
}

void TypeData::upd_generation(TypeData::generation_t other_generation) {
  if (other_generation >= *current_generation_) {
    *current_generation_ = other_generation;
  }
}

inline void get_cpp_style_type(const TypeData *type, std::string &res) {
  const PrimitiveType tp = type->get_real_ptype();

  switch (tp) {
    case tp_Class: {
      res += "class_instance<";
      res += type->class_type()->src_name;
      res += ">";
      break;
    }
    case tp_float: {
      res += "double";
      break;
    }
    case tp_tuple: {
      res += "std::tuple";
      break;
    }
    default : {
      res += ptype_name(tp);
      break;
    }
  }
}

inline void get_txt_style_type(const TypeData *type, std::string &res) {
  const PrimitiveType tp = type->get_real_ptype();
  switch (tp) {
    case tp_Class:
      res += vk::join(type->class_types(), ", ", std::mem_fn(&ClassData::name));
      break;
    case tp_bool:
      res += "boolean";
      break;
    default :
      res += ptype_name(tp);
      break;
  }
}

static bool try_get_txt_or_false_or_null_for_unknown(const TypeData *type, std::string &res) {
  if (type->ptype() == tp_Unknown) {
    if (type->or_false_flag()) {
      res += "false";
    }
    if (type->or_null_flag()) {
      if (type->or_false_flag()) {
        res += "|";
      }
      res += "null";
    }
    if (type->or_false_flag() || type->or_null_flag()) {
      return true;
    }
  }
  return false;
}

static void get_txt_or_false_or_null(const TypeData *type, std::string &res) {
  if (type->use_or_null()) {
    res += "|null";
  }
  if (type->use_or_false()) {
    res += "|false";
  }
}

static void type_out_impl(const TypeData *type, std::string &res, gen_out_style style) {
  if (style == gen_out_style::txt && try_get_txt_or_false_or_null_for_unknown(type, res)) {
    return;
  }

  const bool use_optional = style != gen_out_style::txt && type->use_optional();
  if (use_optional) {
    res += "Optional < ";
  }

  const PrimitiveType tp = type->get_real_ptype();
  if (style == gen_out_style::tagger && vk::any_of_equal(tp, tp_future, tp_future_queue)) {
    res += "int";
  } else {
    if (vk::any_of_equal(style, gen_out_style::cpp, gen_out_style::tagger)) {
      get_cpp_style_type(type, res);
    } else {
      get_txt_style_type(type, res);
    }

    const bool need_any_key = vk::any_of_equal(tp, tp_array, tp_future, tp_future_queue);
    const TypeData *anykey_value = need_any_key ? type->lookup_at(Key::any_key()) : nullptr;
    if (anykey_value) {
      res += "< ";
      type_out_impl(anykey_value, res, style);
      res += " >";
    }

    if (tp == tp_tuple) {
      res += "<";
      for (const auto &subkey : std::set<TypeData::KeyValue>{type->lookup_begin(), type->lookup_end()}) {
        if (res.back() != '<') {
          res += " , ";
        }
        kphp_assert(subkey.first.is_int_key());
        type_out_impl(type->const_read_at(subkey.first), res, style);
      }
      res += ">";
    }

    if (tp == tp_shape) {
      // рассчитывать на порядок ключей в subkeys_values у TypeData мы не можем, и для стабильной кодогенерации
      // ключи shape'ов выводим отсортированными по хешам (не по id ключей! они разные между запусками)
      // важно! в таком же порядке выводятся значения при конструировании shape'а: см. compile_shape()
      std::vector<std::pair<Key, TypeData *>> sorted_by_hash(type->lookup_begin(), type->lookup_end());
      std::sort(sorted_by_hash.begin(), sorted_by_hash.end(), [](const auto &a, const auto &b) -> bool {
        const std::string &a_str = a.first.to_string();
        const std::string &b_str = b.first.to_string();
        return string_hash(a_str.c_str(), a_str.size()) < string_hash(b_str.c_str(), b_str.size());
      });

      std::string keys_hashes_str, types_str;
      for (auto subkey : sorted_by_hash) {
        if (!keys_hashes_str.empty()) {
          keys_hashes_str += ",";
          types_str += ", ";
        }
        const std::string &key_str = subkey.first.to_string();
        keys_hashes_str += std::to_string(static_cast<unsigned int>(string_hash(key_str.c_str(), key_str.size())));
        if (style == gen_out_style::txt) {
          types_str += key_str;
          types_str += ":";
        }
        type_out_impl(subkey.second, types_str, style);
      }

      res += "<";
      if (style != gen_out_style::txt) {
        res += "std::index_sequence<";
        res += keys_hashes_str;
        res += ">, ";
      }
      res += types_str;
      if (style == gen_out_style::txt && type->shape_has_varg_flag()) {
        res += ", ...";
      }
      res += ">";
    }
  }

  if (use_optional) {
    res += " >";
  }

  if (style == gen_out_style::txt) {
    get_txt_or_false_or_null(type, res);
  }
}

std::string type_out(const TypeData *type, gen_out_style style) {
  std::string res;
  type_out_impl(type, res, style);
  return res;
}

std::string colored_type_out(const TypeData *type) {
  std::string type_str = type_out(type, gen_out_style::txt);
  return TermStringFormat::paint(type_str, TermStringFormat::green);
}

int type_strlen(const TypeData *type) {
  PrimitiveType tp = type->ptype();
  switch (tp) {
    case tp_Unknown:
      if (type->or_null_flag() || type->or_false_flag()) {
        return STRLEN_EMPTY;
      }
      return STRLEN_UNKNOWN;
    case tp_Null:
    case tp_False:
      return STRLEN_EMPTY;
    case tp_bool:
      return STRLEN_BOOL_;
    case tp_int:
      return STRLEN_INT;
    case tp_float:
      return STRLEN_FLOAT;
    case tp_array:
      return STRLEN_ARRAY_;
    case tp_tuple:
      return STRLEN_ARRAY_;
    case tp_shape:
      return STRLEN_ARRAY_;
    case tp_string:
      return STRLEN_STRING;
    case tp_var:
      return STRLEN_VAR;
    case tp_UInt:
      return STRLEN_UINT;
    case tp_Long:
      return STRLEN_LONG;
    case tp_ULong:
      return STRLEN_ULONG;
    case tp_Class:
      return STRLEN_CLASS;
    case tp_void:
      return STRLEN_VOID;
    case tp_future:
      return STRLEN_FUTURE;
    case tp_future_queue:
      return STRLEN_FUTURE_QUEUE;
    case tp_Error:
      return STRLEN_ERROR;
    case tp_Any:
      return STRLEN_ANY;
    case tp_CreateAny:
      return STRLEN_CREATE_ANY;
    case tp_regexp:
    case ptype_size:
    kphp_fail();
  }
  return STRLEN_ERROR;
}

bool can_be_same_type(const TypeData *type1, const TypeData *type2) {
  if (type1->ptype() == tp_var || type2->ptype() == tp_var) {
    return true;
  }
  if (type1->can_store_false() && type2->can_store_false()) {
    return true;
  }
  if (type1->can_store_null() && type2->can_store_null()) {
    return true;
  }

  // todo по-моему, этого не нужно
  auto is_array_or_tuple = [](const TypeData *type) { return vk::any_of_equal(type->ptype(), tp_array, tp_tuple, tp_shape); };
  if (is_array_or_tuple(type1) && is_array_or_tuple(type2)) {
    return true;
  }

  return type1->ptype() == type2->ptype();
}

// compare types in sense of cpp type out
bool is_equal_types(const TypeData *type1, const TypeData *type2) {
  if (type1 == nullptr) {
    return type2 == nullptr;
  }
  if (type2 == nullptr) {
    return false;
  }

  if (type1->get_real_ptype() != type2->get_real_ptype()) {
    return false;
  }
  if (type1->use_or_false() != type2->use_or_false()) {
    return false;
  }
  if (type1->use_or_null() != type2->use_or_null()) {
    return false;
  }
  const PrimitiveType tp = type1->get_real_ptype();

  if (tp == tp_Class) {
    return type1->class_types() == type2->class_types();
  }

  if (tp == tp_array) {
    return is_equal_types(type1->lookup_at(Key::any_key()), type2->lookup_at(Key::any_key()));
  }

  if (vk::any_of_equal(tp, tp_shape, tp_tuple)) {
    for (auto it1 = type1->lookup_begin(); it1 != type1->lookup_end(); ++it1) {
      TypeData *t2_at_it = type2->lookup_at(it1->first);
      if (t2_at_it == nullptr && !type2->shape_has_varg_flag()) {
        return false;
      } else if (t2_at_it && !is_equal_types(it1->second, t2_at_it)) {
        return false;
      }
    }
    for (auto it2 = type2->lookup_begin(); it2 != type2->lookup_end(); ++it2) {
      TypeData *t1_at_it = type1->lookup_at(it2->first);
      if (t1_at_it == nullptr && !type1->shape_has_varg_flag()) {
        return false;
      }
    }
  }

  return true;
}

size_t TypeData::get_tuple_max_index() const {
  kphp_assert(ptype() == tp_tuple);
  return subkeys_values.size();
}

const TypeData *TypeData::create_for_class(ClassPtr klass) {
  auto *res = new TypeData(tp_Class);
  res->class_type_ = {klass};
  return res;
}

TypeData *TypeData::create_type_data(const TypeData *element_type) {
  auto *res = new TypeData(tp_array);
  res->set_lca_at(MultiKey::any_key(1), element_type);
  return res;
}

TypeData *TypeData::create_type_data(const std::vector<const TypeData *> &subkeys_values) {
  auto *res = new TypeData(tp_tuple);
  for (int int_index = 0; int_index < subkeys_values.size(); ++int_index) {
    res->set_lca_at(MultiKey({Key::int_key(int_index)}), subkeys_values[int_index]);
  }
  return res;
}

TypeData *TypeData::create_type_data(const std::map<std::string, const TypeData *> &subkeys_values) {
  auto *res = new TypeData(tp_shape);
  for (const auto &sub: subkeys_values) {
    res->set_lca_at(MultiKey({Key::string_key(sub.first)}), sub.second);
  }
  return res;
}
