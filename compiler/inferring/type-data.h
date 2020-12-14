// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <forward_list>
#include <map>
#include <string>

#include "common/algorithms/find.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/code-gen/gen-out-style.h"
#include "compiler/data/data_ptr.h"
#include "compiler/inferring/key.h"
#include "compiler/inferring/multi-key.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/stage.h"
#include "compiler/threading/tls.h"

/*** TypeData ***/
// read/write/lookup at
// check if something changed since last

class TypeData {
private:
  enum flag_id_t : uint8_t {
    write_flag_e          = 0b00000001,
    or_null_flag_e        = 0b00000010,
    or_false_flag_e       = 0b00000100,
    error_flag_e          = 0b00001000,
    shape_has_varg_flag_e = 0b00010000,
  };

public:
  using KeyValue = std::pair<Key, TypeData *>;
  using lookup_iterator = std::forward_list<KeyValue>::const_iterator;
  using generation_t = uint32_t;
  using flags_t = std::underlying_type_t<flag_id_t>;

private:
  class SubkeysValues {
  private:
    std::forward_list<KeyValue> values_pairs_;

  public:
    void add(const Key &key, TypeData *value) { values_pairs_.emplace_front(key, value); }
    TypeData *create_if_empty(const Key &key, TypeData *parent);
    TypeData *find(const Key &key) const;

    inline auto begin() { return values_pairs_.begin(); }
    inline auto end() { return values_pairs_.end(); }
    inline auto begin() const { return values_pairs_.cbegin(); }
    inline auto end() const { return values_pairs_.cend(); }

    inline bool empty() const { return values_pairs_.empty(); }
    inline auto size() const { return std::distance(begin(), end()); }
    inline void clear() { values_pairs_.clear(); }
  };

  // There is a bug in GCC before 8.4 where we can't declare underlying type of the enum PrimitiveType
  // due-to vertex-meta-op_base.h:33, where the PrimiviteType used as a bit field
  // GCC produces a false positive warning, which you may not disable;
  // Later we need to add uint8_t underlying type for the enum PrimitiveType
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61414
  PrimitiveType ptype_ : 8;
  flags_t flags_{0};
  generation_t generation_;

  std::forward_list<ClassPtr> class_type_;
  TypeData *parent_{nullptr};
  TypeData *anykey_value{nullptr};
  SubkeysValues subkeys_values;

  static TLS<generation_t> current_generation_;
  explicit TypeData(PrimitiveType ptype = tp_Unknown);

  TypeData *at(const Key &key) const;
  TypeData *at_force(const Key &key);

  template<flag_id_t FLAG>
  inline bool get_flag() const {
    return flags_ & FLAG;
  }

  template<flag_id_t FLAG>
  void set_flag();

  TypeData *write_at(const Key &key);

  template<typename F>
  bool for_each_deep(const F &visitor) const;

public:
  TypeData &operator=(const TypeData &) = delete;
  TypeData(const TypeData &from);
  ~TypeData();

  PrimitiveType ptype() const { return static_cast<PrimitiveType>(ptype_); }
  PrimitiveType get_real_ptype() const;
  flags_t flags() const;
  void set_ptype(PrimitiveType new_ptype);

  const std::forward_list<ClassPtr> &class_types() const;
  ClassPtr class_type() const;
  void set_class_type(const std::forward_list<ClassPtr> &new_class_type);
  bool has_class_type_inside() const;
  void mark_classes_used() const;
  void get_all_class_types_inside(std::unordered_set<ClassPtr> &out) const;
  ClassPtr get_first_class_type_inside() const;
  bool is_primitive_type() const;

  bool or_false_flag() const { return get_flag<or_false_flag_e>(); }
  void set_or_false_flag() { set_flag<or_false_flag_e>(); }
  bool use_or_false() const;
  bool can_store_false() const;

  bool or_null_flag() const { return get_flag<or_null_flag_e>(); }
  void set_or_null_flag() { set_flag<or_null_flag_e>(); }
  bool use_or_null() const;
  bool can_store_null() const;

  bool use_optional() const;

  void set_write_flag() { set_flag<write_flag_e>(); }

  void set_error_flag() { set_flag<error_flag_e>(); }
  bool error_flag() const { return get_flag<error_flag_e>(); }

  void set_shape_has_varg_flag() { set_flag<shape_has_varg_flag_e>(); }
  bool shape_has_varg_flag() const { return get_flag<shape_has_varg_flag_e>(); }

  void set_flags(flags_t new_flags);

  bool structured() const;
  void make_structured();
  generation_t generation() const;
  void on_changed();
  TypeData *clone() const;
  void convert_Unknown_to_Any();

  TypeData *lookup_at(const Key &key) const;
  lookup_iterator lookup_begin() const;
  lookup_iterator lookup_end() const;

  const TypeData *get_deepest_type_of_array() const;

  const TypeData *const_read_at(const Key &key) const;
  const TypeData *const_read_at(const MultiKey &multi_key) const;
  void set_lca(const TypeData *rhs, bool save_or_false = true, bool save_or_null = true);
  void set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false = true, bool save_or_null = true);
  void set_lca(PrimitiveType ptype);
  void fix_inf_array();
  bool should_proxy_error_flag_to_parent() const;

  size_t get_tuple_max_index() const;

  static void init_static();
  static const TypeData *get_type(PrimitiveType type);
  static const TypeData *get_type(PrimitiveType array, PrimitiveType type);
  static const TypeData *create_for_class(ClassPtr klass);
  template<class T>
  static const TypeData *create_type_data(const T &element_type, bool or_null, bool or_false) {
    auto res = create_type_data(element_type);
    if (or_null) {
      res->set_or_null_flag();
    }
    if (or_false) {
      res->set_or_false_flag();
    }
    return res;
  }
  static TypeData *create_type_data(const TypeData *element_type);
  static TypeData *create_type_data(const std::vector<const TypeData *> &subkeys_values);
  static TypeData *create_type_data(const std::map<std::string, const TypeData *> &subkeys_values);
  //FIXME:??
  static void inc_generation();
  static generation_t current_generation();
  static void upd_generation(TypeData::generation_t other_generation);
};

std::string type_out(const TypeData *type, gen_out_style style = gen_out_style::cpp);
std::string colored_type_out(const TypeData *type);
int type_strlen(const TypeData *type);
bool can_be_same_type(const TypeData *type1, const TypeData *type2);
bool are_equal_types(const TypeData *type1, const TypeData *type2);
bool is_implicit_array_conversion(const TypeData *from, const TypeData *to) noexcept;

template<TypeData::flag_id_t FLAG>
void TypeData::set_flag() {
  if (!get_flag<FLAG>()) {
    flags_ |= FLAG;

    if (FLAG == error_flag_e) {
      if (parent_ != nullptr && should_proxy_error_flag_to_parent()) {
        parent_->set_flag<error_flag_e>();
      }
    } else {
      on_changed();
    }
  }
}
