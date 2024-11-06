// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/type-data.h"

#include "compiler/inferring/primitive-type.h"
#include "compiler/kphp_assert.h"
#include <string>
#include <vector>

#include "common/algorithms/compare.h"
#include "common/algorithms/contains.h"
#include "common/termformat/termformat.h"

#include "common/php-functions.h"
#include "compiler/compiler-core.h"
#include "compiler/code-gen/common.h"
#include "compiler/data/class-data.h"
#include "compiler/data/ffi-data.h"
#include "compiler/pipes/collect-main-edges.h"
#include "compiler/stage.h"
#include "compiler/threading/hash-table.h"
#include "compiler/utils/string-utils.h"


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
    array_types[tp] = create_array_of(primitive_types[tp]);
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
  ptype_(ptype) {
  if (ptype_ == tp_Null) {
    set_or_null_flag();
    ptype_ = tp_any;
  }
  if (ptype_ == tp_False) {
    set_or_false_flag();
    ptype_ = tp_any;
  }
}

TypeData::TypeData(const TypeData &from) :
  ptype_(from.ptype_),
  flags_(from.flags_),
  indirection_(from.indirection_),
  class_type_(from.class_type_),
  subkeys(from.subkeys) {
  for (auto &subkey : subkeys) {
    subkey.second = subkey.second->clone();
  }
}

TypeData::~TypeData() {
  for (auto &subkey : subkeys) {
    delete subkey.second;
  }
}

std::string TypeData::as_human_readable(bool colored) const {
  std::string res;

  switch (ptype_) {
    case tp_array: {
      const TypeData *inner = lookup_at_any_key() ?: TypeData::get_type(tp_any);
      if (inner->get_real_ptype() == tp_any) {
        res = "array";
      } else {
        std::string inner_str = inner->as_human_readable(false);
        res = inner->use_optional() ? "(" + inner_str + ")" + "[]" : inner_str + "[]";
      }
      break;
    }
    case tp_tuple: {
      res = "tuple(";
      for (int tuple_i = 0; tuple_i < get_tuple_max_index(); ++tuple_i) {
        if (tuple_i > 0) {
          res += ", ";
        }
        res += lookup_at(Key::int_key(tuple_i))->as_human_readable(false);
      }
      res += ")";
      break;
    }
    case tp_shape: {
      std::vector<SubkeyItem> items{lookup_begin(), lookup_end()};
      std::reverse(items.begin(), items.end());
      res = "shape(" + vk::join(items, ", ", [](const SubkeyItem &p) { return p.first.to_string() + ":" + p.second->as_human_readable(false); });
      if (shape_has_varg_flag()) {
        res += ", ...";
      }
      res += ")";
      break;
    }
    case tp_future: {
      const TypeData *inner = lookup_at_any_key() ?: TypeData::get_type(tp_any);
      res = "future<" + inner->as_human_readable(false) + ">";
      break;
    }
    case tp_future_queue: {
      const TypeData *inner = lookup_at_any_key() ?: TypeData::get_type(tp_any);
      res = "future_queue<" + inner->as_human_readable(false) + ">";
      break;
    }
    case tp_Class: {
      res = class_type()->as_human_readable();
      break;
    }
    case tp_any: {
      if (or_null_flag() && !or_false_flag()) {
        res = "null";
      } else if (or_false_flag() && !or_null_flag()) {
        res += "false";
      } else if (or_false_flag() && or_null_flag()) {
        res += "false|null";
      } else {
        res = "any";
      }
      break;
    }
    default:
      res = ptype_name(ptype_);
  }

  if (ptype_ != tp_any) {
    if (use_or_null() && !use_or_false()) {
      res = "?" + res;
    } else if (use_or_false() && !use_or_null()) {
      res += "|false";
    } else if (use_or_false() && use_or_null()) {
      res += "|false|null";
    }
  }

  if (ffi_const_flag()) {
    res = "const " + res;
  }
  if (indirection_ != 0) {
    res += std::string(indirection_, '*');
  }

  return colored ? TermStringFormat::paint_green(res) : res;
}

TypeData *TypeData::at_force(const Key &key) {
  kphp_assert_msg (structured(), "bug in TypeData");

  for (const auto &subkey : subkeys) {
    if (subkey.first == key) {
      return subkey.second;
    }
  }

  TypeData *value = get_type(tp_any)->clone();
  subkeys.emplace_front(key, value);
  return value;
}

