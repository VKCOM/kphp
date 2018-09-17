#include "compiler/types.h"

#include "compiler/data.h"

/*** PrimitiveType ***/
map<string, PrimitiveType> name_to_ptype;

void get_ptype_by_name_init() {
#define FOREACH_PTYPE(tp) name_to_ptype[PTYPE_NAME(tp)] = tp;

#include "foreach_ptype.h"
}

PrimitiveType get_ptype_by_name(const string &s) {
  static bool inited = false;
  if (!inited) {
    get_ptype_by_name_init();
    inited = true;
  }
  map<string, PrimitiveType>::iterator it = name_to_ptype.find(s);
  if (it == name_to_ptype.end()) {
    return tp_Error;
  }
  return it->second;
}

const char *ptype_name(PrimitiveType id) {
  switch (id) {
#define FOREACH_PTYPE(tp) case tp: return PTYPE_NAME (tp);

#include "foreach_ptype.h"

    default:
      return nullptr;
  }
}

bool can_store_bool(PrimitiveType tp) {
  return tp == tp_var || tp == tp_MC || tp == tp_DB || tp == tp_Class ||
         tp == tp_Exception || tp == tp_RPC || tp == tp_bool ||
         tp == tp_Any;
}


PrimitiveType type_lca(PrimitiveType a, PrimitiveType b) {
  if (a == b) {
    return a;
  }
  if (a == tp_Error || b == tp_Error) {
    return tp_Error;
  }

  if (a == tp_Any) {
    return tp_Any;
  }
  if (b == tp_Any) {
    return a;
  }

  if (b == tp_CreateAny) {
    return tp_Any;
  }

  if (a > b) {
    std::swap(a, b);
  }

  if (a == tp_Unknown) {
    return b;
  }

  if (b >= tp_MC && a >= tp_int) { // Memcache and e.t.c can store only bool
    return tp_Error;
  }

  if (b == tp_void) { // void can't store anything (except void)
    return tp_Error;
  }

  if ((b == tp_UInt || b == tp_Long || b == tp_ULong) && a != tp_int) { // UInt, Long and ULong can store only int
    return tp_Error;
  }

  if (tp_int <= a && a <= tp_float && tp_int <= b && b <= tp_float) { // float can store int
    return tp_float;
  }

  if (tp_bool <= a && a <= tp_var && tp_bool <= b && b <= tp_var) {
    return tp_var;
  }

  return max(a, b);
}

/*** Key ***/

namespace {
HT<Key *> int_keys_ht;
HT<Key *> string_keys_ht;
HT<string *> string_key_names_ht;
int n_string_keys_ht = 0;
}

Key::Key() :
  id(-1) {
}

Key::Key(int id) :
  id(id) {
}

Key Key::any_key() {
  return Key(0);
}

Key Key::string_key(const string &key) {
  HT<Key *>::HTNode *node = string_keys_ht.at(hash_ll(key));
  if (node->data != nullptr) {
    return *node->data;
  }

  AutoLocker<Lockable *> locker(node);
  int old_n = atomic_int_inc(&n_string_keys_ht);
  node->data = new Key(old_n * 2 + 2);

  HT<string *>::HTNode *name_node = string_key_names_ht.at(node->data->id);
  dl_assert(name_node->data == nullptr, "");
  name_node->data = new string(key);

  return *node->data;
}

Key Key::int_key(int key) {
  HT<Key *>::HTNode *node = int_keys_ht.at((unsigned int)key);
  if (node->data != nullptr) {
    return *node->data;
  }

  AutoLocker<Lockable *> locker(node);
  node->data = new Key((unsigned int)key * 2 + 1);
  return *node->data;
}

string Key::to_string() const {
  if (is_int_key()) {
    return dl_pstr("%d", (id - 1) / 2);
  }
  if (is_string_key()) {
    return dl_pstr("%s", string_key_names_ht.at(id)->data->c_str());
  }
  if (is_any_key()) {
    return "Any";
  }
  dl_unreachable("...");
  return "fail";
}

/*** MultiKey ***/
MultiKey::MultiKey() :
  keys_() {
}

MultiKey::MultiKey(const vector<Key> &keys) :
  keys_(keys) {
}

