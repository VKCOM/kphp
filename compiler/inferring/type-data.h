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
  DEBUG_STRING_METHOD { return as_human_readable(false); }

private:
  enum flag_id_t : uint8_t {
    write_flag_e          = 0b00000001,
    or_null_flag_e        = 0b00000010,
    or_false_flag_e       = 0b00000100,
    shape_has_varg_flag_e = 0b00001000,
  };

public:
  using KeyValue = std::pair<Key, TypeData *>;
  using lookup_iterator = std::forward_list<KeyValue>::const_iterator;

private:
  class SubkeysValues {
  private:
    std::forward_list<KeyValue> values_pairs_;

  public:
    void add(const Key &key, TypeData *value) { values_pairs_.emplace_front(key, value); }
    TypeData *create_if_empty(const Key &key);
    TypeData *find(const Key &key) const;
    
    TypeData *get_at_any_key() const { // a bit faster than to call find(Key::any_key()), but find() also surely works
      if (values_pairs_.empty()) {
        return nullptr;
      }
      const KeyValue &front = values_pairs_.front();  // if any_key exists, it's the only one
      return front.first.is_any_key() ? front.second : nullptr;
    }

    inline auto begin() { return values_pairs_.begin(); }
    inline auto end() { return values_pairs_.end(); }
    inline auto begin() const { return values_pairs_.cbegin(); }
    inline auto end() const { return values_pairs_.cend(); }

    inline bool empty() const { return values_pairs_.empty(); }
    inline auto size() const { return std::distance(begin(), end()); }
    inline void clear() { values_pairs_.clear(); }
  };

  PrimitiveType ptype_ : 8;     // current type (int/array/etc); tp_any for uninited, tp_Error if error
  uint8_t flags_{0};            // a binary mask of flag_id_t

  // current class for tp_Class (but during inferring it could contain many classes due to multiple implements)
  std::forward_list<ClassPtr> class_type_;
  // subkeys are inner representations for arrays, tuples and other types containing other types
  // for non-structured, it's empty (although it could remain any_key if array turned to mixed, then it doesn't make sense)
  // for array/future, it contains a single item — {any_key, anykey_type}
  // for tuples/shapes, it contains multiple items — {key1, key1_type}, {key2, key2_type}, order is undetermined
  SubkeysValues subkeys_values;

  
  explicit TypeData(PrimitiveType ptype);

  TypeData *at_force(const Key &key);
  TypeData *write_at(const Key &key);
  const TypeData *const_read_at(const Key &key) const;

  template<flag_id_t FLAG>
  inline bool get_flag() const { return flags_ & FLAG; }

  template<flag_id_t FLAG>
  void set_flag() { flags_ |= FLAG; }

  template<typename F>
  bool for_each_deep(const F &visitor) const;

public:
  TypeData &operator=(const TypeData &) = delete;
  TypeData(const TypeData &from);
  ~TypeData();

  std::string as_human_readable(bool colored = true) const;

  PrimitiveType ptype() const { return ptype_; }
  PrimitiveType get_real_ptype() const;
  void set_ptype(PrimitiveType new_ptype);

  const std::forward_list<ClassPtr> &class_types() const { return class_type_; }
  ClassPtr class_type() const;
  void set_class_type(const std::forward_list<ClassPtr> &new_class_type);
  bool has_class_type_inside() const;
  bool has_tp_any_inside() const;
  void mark_classes_used() const;
  void get_all_class_types_inside(std::unordered_set<ClassPtr> &out) const;
  ClassPtr get_first_class_type_inside() const;
  bool is_primitive_type() const;

  uint8_t flags() const { return flags_; }
  void set_flags(uint8_t new_flags);

  bool or_false_flag() const { return get_flag<or_false_flag_e>(); }
  void set_or_false_flag() { set_flag<or_false_flag_e>(); }
  bool use_or_false() const { return or_false_flag() && !::can_store_false(ptype_); }
  bool can_store_false() const;

  bool or_null_flag() const { return get_flag<or_null_flag_e>(); }
  void set_or_null_flag() { set_flag<or_null_flag_e>(); }
  bool use_or_null() const { return or_null_flag() && !::can_store_null(ptype_); }
  bool can_store_null() const;

  bool use_optional() const { return use_or_false() || use_or_null(); }

  void set_write_flag() { set_flag<write_flag_e>(); }

  bool error_flag() const { return ptype_ == tp_Error; }

  void set_shape_has_varg_flag() { set_flag<shape_has_varg_flag_e>(); }
  bool shape_has_varg_flag() const { return get_flag<shape_has_varg_flag_e>(); }

  bool structured() const;
  void make_structured();
  TypeData *clone() const;

  const TypeData *lookup_at(const Key &key) const { return subkeys_values.find(key); }
  const TypeData *lookup_at_any_key() const { return subkeys_values.get_at_any_key(); }
  lookup_iterator lookup_begin() const { return subkeys_values.begin(); }
  lookup_iterator lookup_end() const { return subkeys_values.end(); }

  const TypeData *const_read_at(const MultiKey &multi_key) const;
  void set_lca(const TypeData *rhs, bool save_or_false = true, bool save_or_null = true);
  void set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false = true, bool save_or_null = true);
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