PrimitiveType TypeData::get_real_ptype() const {
  const PrimitiveType p = ptype();
  if (p == tp_any && (or_null_flag() || or_false_flag())) {
    return tp_bool;
  }
  return p;
}

bool TypeData::is_ffi_ref() const {
  auto klass = class_type();
  if (klass && klass->ffi_class_mixin) {
    return klass->ffi_class_mixin->is_ref();
  }
  return false;
}

void TypeData::set_ffi_pointer_type(const TypeData *new_ptr_type, int new_indirection) {
  if (std::distance(new_ptr_type->class_type_.begin(), new_ptr_type->class_type_.end()) != 1) {
    set_ptype(tp_Error);
    return;
  }

  if (ptype() == tp_any) {
    set_ptype(tp_Class);
  }

  if (class_type_.empty()) {
    class_type_ = new_ptr_type->class_type_;
    indirection_ = new_indirection;
    return;
  }

  if (class_type() == G->get_class("FFI\\CData")) {
    // a special case: CData is our opaque type for "any" C type;
    // used in FFI functions.txt file to describe an arbitrary C type
    return;
  }

  auto *ptr_class = class_type()->ffi_class_mixin;

  if (!ptr_class) {
    set_ptype(tp_Error);
    return;
  }

  if (ptr_class->ffi_type->kind == FFITypeKind::Void && indirection_ == 1 && new_indirection != 0) {
    // any pointer is compatible with `void*`,
    // the type remains `void*`
    return;
  }

  auto *new_ptr_class = new_ptr_type->class_type()->ffi_class_mixin;
  if (ptr_class != new_ptr_class) {
    set_ptype(tp_Error);
    return;
  }

  // situations like `T*` vs `T**`
  if (indirection_ != new_indirection) {
    set_ptype(tp_Error);
    return;
  }
}

void TypeData::set_class_type(const std::forward_list<ClassPtr> &new_class_type) {
  if (new_class_type.empty()) {
    return;
  }

  if (class_type_.empty()) {
    class_type_ = new_class_type;
  } else if (!vk::all_of(class_type_, [&](ClassPtr c) { return vk::contains(new_class_type, c); })) {
    std::unordered_set<ClassPtr> result_type;
    for (const auto &possible_class : class_type_) {
      for (const auto &new_class : new_class_type) {
        auto common_interfaces = possible_class->get_common_base_or_interface(new_class);
        result_type.insert(common_interfaces.begin(), common_interfaces.end());
      }
    }

    if (result_type.empty()) {
      // it's illegal to mix instances of different classes inside one variable/array
      set_ptype(tp_Error);
    } else if (!vk::all_of(class_type_, [&](ClassPtr c) { return vk::contains(result_type, c); })) {
      class_type_ = {result_type.begin(), result_type.end()};
    }
  }
}

template<typename F>
bool TypeData::for_each_deep(const F &visitor) const {
  if (visitor(*this)) {
    return true;
  }
  for (const auto &sub_key: subkeys) {
    if (sub_key.second->for_each_deep(visitor)) {
      return true;
    }
  }
  return false;
}