MultiKey::MultiKey(const MultiKey &multi_key) :
  keys_(multi_key.keys_) {
}

MultiKey &MultiKey::operator=(const MultiKey &multi_key) {
  if (this == &multi_key) {
    return *this;
  }
  keys_ = multi_key.keys_;
  return *this;
}

void MultiKey::push_back(const Key &key) {
  keys_.push_back(key);
}

void MultiKey::push_front(const Key &key) {
  keys_.insert(keys_.begin(), key);
}

string MultiKey::to_string() const {
  string res;
  for (Key key : *this) {
    res += "[";
    res += key.to_string();
    res += "]";
  }
  return res;
}

MultiKey::iterator MultiKey::begin() const {
  return keys_.begin();
}

MultiKey::iterator MultiKey::end() const {
  return keys_.end();
}

MultiKey::reverse_iterator MultiKey::rbegin() const {
  return keys_.rbegin();
}

MultiKey::reverse_iterator MultiKey::rend() const {
  return keys_.rend();
}

vector<MultiKey> MultiKey::any_key_vec;

void MultiKey::init_static() {
  vector<Key> keys;
  for (int i = 0; i < 10; i++) {
    any_key_vec.push_back(MultiKey(keys));
    keys.push_back(Key::any_key());
  }
}

const MultiKey &MultiKey::any_key(int depth) {
  if (depth >= 0 && depth < (int)any_key_vec.size()) {
    return any_key_vec[depth];
  }
  kphp_fail();
  return any_key_vec[0];
}


/*** TypeData::SubkeysValues ***/

void TypeData::SubkeysValues::add(const Key &key, TypeData *value) {
  KeyValue to_add(key, value);
  auto insert_pos = lower_bound(values_pairs.begin(), values_pairs.end(), KeyValue(key, nullptr));
  values_pairs.insert(insert_pos, to_add);
}

TypeData *TypeData::SubkeysValues::create_if_empty(const Key &key, TypeData *parent) {
  auto it = lower_bound(values_pairs.begin(), values_pairs.end(), KeyValue(key, nullptr));
  if (it != values_pairs.end() && it->first == key) {
    return it->second;
  }

  TypeData *value = get_type(tp_Unknown)->clone();
  value->parent_ = parent;

  KeyValue to_add(key, value);
  values_pairs.insert(it, to_add);
  return value;
}

TypeData *TypeData::SubkeysValues::find(const Key &key) const {
  auto it = lower_bound(values_pairs.begin(), values_pairs.end(), KeyValue(key, nullptr));
  if (it != values_pairs.end() && it->first == key) {
    return it->second;
  }
  return nullptr;
}

inline void TypeData::SubkeysValues::clear() {
  if (!values_pairs.empty()) {
    values_pairs.clear();
  }
}


/*** TypeData ***/

static vector<TypeData *> primitive_types;
static vector<TypeData *> array_types;

void TypeData::init_static() {
  if (!primitive_types.empty()) {
    return;
  }
  primitive_types.resize(ptype_size);
  array_types.resize(ptype_size);
  #define FOREACH_PTYPE(tp) primitive_types[tp] = new TypeData (tp);

  #include "foreach_ptype.h"

  #define FOREACH_PTYPE(tp) \
    array_types[tp] = new TypeData (tp_array); \
    array_types[tp]->set_lca_at (MultiKey::any_key (1), primitive_types[tp ==  tp_Any ? tp_CreateAny : tp]);

  #include "foreach_ptype.h"
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

TypeData::TypeData() :
  ptype_(tp_Unknown),
  flags_(0),
  generation_(current_generation()),
  parent_(nullptr),
  anykey_value(nullptr),
  subkeys_values() {
}

TypeData::TypeData(PrimitiveType ptype) :
  ptype_(ptype),
  flags_(0),
  generation_(current_generation()),
  parent_(nullptr),
  anykey_value(nullptr),
  subkeys_values() {
  if (ptype_ == tp_False) {
    set_or_false_flag(true);
    ptype_ = tp_Unknown;
  }
}

TypeData::TypeData(const TypeData &from) :
  ptype_(from.ptype_),
  class_type_(from.class_type_),
  flags_(from.flags_),
  generation_(from.generation_),
  parent_(nullptr),
  anykey_value(nullptr),
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
  dl_assert (structured(), "bug in TypeData");

  return key.is_any_key() ? anykey_value : subkeys_values.find(key);  // любое может быть nullptr
}

