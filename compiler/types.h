#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"
#include "compiler/utils.h"

//H
/*** PrimitiveType ***/

//foreach_ptype.h contain calls of FOREACH_PTYPE(tp) for each primitive_type name.
//All new primitive types should be added to foreach_ptype.h
//All primitive type names must start with tp_
enum PrimitiveType {
#define FOREACH_PTYPE(tp) tp,

#include "foreach_ptype.h"

  ptype_size
};

//interface to PrimitiveType
template<PrimitiveType T_ID>
inline const char *ptype_name();
const char *ptype_name(PrimitiveType tp);
PrimitiveType get_ptype_by_name(const string &s);
PrimitiveType type_lca(PrimitiveType a, PrimitiveType b);
bool can_store_bool(PrimitiveType tp);

/*** Key ***/
class Key {
private:
  int id;

  explicit Key(int id);

public:
  Key();
  Key(const Key &other) = default;
  Key &operator=(const Key &other) = default;

  static Key any_key();
  static Key string_key(const string &key);
  static Key int_key(int key);

  string to_string() const;

  inline bool is_any_key() const { return id == 0; }

  inline bool is_int_key() const { return id > 0 && id % 2 == 1; }

  inline bool is_string_key() const { return id > 0 && id % 2 == 0; }

  friend inline bool operator<(const Key &a, const Key &b);
  friend inline bool operator>(const Key &a, const Key &b);
  friend inline bool operator<=(const Key &a, const Key &b);
  friend inline bool operator!=(const Key &a, const Key &b);
  friend inline bool operator==(const Key &a, const Key &b);
};

inline bool operator<(const Key &a, const Key &b);
inline bool operator>(const Key &a, const Key &b);
inline bool operator<=(const Key &a, const Key &b);
inline bool operator!=(const Key &a, const Key &b);
inline bool operator==(const Key &a, const Key &b);

/*** MultiKey ***/
class MultiKey {
private:
  vector<Key> keys_;
public:
  typedef vector<Key>::const_iterator iterator;
  typedef vector<Key>::const_reverse_iterator reverse_iterator;
  MultiKey();
  MultiKey(const MultiKey &multi_key);
  MultiKey &operator=(const MultiKey &multi_key);
  explicit MultiKey(const vector<Key> &keys);
  void push_back(const Key &key);
  void push_front(const Key &key);
  string to_string() const;

  inline unsigned int depth() const { return (unsigned int)keys_.size(); }

  iterator begin() const;
  iterator end() const;
  reverse_iterator rbegin() const;
  reverse_iterator rend() const;

  static vector<MultiKey> any_key_vec;
  static void init_static();
  static const MultiKey &any_key(int depth);
};

/*** TypeData ***/
// read/write/lookup at
// check if something changed since last

class TypeData {
public:
  typedef pair<Key, TypeData *> KeyValue;
  typedef vector<KeyValue>::const_iterator lookup_iterator;
  typedef long generation_t;
  typedef unsigned long flags_t;
private:

  class SubkeysValues {
  private:
    vector<KeyValue> values_pairs;

  public:
    void add(const Key &key, TypeData *value);
    TypeData *create_if_empty(const Key &key, TypeData *parent);
    TypeData *find(const Key &key) const;
    void clear();

    inline vector<KeyValue>::iterator begin() { return values_pairs.begin(); }

    inline vector<KeyValue>::iterator end() { return values_pairs.end(); }

    inline vector<KeyValue>::const_iterator begin() const { return values_pairs.begin(); }

    inline vector<KeyValue>::const_iterator end() const { return values_pairs.end(); }

    inline bool empty() const { return values_pairs.empty(); }

    inline unsigned int size() const { return (unsigned int)values_pairs.size(); }
  };

  PrimitiveType ptype_;
  ClassPtr class_type_;
  flags_t flags_;
  generation_t generation_;

  TypeData *parent_;
  TypeData *anykey_value;
  SubkeysValues subkeys_values;

  static TLS<generation_t> current_generation_;
  TypeData();
  explicit TypeData(PrimitiveType ptype);

  TypeData *at(const Key &key) const;
  TypeData *at_force(const Key &key);

  enum flag_id_t {
    write_flag_e = 1,
    read_flag_e = 2,
    or_false_flag_e = 4,
    error_flag_e = 8
  };

  template<TypeData::flag_id_t FLAG>
  inline bool get_flag() const {
    return flags_ & FLAG;
  }

  template<flag_id_t FLAG>
  void set_flag(bool f);

  TypeData *write_at(const Key &key);

public:
  TypeData(const TypeData &from);
  ~TypeData();

  PrimitiveType ptype() const;
  PrimitiveType get_real_ptype() const;
  flags_t flags() const;
  void set_ptype(PrimitiveType new_ptype);

  ClassPtr class_type() const;
  void set_class_type(ClassPtr new_class_type);
  bool has_class_type_inside() const;
  void get_all_class_types_inside(vector<ClassPtr> &out) const;

  bool or_false_flag() const;
  void set_or_false_flag(bool f);
  bool use_or_false() const;
  bool write_flag() const;
  void set_write_flag(bool f);
  bool read_flag() const;
  void set_read_flag(bool f);
  bool error_flag() const;
  void set_error_flag(bool f);
  void set_flags(flags_t new_flags);

  bool structured() const;
  void make_structured();
  generation_t generation() const;
  void on_changed();
  TypeData *clone() const;

  TypeData *lookup_at(const Key &key) const;
  lookup_iterator lookup_begin() const;
  lookup_iterator lookup_end() const;

  const TypeData *const_read_at(const Key &key) const;
  const TypeData *const_read_at(const MultiKey &multi_key) const;
  void set_lca(const TypeData *rhs, bool save_or_false = true);
  void set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false = true);
  void set_lca(PrimitiveType ptype);
  void fix_inf_array();
  bool should_proxy_error_flag_to_parent() const;

  static void init_static();
  static const TypeData *get_type(PrimitiveType type);
  static const TypeData *get_type(PrimitiveType array, PrimitiveType type);
  //FIXME:??
  static void inc_generation();
  static generation_t current_generation();
  static void upd_generation(TypeData::generation_t other_generation);
};

inline bool operator<(const TypeData::KeyValue &a, const TypeData::KeyValue &b);
bool operator<(const TypeData &a, const TypeData &b);
bool operator==(const TypeData &a, const TypeData &b);

string type_out(const TypeData *type);
int type_strlen(const TypeData *type);
bool can_be_same_type(const TypeData *type1, const TypeData *type2);

void test_TypeData();
void test_PrimitiveType();

template<TypeData::flag_id_t FLAG>
void TypeData::set_flag(bool f) {
  bool old_f = get_flag<FLAG>();
  if (old_f) {
    dl_assert (f, dl_pstr("It is forbidden to remove flag %d", FLAG));
  } else if (f) {
    flags_ |= FLAG;
    on_changed();
  }
}

template<>
inline void TypeData::set_flag<TypeData::error_flag_e>(bool f) {
  if (f) {
    if (!get_flag<error_flag_e>()) {
      flags_ |= error_flag_e;
      if (parent_ != nullptr && should_proxy_error_flag_to_parent()) {
        parent_->set_flag<error_flag_e>(true);
      }
    }
  }
}

#include "types.hpp"