/**
 * Faster alternative for !get_all_class_types_inside().empty()
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

void TypeData::set_flags(uint8_t new_flags) {
  kphp_assert_msg((flags_ & new_flags) == flags_, "It is forbidden to remove flag");
  flags_ = new_flags;
}

bool TypeData::can_store_null() const {
  return ::can_store_null(ptype()) || or_null_flag();
}

bool TypeData::can_store_false() const {
  return ::can_store_false(ptype()) || or_false_flag();
}

bool TypeData::structured() const {
  if (ptype() == tp_Class && !class_type_.empty() && class_type()->name == "FFI\\CData") {
    return true;
  }
  return vk::any_of_equal(ptype(), tp_array, tp_tuple, tp_shape, tp_future, tp_future_queue);
}

TypeData *TypeData::clone() const {
  return new TypeData(*this);
}

const TypeData *TypeData::const_read_at(const Key &key) const {
  if (ptype() == tp_mixed) {
    return get_type(tp_mixed);
  }
  if (ptype() == tp_string) {
    return get_type(tp_string);
  }
  if (ptype() == tp_Class) {
    // TODO any race conditions?
    if (!class_type_.empty()) {
      auto klass = class_type();

      ClassPtr aa = G->get_class("ArrayAccess");
      assert(aa && "Cannot find ArrayAccess");

      // Why offsetSet for ArrayAccess is going through here
      // And in some later pass (FinalCheckPass, for example) check that access via [.] is enabled only for arrays and classes that implements ArrayAccess (by
      // chain)
      if (aa->is_parent_of(klass)) {
        return get_type(tp_mixed);
      }
      kphp_error(false, fmt_format("Class {} that does not implement \\ArrayAccess", klass->name));
    } else {
      kphp_fail_msg("class types is empty! =(");
    }
  }
  if (!structured()) {
    return get_type(tp_any);
  }

  const TypeData *res = lookup_at(key);
  if (res == nullptr && !key.is_any_key()) {
    res = lookup_at_any_key();
  }

  if (res == nullptr) {
    if (vk::any_of_equal(ptype(), tp_tuple, tp_shape) && key.is_any_key()) {
      return get_type(tp_Error);
    }
    return get_type(tp_any);
  }
  return res;
}

const TypeData *TypeData::lookup_at(const Key &key) const {
  for (const auto &subkey : subkeys) {
    if (subkey.first == key) {
      return subkey.second;
    }
  }
  return nullptr;
}

const TypeData *TypeData::const_read_at(const MultiKey &multi_key) const {
  const TypeData *res = this;
  for (Key i : multi_key) {
    res = res->const_read_at(i);
  }
  return res;
}

void TypeData::make_structured() {
  // TODO fix here for writing into objects that implements ArrayAccess
  // 'lvalue $s[idx]' makes $s array-typed: strings and tuples keep their types only for read-only operations
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

const TypeData *TypeData::get_deepest_type_of_array() const {
  if (ptype() == tp_array) {
    return lookup_at_any_key()->get_deepest_type_of_array();
  }
  return this;
}

void TypeData::set_lca(const TypeData *rhs, bool save_or_false, bool save_or_null, FFIRvalueFlags ffi_flags) {
  if (rhs == nullptr) {
    return;
  }
  TypeData *lhs = this;

  PrimitiveType new_ptype = type_lca(lhs->ptype(), rhs->ptype());
  if (lhs->ptype_ == tp_array && rhs->ptype_ == tp_Class) {
    if (lhs->get_write_flag()) {
      // It means that lhs(==this) is something like that "$a[.] = "
      new_ptype = tp_Class; // for array access
    }
  }
  if (new_ptype == tp_mixed) {
    if (lhs->ptype() == tp_array && lhs->lookup_at_any_key()) {
      lhs->set_lca_at(MultiKey::any_key(1), TypeData::get_type(tp_mixed));
      if (lhs->ptype() == tp_Error) {
        new_ptype = tp_Error;
      }
    }
    if (rhs->ptype() == tp_array && rhs->lookup_at_any_key()) {
      TypeData tmp(tp_mixed);
      tmp.set_lca(rhs->lookup_at_any_key());
      if (tmp.ptype() == tp_Error) {
        new_ptype = tp_Error;
      }
    }
  }

  lhs->set_ptype(new_ptype);
  uint8_t new_flags = rhs->flags_;
  if (!save_or_false) {
    new_flags &= ~(or_false_flag_e);
  }
  if (!save_or_null) {
    new_flags &= ~(or_null_flag_e);
  }
  new_flags |= lhs->flags_;

  lhs->set_flags(new_flags);

  if (ffi_flags.drop_ref && rhs->is_ffi_ref()) {
    auto *new_rhs = rhs->clone();
    new_rhs->class_type_ = {rhs->class_type()->ffi_class_mixin->non_ref};
    rhs = new_rhs;
  }
  int rhs_indirection = rhs->get_indirection();
  if (ffi_flags.take_addr) {
    rhs_indirection++;
  }

  if (rhs->ptype() == tp_Class && (lhs->indirection_ != 0 || rhs_indirection != 0)) {
    lhs->set_ffi_pointer_type(rhs, rhs_indirection);
    return;
  }

  // void + false/null does not convert to tp_Error here anymore, as that errors are hard to understand without context
  // (it remains void with flags, and a comprehensive error is printed in final check)

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
    if (!lhs->subkeys.empty() && !rhs->subkeys.empty() && lhs->get_tuple_max_index() != rhs->get_tuple_max_index()) {
      lhs->set_ptype(tp_Error);   // mixing tuples of different sizes
      return;
    }
  }

  if (new_ptype == tp_shape && rhs->ptype() == tp_shape) {
    // lca(shape1, shape2) results in a union shape;
    // we don't emit a tp_Error here, associated @param with shape structure definition
    // will be validated by restrictions
  }

  bool needs_any_key = vk::any_of_equal(new_ptype, tp_array, tp_future, tp_future_queue);
  if (needs_any_key) {
    lhs->at_force(Key::any_key());  // if didn't exist, became tp_any
  }

  if (!rhs->subkeys.empty()) {
    for (const auto &rhs_subkey : rhs->subkeys) {
      Key rhs_key = rhs_subkey.first;
      TypeData *rhs_value = rhs_subkey.second;
      TypeData *lhs_value = lhs->at_force(rhs_key);
      lhs_value->set_lca(rhs_value);
    }
    for (auto &lhs_subkey : lhs->subkeys) {
      if (!rhs->lookup_at(lhs_subkey.first)) {
        lhs_subkey.second->set_or_null_flag();
      }
    }
  }
}

void TypeData::set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false, bool save_or_null, FFIRvalueFlags ffi_flags) {
  TypeData *cur = this;
  
  for (const Key &key : multi_key) {
    auto *prev = cur;
    cur = cur->write_at(key); // HERE
    // handle writing to a subkey of mixed (when cur is not structured)
    if (cur == nullptr) {
      if (prev->ptype() == tp_mixed) {
        TypeData tmp(tp_mixed);
        tmp.set_lca(rhs);
        if (tmp.ptype() == tp_Error) {
          prev->set_ptype(tp_Error);
          cur = prev;
          break;
        }
      }
      return;
    }
  }

  if (cur->get_write_flag()) {
    this->set_write_flag();
  }

  cur->set_lca(rhs, save_or_false, save_or_null, ffi_flags);
  if (cur->error_flag()) {  // proxy tp_Error from keys to the type itself
    this->set_ptype(tp_Error);
  }
}

void TypeData::fix_inf_array() {
  //hack: used just to make current version stable
  int depth = 0;
  const TypeData *cur = this;
  while (cur != nullptr) {
    cur = cur->lookup_at_any_key();
    depth++;
  }
  if (depth > 6) {
    set_lca_at(MultiKey::any_key(6), TypeData::get_type(tp_mixed));
  }
}

void TypeData::set_lca(PrimitiveType ptype) {
  set_lca(TypeData::get_type(ptype));
}

static void append_ffi_type(const TypeData *type, ClassPtr klass, bool boxed, std::string &res) {
  auto *as_ffi = klass->ffi_class_mixin;
  const auto *key = type->lookup_at_any_key();
  if (key) {
    // CData as FFI array
    res += "class_instance<CDataArray<";
    if (key->ffi_const_flag()) {
      res += "const ";
    }
    append_ffi_type(key, key->class_type(), false, res);
    res += ">>";
    return;
  }

  std::string c_type = ffi_mangled_decltype_string(as_ffi->scope_name, FFIRoot::get_ffi_type(klass));

  // TODO: can we avoid manual ptr addition here?
  if (type->get_indirection() != 0) {
    // boxed types are wrapped into CDataPtr which already includes 1 level of indirection
    // unboxed types are normal C pointers, we need all '*' here
    int num_stars = boxed ? type->get_indirection() - 1 : type->get_indirection();
    c_type += std::string(num_stars, '*');
  }

  if (!boxed) {
    res += c_type;
    return;
  }

  if (as_ffi->is_ref()) {
    res += klass->src_name;
  } else if (type->get_indirection() != 0) {
    std::string maybe_const = type->ffi_const_flag() ? "const " : "";
    res += "CDataPtr<" + maybe_const + c_type + ">";
  } else {
    res += "class_instance<C$FFI$CData<" + c_type + ">>";
  }
}

inline void get_cpp_style_type(const TypeData *type, std::string &res) {
  const PrimitiveType tp = type->get_real_ptype();

  switch (tp) {
    case tp_Class: {
      auto klass = type->class_type();
      if (klass->ffi_class_mixin) {
        append_ffi_type(type, klass, true, res);
        break;
      }
      res += "class_instance<";
      res += klass->src_name;
      res += ">";
      break;
    }
    case tp_float: {
      res += "double";
      break;
    }
    case tp_int: {
      res += "int64_t";
      break;
    }
    case tp_tuple: {
      res += "std::tuple";
      break;
    }
    case tp_any: {
      res += "Unknown";
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
    default :
      res += ptype_name(tp);
      break;
  }
}

static bool try_get_txt_or_false_or_null_for_unknown(const TypeData *type, std::string &res) {
  if (type->ptype() == tp_any) {
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
    res += "int64_t";
  } else {
    if (vk::any_of_equal(style, gen_out_style::cpp, gen_out_style::tagger)) {
      get_cpp_style_type(type, res);
    } else {
      get_txt_style_type(type, res);
    }

    if (vk::any_of_equal(tp, tp_array, tp_future, tp_future_queue)) {
      res += "< ";
      type_out_impl(type->lookup_at_any_key(), res, style);
      res += " >";
    }

    if (tp == tp_tuple) {
      res += "<";
      int size = type->get_tuple_max_index();             // order of keys is undetermined
      for (int tuple_i = 0; tuple_i < size; ++tuple_i) {  // that's why use loop by indexes
        if (tuple_i > 0) {
          res += " , ";
        }
        type_out_impl(type->lookup_at(Key::int_key(tuple_i)), res, style);
      }
      res += ">";
    }

    if (tp == tp_shape) {
      // since we can't depend on the TypeData::subkeys order,
      // we emit the shape keys sorted by their key hashes to get the stable code generation
      // Note: key ids can vary between the compiler runs, so they can't be used for sorting
      // Note: this order is used during the shape construction, see compile_shape()
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
        keys_hashes_str += std::to_string(static_cast<size_t>(string_hash(key_str.c_str(), key_str.size())));
        keys_hashes_str += "UL";
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

int type_strlen(const TypeData *type) {
  PrimitiveType tp = type->ptype();
  switch (tp) {
    case tp_any:
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
    case tp_tuple:
    case tp_shape:
      return STRLEN_ARRAY_;
    case tp_string:
    case tp_tmp_string:
      return STRLEN_STRING;
    case tp_mixed:
      return STRLEN_VAR;
    case tp_Class:
    case tp_object:
      return STRLEN_CLASS;
    case tp_void:
      return STRLEN_VOID;
    case tp_future:
      return STRLEN_FUTURE;
    case tp_future_queue:
      return STRLEN_FUTURE_QUEUE;
    case tp_Error:
      return STRLEN_ERROR;
    case tp_regexp:
    case ptype_size:
    kphp_fail();
  }
  return STRLEN_ERROR;
}

bool can_be_same_type(const TypeData *type1, const TypeData *type2) {
  if (type1->ptype() == tp_mixed || type2->ptype() == tp_mixed) {
    return true;
  }
  if (type1->can_store_false() && type2->can_store_false()) {
    return true;
  }
  if (type1->can_store_null() && type2->can_store_null()) {
    return true;
  }

  // TODO: do we need this?
  auto is_array_or_tuple = [](const TypeData *type) { return vk::any_of_equal(type->ptype(), tp_array, tp_tuple, tp_shape); };
  if (is_array_or_tuple(type1) && is_array_or_tuple(type2)) {
    return true;
  }

  return type1->ptype() == type2->ptype();
}

// check if types fully equal (if type2 is any, it's equal to anything)
// note that false != bool here
bool are_equal_types(const TypeData *type1, const TypeData *type2) {
  if (type1 == nullptr) {
    return type2 == nullptr;
  }
  if (type2 == nullptr) {
    return false;
  }

  bool fully_eq = type1->ptype() == type2->ptype() && type1->use_or_false() == type2->use_or_false() && type1->use_or_null() == type2->use_or_null();
  if (!fully_eq) {
    return type2->get_real_ptype() == tp_any;
  }

  const PrimitiveType tp = type1->ptype();

  if (tp == tp_Class) {
    return type1->class_types() == type2->class_types();
  }

  if (vk::any_of_equal(tp, tp_array, tp_future, tp_future_queue)) {
    return are_equal_types(type1->lookup_at_any_key(), type2->lookup_at_any_key());
  }

  if (vk::any_of_equal(tp, tp_shape, tp_tuple)) {
    for (auto it1 = type1->lookup_begin(); it1 != type1->lookup_end(); ++it1) {
      const TypeData *t2_at_it = type2->lookup_at(it1->first);
      if (t2_at_it == nullptr && !type2->shape_has_varg_flag()) {
        return false;
      } else if (t2_at_it && !are_equal_types(it1->second, t2_at_it)) {
        return false;
      }
    }
    for (auto it2 = type2->lookup_begin(); it2 != type2->lookup_end(); ++it2) {
      const TypeData *t1_at_it = type1->lookup_at(it2->first);
      if (t1_at_it == nullptr && !type1->shape_has_varg_flag()) {
        return false;
      }
    }
  }

  return true;
}

// check that given <= expected, to if expected is a phpdoc restriction, check that actual inferred type matches
// note that false < bool
bool is_less_or_equal_type(const TypeData *given, const TypeData *expected, const MultiKey *from_at) {
  // optimization: for obvious cases (like primitive=primitive or primitive<mixed, which is about 80% of calls)
  // immediately return true, without extra memory allocations and lca checks
  bool eq_flags = given->use_or_false() == expected->use_or_false() && given->use_or_null() == expected->use_or_null();
  if (eq_flags && !from_at) {
    PrimitiveType tp = given->ptype();
    switch (expected->ptype()) {
      case tp_any:
        if (expected->get_real_ptype() == tp_any) {
          return true;
        }
        break;
      case tp_string:
      case tp_int:
      case tp_float:
      case tp_bool:
      case tp_void:
        if (tp == expected->ptype()) {
          return true;
        }
        break;
      case tp_array:
        if (tp == tp_array && expected->lookup_at(Key::any_key())->get_real_ptype() == tp_any) {
          return true;
        }
        break;
      case tp_mixed:
        if (vk::any_of_equal(tp, tp_bool, tp_int, tp_float, tp_string, tp_mixed)) {
          return true;
        }
        break;
      case tp_Class:
        if (given->class_types() == expected->class_types()) {
          if (given->ffi_const_flag() && !expected->ffi_const_flag()) {
            return false;
          }
          if (given->get_indirection() == expected->get_indirection()) {
            return true;
          }
        }
        break;
      case tp_object:
        return tp == tp_Class || tp == tp_object;
      default:
        break;
    }
  }

  // for non-obvious cases like arrays, tuples or varying flags, we do a heavy check, that takes more time
  std::unique_ptr<TypeData> type_of_to_node(expected->clone());

  if (from_at && !from_at->empty()) {
    type_of_to_node->set_lca_at(*from_at, given);
  } else {
    type_of_to_node->set_lca(given);
  }

  return are_equal_types(type_of_to_node.get(), expected);
}

bool is_implicit_array_conversion(const TypeData *from, const TypeData *to) noexcept {
  if (!from || !to) {
    return false;
  }
  if (from->get_real_ptype() == tp_array) {
    const auto *from_array_value_type = from->lookup_at_any_key();
    if (from_array_value_type->get_real_ptype() == tp_any) {
      return false;
    }
    if (to->get_real_ptype() == tp_mixed) {
      return from_array_value_type->get_real_ptype() != tp_mixed;
    }
    return !are_equal_types(from_array_value_type, to->lookup_at_any_key());
  }

  const auto implicit_cast_pred = [to](const TypeData::SubkeyItem &key_value) {
    return is_implicit_array_conversion(key_value.second, to->lookup_at(key_value.first));
  };
  return std::find_if(from->lookup_begin(), from->lookup_end(), implicit_cast_pred) != from->lookup_end();
}

size_t TypeData::get_tuple_max_index() const {
  kphp_assert(ptype() == tp_tuple);
  return std::distance(subkeys.begin(), subkeys.end());
}

bool TypeData::did_type_data_change_after_tinf_step(const TypeData *before) const {
  if (ptype_ != before->ptype_ || flags_ != before->flags_) {
    return true;
  }
  if (!class_type_.empty() && class_type_ != before->class_type_) {
    return true;
  }

  // most likely we have no subkeys and return false now
  if (subkeys.empty() && before->subkeys.empty()) {
    return false;
  }

  auto i1 = subkeys.begin();
  auto i2 = before->subkeys.begin();
  auto e1 = subkeys.end();
  auto e2 = before->subkeys.end();
  for (; i1 != e1 && i2 != e2; ++i1, ++i2) {
    if (i1->first != i2->first || i1->second->did_type_data_change_after_tinf_step(i2->second)) {
      return true;
    }
  }
  return i1 != e1 || i2 != e2;
}

const TypeData *TypeData::create_for_class(ClassPtr klass) {
  auto *res = new TypeData(tp_Class);
  res->class_type_ = {klass};
  return res;
}

const TypeData *TypeData::create_array_of(const TypeData *element_type) {
  auto *res = new TypeData(tp_array);
  res->set_lca_at(MultiKey::any_key(1), element_type);
  return res;
}