TypeData *TypeData::at_force(const Key &key) {
  dl_assert (structured(), "bug in TypeData");

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

PrimitiveType TypeData::ptype() const {
  return ptype_;
}

PrimitiveType TypeData::get_real_ptype() const {
  PrimitiveType p = ptype();
  if (p == tp_Unknown && or_false_flag()) {
    return tp_bool;
  }
  return p;
}

void TypeData::set_ptype(PrimitiveType new_ptype) {
  if (new_ptype != ptype_) {
    ptype_ = new_ptype;
    if (new_ptype == tp_Error) {
      set_error_flag(true);
    }
    on_changed();
  }
}

ClassPtr TypeData::class_type() const {
  return class_type_;
}

void TypeData::set_class_type(ClassPtr new_class_type) {
  if (class_type_.is_null()) {
    class_type_ = new_class_type;
    on_changed();
  } else if (class_type_.ptr != new_class_type.ptr) {
    // нельзя в одной переменной/массиве смешивать инстансы разных классов
    set_ptype(tp_Error);
  }
}

/**
 * Быстрый аналог !get_all_class_types_inside().empty()
 */
bool TypeData::has_class_type_inside() const {
  if (class_type().not_null()) {
    return true;
  }
  if (anykey_value != nullptr && anykey_value->has_class_type_inside()) {
    return true;
  }
  if (!subkeys_values.empty()) {
    return std::any_of(subkeys_values.begin(), subkeys_values.end(),
                       [](const std::pair<Key, TypeData *> &p) { return p.second->has_class_type_inside(); });
  }
  return false;
}

void TypeData::get_all_class_types_inside(vector<ClassPtr> &out) const {
  if (class_type().not_null()) {
    out.push_back(class_type());
  }
  if (anykey_value != nullptr) {
    anykey_value->get_all_class_types_inside(out);
  }
  for (auto &subkey : subkeys_values) {
    subkey.second->get_all_class_types_inside(out);
  }
}

TypeData::flags_t TypeData::flags() const {
  return flags_;
}

void TypeData::set_flags(TypeData::flags_t new_flags) {
  dl_assert ((flags_ & new_flags) == flags_, "It is forbidden to remove flag");
  if (flags_ != new_flags) {
    if (new_flags & error_flag_e) {
      set_error_flag(true);
    }
    flags_ = new_flags;
    on_changed();
  }
}

bool TypeData::or_false_flag() const {
  return get_flag<or_false_flag_e>();
}

void TypeData::set_or_false_flag(bool f) {
  set_flag<or_false_flag_e>(f);
}

bool TypeData::write_flag() const {
  return get_flag<read_flag_e>();
}

void TypeData::set_write_flag(bool f) {
  set_flag<write_flag_e>(f);
}

bool TypeData::read_flag() const {
  return get_flag<read_flag_e>();
}

void TypeData::set_read_flag(bool f) {
  set_flag<read_flag_e>(f);
}

bool TypeData::error_flag() const {
  return get_flag<error_flag_e>();
}

void TypeData::set_error_flag(bool f) {
  set_flag<error_flag_e>(f);
}

bool TypeData::use_or_false() const {
  return !can_store_bool(ptype()) && or_false_flag();
}

bool TypeData::structured() const {
  return ptype() == tp_array || ptype() == tp_tuple;
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
  if (ptype() == tp_tuple && key.is_any_key()) {
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
  res->set_write_flag(true);
  return res;
}

TypeData *TypeData::lookup_at(const Key &key) const {
  if (!structured()) {
    return nullptr;
  }
  TypeData *res = at(key);
  if (res == nullptr && !key.is_any_key()) {
    res = anykey_value;
  }
  return res;
}

TypeData::lookup_iterator TypeData::lookup_begin() const {
  return structured() ? subkeys_values.begin() : subkeys_values.end();
}

TypeData::lookup_iterator TypeData::lookup_end() const {
  return subkeys_values.end();
}

void TypeData::set_lca(const TypeData *rhs, bool save_or_false) {
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

  TypeData::flags_t mask = save_or_false ? -1 : ~or_false_flag_e;
  TypeData::flags_t new_flags = lhs->flags_ | (rhs->flags_ & mask);
  lhs->set_flags(new_flags);

  if (rhs->ptype() == tp_Class) {
    lhs->set_class_type(rhs->class_type());
  }

  if (!lhs->structured()) {
    return;
  }

  if (new_ptype == tp_tuple && rhs->ptype() == tp_tuple) {
    unsigned int s1 = lhs->subkeys_values.size(), s2 = rhs->subkeys_values.size();
    if (s1 && s2 && s1 != s2) {
      lhs->set_ptype(tp_Error);   // совмещение tuple'ов разных размеров
      return;
    }
  }

  TypeData *lhs_any_key = lhs->at_force(Key::any_key());
  TypeData *rhs_any_key = rhs->lookup_at(Key::any_key());
  lhs_any_key->set_lca(rhs_any_key, true);

  if (!rhs->subkeys_values.empty()) {
    for (auto &rhs_subkey : rhs->subkeys_values) {
      Key rhs_key = rhs_subkey.first;
      TypeData *rhs_value = rhs_subkey.second;
      TypeData *lhs_value = lhs->subkeys_values.create_if_empty(rhs_key, lhs);
      lhs_value->set_lca(rhs_value, true);
    }
  }
}

void TypeData::set_lca_at(const MultiKey &multi_key, const TypeData *rhs, bool save_or_false) {
  TypeData *cur = this;
  for (auto key : multi_key) {
    cur = cur->write_at(key);
    if (cur == nullptr) {
      return;
    }
  }
  cur->set_lca(rhs, save_or_false);
  for (MultiKey::reverse_iterator it = multi_key.rbegin(); it != multi_key.rend(); it++) {
    cur = cur->parent_;
    if (*it != Key::any_key()) {
      TypeData *any_value = cur->at_force(Key::any_key());
      TypeData *key_value = cur->write_at(*it);
      any_value->set_lca(key_value, true);
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
  if (parent_->ptype() == tp_tuple && parent_->anykey_value == this) {
    return false;   // tp_tuple any key может быть tp_Error (к примеру, tuple(1, new A)), сам tuple от этого не error
  }
  return true;
}

void TypeData::set_lca(PrimitiveType ptype) {
  set_lca(TypeData::get_type(ptype), true);
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

static inline int cmp(const TypeData *a, const TypeData *b) {
  if (a == b) {
    return 0;
  }
  if (a == nullptr) {
    return -1;
  }
  if (b == nullptr) {
    return 1;
  }
  if (a->ptype() < b->ptype()) {
    return -1;
  }
  if (a->ptype() > b->ptype()) {
    return +1;
  }
  if (a->flags() < b->flags()) {
    return -1;
  }
  if (a->flags() > b->flags()) {
    return +1;
  }

  int res;
  res = cmp(a->lookup_at(Key::any_key()), b->lookup_at(Key::any_key()));
  if (res) {
    return res;
  }

  TypeData::lookup_iterator a_begin = a->lookup_begin(),
    a_end = a->lookup_end(),
    b_begin = b->lookup_begin(),
    b_end = b->lookup_end();
  int a_size = (int)(a_end - a_begin);
  int b_size = (int)(b_end - b_begin);
  if (a_size < b_size) {
    return -1;
  }
  if (a_size > b_size) {
    return +1;
  }

  while (a_begin != a_end) {
    if (a_begin->first < b_begin->first) {
      return -1;
    }
    if (a_begin->first > b_begin->first) {
      return +1;
    }
    res = cmp(a_begin->second, b_begin->second);
    if (res) {
      return res;
    }
    a_begin++;
    b_begin++;
  }
  return 0;
}

bool operator<(const TypeData &a, const TypeData &b) {
  return cmp(&a, &b) < 0;
}

bool operator==(const TypeData &a, const TypeData &b) {
  return cmp(&a, &b) == 0;
}

void type_out_impl(const TypeData *type, string *res) {
  PrimitiveType tp = type->get_real_ptype();
  bool or_false = type->use_or_false() && tp != tp_bool;

  if (or_false) {
    *res += "OrFalse < ";
  }

  if (tp == tp_DB) {
    *res += "MyDB";
  } else if (tp == tp_MC) {
    *res += "MyMemcache";
  } else if (tp == tp_Class) {
    *res += "class_instance<";
    *res += type->class_type()->src_name;
    *res += ">";
  } else if (tp == tp_RPC) {
    *res += "rpc_connection";
  } else if (tp == tp_float) {
    *res += "double";
  } else {
    *res += ptype_name(tp);
  }

  bool need_any_key = tp == tp_array;
  const TypeData *anykey_value = need_any_key ? type->lookup_at(Key::any_key()) : nullptr;
  if (anykey_value) {
    *res += "< ";
    type_out_impl(anykey_value, res);
    *res += " >";
  }

  bool need_all_subkeys = tp == tp_tuple;
  if (need_all_subkeys) {
    *res += "< ";
    for (auto subkey = type->lookup_begin(); subkey != type->lookup_end(); ++subkey) {
      if (subkey != type->lookup_begin()) {
        *res += ",";
      }
      kphp_assert(subkey->first.is_int_key());
      type_out_impl(type->const_read_at(subkey->first), res);
    }
    *res += " >";
  }

  if (or_false) {
    *res += " >";
  }
}

string type_out(const TypeData *type) {
  string res;
  type_out_impl(type, &res);
  return res;
}

int type_strlen(const TypeData *type) {
  PrimitiveType tp = type->ptype();
  switch (tp) {
    case tp_Unknown:
      if (type->use_or_false()) {
        return STRLEN_FALSE;
      }
      return STRLEN_UNKNOWN;
    case tp_False:
      return STRLEN_FALSE_;
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
    case tp_MC:
      return STRLEN_MC;
    case tp_DB:
      return STRLEN_DB;
    case tp_RPC:
      return STRLEN_RPC;
    case tp_Exception:
      return STRLEN_EXCEPTION;
    case tp_Class:
      return STRLEN_CLASS;
    case tp_void:
      return STRLEN_VOID;
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

bool can_store_false(const TypeData *type) {
  return type->ptype() == tp_False || can_store_bool(type->ptype()) || type->or_false_flag();
}

bool can_be_same_type(const TypeData *type1, const TypeData *type2) {
  if (type1->ptype() == tp_var || type2->ptype() == tp_var) {
    return true;
  }
  if (can_store_false(type1) && can_store_false(type2)) {
    return true;
  }
  if (type1->ptype() == type2->ptype()) {
    return true;
  }
  return false;
}


void test_PrimitiveType() {
  const char *cur;
  cur = ptype_name<tp_Unknown>();
  assert (cur != nullptr);
  assert (!strcmp(cur, "Unknown"));

  cur = ptype_name<ptype_size>();
  assert (cur == nullptr);

  cur = ptype_name(ptype_size);
  assert (cur == nullptr);

  cur = ptype_name(tp_array);
  assert (cur != nullptr);
  assert (!strcmp(cur, "array"));


  PrimitiveType tp;
  tp = get_ptype_by_name("array");
  assert (tp == tp_array);
  tp = get_ptype_by_name("MC");
  assert (tp == tp_MC);
  tp = get_ptype_by_name("XX");
  assert (tp == tp_Error);
  tp = get_ptype_by_name("X");
  assert (tp == tp_Error);
  tp = get_ptype_by_name("");
  assert (tp == tp_Error);
}

void test_TypeData() {
  TypeData *a = TypeData::get_type(tp_Unknown)->clone();

  assert (type_out(a) == "Unknown");
  a->set_lca_at(MultiKey::any_key(2), TypeData::get_type(tp_int));
  assert (type_out(a) == "array< array< int > >");
  a->set_lca(TypeData::get_type(tp_var));
  assert (type_out(a) == "var");
  a->set_lca_at(MultiKey::any_key(2), TypeData::get_type(tp_int));
  assert (type_out(a) == "var");


  printf("OK\n");
}
