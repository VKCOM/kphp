// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <forward_list>
#include <map>
#include <string>

#include "compiler/code-gen/gen-out-style.h"
#include "compiler/data/data_ptr.h"
#include "compiler/debug.h"
#include "compiler/inferring/key.h"
#include "compiler/inferring/multi-key.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/stage.h"
#include "compiler/threading/tls.h"

class TypeData {
  DEBUG_STRING_METHOD {
    return as_human_readable(false);
  }

private:
  enum flag_id_t : uint8_t {
    write_flag_e = 0b00000001,
    or_null_flag_e = 0b00000010,
    or_false_flag_e = 0b00000100,
    shape_has_varg_flag_e = 0b00001000,
    ffi_const_flag_e = 0b00010000,
    tuple_as_array_flag_e = 0b00100000,
  };

public:
  using SubkeyItem = std::pair<Key, TypeData *>;
  using lookup_iterator = std::forward_list<SubkeyItem>::const_iterator;

  // TODO: move all flags (drop_false, drop_null) here and rename this struct?
  // passing several booleans as set_lca args is clumsy
  struct FFIRvalueFlags {
    bool drop_ref : 1;
    bool take_addr : 1;

    FFIRvalueFlags()
      : drop_ref{false}
      , take_addr{false} {}
  };

private:
  PrimitiveType ptype_ : 8; // current type (int/array/etc); tp_any for uninited, tp_Error if error
  uint8_t flags_{0};        // a binary mask of flag_id_t
  uint8_t indirection_{0};  // ptr levels for FFI pointers

  // current class for tp_Class (but during inferring it could contain many classes due to multiple implements)
  std::forward_list<ClassPtr> class_type_;
  // subkeys are inner representations for arrays, tuples and other types containing other types
  // for non-structured, it's empty (although it could remain any_key if array turned to mixed, then it doesn't make sense)
  // for array/future, it contains a single item — {any_key, anykey_type}
  // for tuples/shapes, it contains multiple items — {key1, key1_type}, {key2, key2_type}, order is undetermined
  std::forward_list<SubkeyItem> subkeys;

  explicit TypeData(PrimitiveType ptype);

  TypeData *at_force(const Key &key);
  TypeData *write_at(const Key &key);
  const TypeData *const_read_at(const Key &key) const;

  template<flag_id_t FLAG>
  inline bool get_flag() const {
    return flags_ & FLAG;
  }

  template<flag_id_t FLAG>
  void set_flag() {
    flags_ |= FLAG;
  }

  template<typename F>
  bool for_each_deep(const F &visitor) const;

public:
  TypeData &operator=(const TypeData &) = delete;
  TypeData(const TypeData &from);
  ~TypeData();

  std::string as_human_readable(bool colored = true) const;

  PrimitiveType ptype() const {
    return ptype_;
  }
  PrimitiveType get_real_ptype() const;
  void set_ptype(PrimitiveType new_ptype) {
    ptype_ = new_ptype;
  }

  const std::forward_list<ClassPtr> &class_types() const {
    return class_type_;
  }
  ClassPtr class_type() const {
    return class_type_.empty() ? ClassPtr{} : class_type_.front();
  }
  void set_class_type(const std::forward_list<ClassPtr> &new_class_type);
  void set_ffi_pointer_type(const TypeData *new_ptr_type, int new_indirection);
  bool has_class_type_inside() const;
  void mark_classes_used() const;
  void get_all_class_types_inside(std::unordered_set<ClassPtr> &out) const;
  ClassPtr get_first_class_type_inside() const;
  bool is_primitive_type() const;

  uint8_t flags() const {
    return flags_;
  }
  void set_flags(uint8_t new_flags);

  bool is_ffi_ref() const;

  bool ffi_const_flag() const {
    return get_flag<ffi_const_flag_e>();
  }
  void set_ffi_const_flag() {
    set_flag<ffi_const_flag_e>();
  }

  bool or_false_flag() const {
    return get_flag<or_false_flag_e>();
  }
  void set_or_false_flag() {
    set_flag<or_false_flag_e>();
  }
  bool use_or_false() const {
    return or_false_flag() && !::can_store_false(ptype_);
  }
  bool can_store_false() const;

  bool or_null_flag() const {
    return get_flag<or_null_flag_e>();
  }
  void set_or_null_flag() {
    set_flag<or_null_flag_e>();
  }
  bool use_or_null() const {
    return or_null_flag() && !::can_store_null(ptype_);
  }
  bool can_store_null() const;

  bool use_optional() const {
    return use_or_false() || use_or_null();
  }

  void set_write_flag() {
    set_flag<write_flag_e>();
  }

  bool error_flag() const {
    return ptype_ == tp_Error;
  }

  void set_shape_has_varg_flag() {
    set_flag<shape_has_varg_flag_e>();
  }
  bool shape_has_varg_flag() const {
    return get_flag<shape_has_varg_flag_e>();
  }

  void set_tuple_as_array_flag() {
    set_flag<tuple_as_array_flag_e>();
  }
  bool tuple_as_array_flag() const {
    return get_flag<tuple_as_array_flag_e>();
  }

  int get_indirection() const noexcept {
    return indirection_;
  }

  void set_indirection(int indirection) {
    kphp_error(indirection > 0 && indirection <= 0xff, "too many indirection level");
    indirection_ = indirection;
  }

  bool structured() const;
  void make_structured();
  TypeData *clone() const;

  lookup_iterator lookup_begin() const {
    return subkeys.begin();
  }
  lookup_iterator lookup_end() const {
    return subkeys.end();
  }

  const TypeData *lookup_at(const Key &key) const;
  const TypeData *lookup_at_any_key() const { // a bit faster than lookup_at(Key::any_key()), but lookup_at() also surely works
    if (subkeys.empty()) {
      return nullptr;
    }
    const SubkeyItem &front = subkeys.front(); // if any_key exists, it's the only one
    return front.first.is_any_key() ? front.second : nullptr;
  }

  const TypeData *const_read_at(const MultiKey &multi_key) const;
  void set_lca(const TypeData *rhs, bool save_or_false = true, bool save_or_null = true, FFIRvalueFlags ffi_flags = {});
  void set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false = true, bool save_or_null = true, FFIRvalueFlags ffi_flags = {});
  void set_lca(PrimitiveType ptype);
  void fix_inf_array();

  size_t get_tuple_max_index() const;
  const TypeData *get_deepest_type_of_array() const;

  bool did_type_data_change_after_tinf_step(const TypeData *before) const;

  static void init_static();
  static const TypeData *get_type(PrimitiveType type);
  static const TypeData *get_type(PrimitiveType array, PrimitiveType type);
  static const TypeData *create_for_class(ClassPtr klass);
  static const TypeData *create_array_of(const TypeData *element_type);
};

std::string type_out(const TypeData *type, gen_out_style style = gen_out_style::cpp);
int type_strlen(const TypeData *type);
bool can_be_same_type(const TypeData *type1, const TypeData *type2);
bool are_equal_types(const TypeData *type1, const TypeData *type2);
bool is_less_or_equal_type(const TypeData *given, const TypeData *expected, const MultiKey *from_at = nullptr);
bool is_implicit_array_conversion(const TypeData *from, const TypeData *to) noexcept;
