// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <libgen.h>
#include <string>

#include "common/php-functions.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/define-data.h"
#include "compiler/data/var-data.h"
#include "compiler/compiler-core.h"
#include "compiler/name-gen.h"
#include "compiler/operation.h"
#include "compiler/pipes/check-access-modifiers.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "compiler/vertex-util.h"

template<typename T>
struct ConstManipulations {
public:
  virtual ~ConstManipulations() = default;

protected:
  virtual T on_trivial(VertexPtr v) { return on_non_const(v); }

  virtual T on_conv(VertexAdaptor<meta_op_unary> v) { return on_non_const(v); }

  virtual T on_unary(VertexAdaptor<meta_op_unary> v) { return on_non_const(v); }

  virtual T on_binary(VertexAdaptor<meta_op_binary> v) { return on_non_const(v); }

  virtual T on_double_arrow(VertexAdaptor<op_double_arrow> v) { return on_non_const(v); }

  virtual bool on_array_value([[maybe_unused]] VertexAdaptor<op_array> array, [[maybe_unused]] size_t ind) { return false; }

  virtual T on_array_finish(VertexAdaptor<op_array> v) { return on_non_const(v); }

  virtual T on_func_name(VertexAdaptor<op_func_name> v) { return on_non_const(v); }

  virtual T on_var(VertexAdaptor<op_var> v) { return on_non_const(v); }

  virtual T on_instance_prop(VertexAdaptor<op_instance_prop> v) { return on_non_const(v); }

  virtual T on_define_val(VertexAdaptor<op_define_val> v) { return on_non_const(v); }

  virtual T on_func_call(VertexAdaptor<op_func_call> v) { return on_non_const(v); }

  virtual T on_alloc(VertexAdaptor<op_alloc> v) { return on_non_const(v); }

  virtual T on_non_const([[maybe_unused]] VertexPtr vertex) { return T(); }

  virtual T on_array(VertexAdaptor<op_array> v) {
    VertexRange arr = v->args();
    for (size_t i = 0; i < arr.size(); ++i) {
      if (!on_array_value(v, i)) {
        return on_non_const(v);
      }
    }
    return on_array_finish(v);
  }

  virtual bool on_index_key([[maybe_unused]] VertexPtr key) { return false; }

  virtual T on_index(VertexAdaptor<op_index> index) {
    if (index->has_key() && !on_index_key(index->key())) {
      return on_non_const(index->key());
    }
    return visit(index->array());
  }

  T visit(VertexPtr v) {
    switch (v->type()) {
      case op_conv_int:
      case op_conv_int_l:
      case op_conv_float:
      case op_conv_string:
      case op_conv_string_l:
      case op_conv_array:
      case op_conv_array_l:
      case op_conv_object:
      case op_conv_bool:
      case op_conv_mixed:
      case op_force_mixed:
      case op_conv_regexp:
        return on_conv(v.as<meta_op_unary>());

      case op_int_const:
      case op_float_const:
      case op_string:
      case op_false:
      case op_true:
      case op_null:
        return on_trivial(v);

      case op_minus:
      case op_plus:
      case op_not:
        return on_unary(v.as<meta_op_unary>());

      case op_add:
      case op_mul:
      case op_sub:
      case op_div:
      case op_mod:
      case op_pow:
      case op_and:
      case op_or:
      case op_xor:
      case op_shl:
      case op_shr:
        return on_binary(v.as<meta_op_binary>());

      case op_array:
        return on_array(v.as<op_array>());

      case op_index:
        return on_index(v.as<op_index>());

      case op_var:
        return on_var(v.as<op_var>());

      case op_instance_prop:
        return on_instance_prop(v.as<op_instance_prop>());

      case op_func_name:
        return on_func_name(v.as<op_func_name>());

      case op_double_arrow:
        return on_double_arrow(v.as<op_double_arrow>());

      case op_define_val:
        return on_define_val(v.as<op_define_val>());

      case op_func_call:
        return on_func_call(v.as<op_func_call>());

      case op_alloc:
        return on_alloc(v.as<op_alloc>());

      default:
        return on_non_const(v);
    }
  }
};

// CheckConst is used as Base class below
// And in name-gen.cpp, to check -- whether we should hash and utilize as const variable or not
struct CheckConst
  : ConstManipulations<bool> {
public:
  static bool is_const(VertexPtr v) {
    static CheckConst check_const;
    return check_const.visit(v);
  }

protected:
  bool on_trivial(VertexPtr) override {
    return true;
  }

  bool on_conv(VertexAdaptor<meta_op_unary> v) override {
    return visit(v->expr());
  }

  bool on_unary(VertexAdaptor<meta_op_unary> v) override {
    return visit(v->expr());
  }

  bool on_binary(VertexAdaptor<meta_op_binary> v) override {
    VertexPtr lhs = VertexUtil::get_actual_value(v->lhs());
    VertexPtr rhs = VertexUtil::get_actual_value(v->rhs());
    return visit(lhs) && visit(rhs);
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) override {
    auto ith_element = v->args()[ind];
    if (auto double_arrow = ith_element.try_as<op_double_arrow>()) {
      VertexPtr key = VertexUtil::get_actual_value(double_arrow->key());
      VertexPtr value = VertexUtil::get_actual_value(double_arrow->value());
      return visit(key) && visit(value);
    }
    return visit(VertexUtil::get_actual_value(ith_element));
  }

  bool on_var(VertexAdaptor<op_var> v) override {
    if (v->var_id && (v->extra_type == op_ex_var_const || v->var_id->is_constant())) {
      return visit(v->var_id->init_val);
    }

    return false;
  }

  bool on_array_finish([[maybe_unused]] VertexAdaptor<op_array> array) override {
    return true;
  }

  bool on_index_key(VertexPtr key) override {
    return visit(key);
  }

  bool on_define_val(VertexAdaptor<op_define_val> v) override {
    return visit(v->value());
  }
};


// CheckConstWithDefines is used in CalcRealDefinesValues pass
// To choose correct `def->val->const_type` (cnst_const_val or def_var)
struct CheckConstWithDefines final
  : CheckConst {
public:
  bool is_const(VertexPtr v) {
    return visit(v);
  }

protected:
  bool on_trivial(VertexPtr v) final {
    return in_concat == 0 || v->has_get_string();
  }

  bool on_func_name(VertexAdaptor<op_func_name> v) final {
    std::string name = resolve_define_name(v->str_val);
    DefinePtr define = G->get_define(name);
    if (define) {
      return visit(define->val);
    }
    return false;
  }

  bool on_func_call(VertexAdaptor<op_func_call> v) override {
    // Allows use objects in const context
    return v->extra_type == op_ex_constructor_call;
  }

  bool on_non_const(VertexPtr v) final {
    if (v->type() == op_concat || v->type() == op_string_build) {
      in_concat++;

      for (auto i : *v) {
        if (!visit(i)) {
          in_concat--;
          return false;
        }
      }
      in_concat--;
      return true;
    }

    return false;
  }

private:
  int in_concat = 0;
};

struct CheckConstAccess final
  : CheckConst {
private:
  ClassPtr caller_class_id;
public:
  void check(VertexPtr v, ClassPtr class_id) {
    caller_class_id = class_id;
    visit(v);
  }

  bool on_func_name(VertexAdaptor<op_func_name> v) final {
    auto define = G->get_define(resolve_define_name(v->str_val));
    if (define->class_id) {
      check_access(caller_class_id, ClassPtr{nullptr}, FieldModifiers{define->access}, define->class_id, "const", define->name);
    }

    return true;
  }
};

// moved from the collect-required-and-classes.cpp to make it more reusable
static inline std::string collect_string_concatenation(VertexPtr v, bool allow_dirname = false) {
  if (auto string = v.try_as<op_string>()) {
    return string->str_val;
  }
  if (auto call = v.try_as<op_func_call>(); allow_dirname && call && call->str_val == "dirname") {
    kphp_error_act(call->size() == 1, "call to 'dirname' must have one arg", return {});
    auto dirname_arg = call->back().try_as<op_string>();
    kphp_error_act(dirname_arg, "'dirname' has to be called with string type arg", return {});
    auto path_copy = dirname_arg->str_val;
    return dirname(path_copy.data());
  }
  if (auto concat = v.try_as<op_concat>()) {
    auto left = collect_string_concatenation(concat->lhs(), allow_dirname);
    auto right = collect_string_concatenation(concat->rhs(), allow_dirname);
    return (left.empty() || right.empty()) ? std::string() : (left + right);
  }
  return {};
}

// MakeConst is used in CalcRealDefinesValues pass
// To choose correct `def->val->const_type` (cnst_const_val or def_var)
struct MakeConst final
  : ConstManipulations<VertexPtr> {
public:
  VertexPtr make_const(VertexPtr v) {
    return visit(v);
  }

protected:
  VertexPtr on_trivial(VertexPtr v) final {
    return v;
  }

  VertexPtr on_unary(VertexAdaptor<meta_op_unary> v) final {
    v->expr() = make_const(v->expr());
    return v;
  }

  VertexPtr on_binary(VertexAdaptor<meta_op_binary> v) final {
    v->lhs() = make_const(v->lhs());
    v->rhs() = make_const(v->rhs());
    return v;
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) final {
    auto ith_element = v->args()[ind];
    if (auto double_arrow = ith_element.try_as<op_double_arrow>()) {
      double_arrow->key() = make_const(double_arrow->key());
      double_arrow->value() = make_const(double_arrow->value());
    } else {
      v->args()[ind] = make_const(ith_element);
    }
    return true;
  }

  VertexPtr on_array_finish(VertexAdaptor<op_array> v) final {
    return v;
  }

  VertexPtr on_conv(VertexAdaptor<meta_op_unary> v) final {
    v->expr() = make_const(v->expr());
    return v;
  }

  VertexPtr on_func_name(VertexAdaptor<op_func_name> v) final {
    std::string name = resolve_define_name(v->str_val);
    auto define_id = G->get_define(name);
    auto response = VertexAdaptor<op_define_val>::create(define_id->val);
    response->define_id = define_id;
    return response;
  }

  VertexPtr on_non_const(VertexPtr v) final {
    if (v->type() == op_concat || v->type() == op_string_build) {
      auto new_val = VertexAdaptor<op_string>::create();
      new_val->location = v->get_location();

      for (auto i : *v) {
        VertexPtr res = VertexUtil::get_actual_value(visit(i));
        kphp_error(res->has_get_string(), ("expected type convertible to string, but got: " + OpInfo::str(res->type())).c_str());
        new_val->str_val += res->get_string();
      }

      return new_val;
    }

    return {};
  }

  VertexPtr on_index(VertexAdaptor<op_index> index) final {
    if (index->has_key()) {
      index->key() = make_const(index->key());
    }
    index->array() = make_const(index->array());
    return index;
  }

  VertexPtr on_define_val(VertexAdaptor<op_define_val> v) final {
    v->value() = make_const(v->value());
    return v;
  }

  VertexPtr on_func_call(VertexAdaptor<op_func_call> v) final {
    return v;
  }

};
struct CommonHash
  : ConstManipulations<void> {

  void feed_hash(uint64_t val) {
    cur_hash = cur_hash * HASH_MULT + val;
  }

  void feed_hash_string(const std::string &s) {
    feed_hash(string_hash(s.c_str(), s.size()));
  }

protected:
  void on_trivial(VertexPtr v) override {
    std::string s = OpInfo::str(v->type());

    if (v->has_get_string()) {
      s += v->get_string();
    }

    feed_hash_string(s);
  }

  void on_conv(VertexAdaptor<meta_op_unary> v) override {
    feed_hash_string(OpInfo::str(v->type()));
    return visit(v->expr());
  }

  void on_unary(VertexAdaptor<meta_op_unary> v) override {
    std::string type_str = OpInfo::str(v->type());
    feed_hash_string(type_str);

    return visit(v->expr());
  }

  void on_binary(VertexAdaptor<meta_op_binary> v) override {
    VertexPtr key = v->lhs();
    VertexPtr value = v->rhs();

    visit(key);
    feed_hash_string(OpInfo::str(v->type()));
    visit(value);
  }

  void on_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    VertexPtr key = VertexUtil::get_actual_value(v->key());
    VertexPtr value = VertexUtil::get_actual_value(v->value());

    visit(key);
    feed_hash_string("=>");
    visit(value);
  }

  void on_array(VertexAdaptor<op_array> v) override {
    feed_hash(v->args().size());
    feed_hash(MAGIC1);

    for (auto it : *v) {
      visit(VertexUtil::get_actual_value(it));
    }

    feed_hash(MAGIC2);
  }

  void on_var(VertexAdaptor<op_var> v) override {
    return visit(VertexUtil::get_actual_value(v));
  }

  void on_non_const(VertexPtr v) override {
    std::string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
  }

  bool on_index_key(VertexPtr key) override {
    visit(key);
    return true;
  }

  void on_func_call(VertexAdaptor<op_func_call> v) override {
    if (v->func_id && v->func_id->is_constructor())
      for (auto son : v->args()) {
        visit(son);
      }
  }

  void on_define_val(VertexAdaptor<op_define_val> v) override {
    auto define_hasher = DefinePtr::Hash();
    feed_hash(static_cast<uint64_t>(define_hasher(v->define_id)));
    visit(v->value());
  }

  void on_alloc(VertexAdaptor<op_alloc> v) override {
    feed_hash_string(v->allocated_class_name);
  }

  uint64_t cur_hash = 0;
  static const uint64_t HASH_MULT = 56235515617499ULL;
  static const uint64_t MAGIC1 = 536536536536960ULL;
  static const uint64_t MAGIC2 = 288288288288069ULL;
};

struct ArrayHash final : CommonHash {
  static uint64_t calc_hash(VertexPtr v) {
    ArrayHash array_hash;
    array_hash.visit(VertexUtil::get_actual_value(v));
    return array_hash.cur_hash;
  }
};

struct ObjectHash final : CommonHash {
  static uint64_t calc_hash(VertexPtr v) {
    ObjectHash object_hash;
    object_hash.visit(v);
    return object_hash.cur_hash;
  }
};

struct VertexPtrFormatter final
  : ConstManipulations<std::string> {
  static std::string to_string(VertexPtr v) {
    static VertexPtrFormatter serializer;
    return serializer.visit(VertexUtil::get_actual_value(v));
  }

protected:
  std::string on_trivial(VertexPtr v) final {
    std::string s;

    if (v->has_get_string()) {
      s += v->get_string() + ':';
    }

    return s + OpInfo::str(v->type());
  }

  std::string on_conv(VertexAdaptor<meta_op_unary> v) final {
    return OpInfo::str(v->type()) + '(' + visit(v->expr()) + ')';
  }

  std::string on_unary(VertexAdaptor<meta_op_unary> v) final {
    return visit(v->expr()) + ':' + OpInfo::str(v->type());
  }

  std::string on_binary(VertexAdaptor<meta_op_binary> v) final {
    VertexPtr key = v->lhs();
    VertexPtr value = v->rhs();

    return '(' + visit(key) + OpInfo::str(v->type()) + visit(value) + ')';
  }

  std::string on_double_arrow(VertexAdaptor<op_double_arrow> v) final {
    VertexPtr key = VertexUtil::get_actual_value(v->key());
    VertexPtr value = VertexUtil::get_actual_value(v->value());

    return visit(key) + "=>" + visit(value);
  }

  std::string on_array(VertexAdaptor<op_array> v) final {
    std::string res;

    for (auto it : *v) {
      res += visit(VertexUtil::get_actual_value(it)) + ", ";
    }

    return res;
  }

  std::string on_var(VertexAdaptor<op_var> v) final {
    return v->get_string() + OpInfo::str(v->type());
  }

  std::string on_instance_prop(VertexAdaptor<op_instance_prop> v) final {
    return visit(v->instance()) + "->" + v->get_string();
  }

  std::string on_non_const(VertexPtr v) final {
    if (v->has_get_string()) {
      return v->get_string() + OpInfo::str(v->type());
    }

    std::string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
    return "ERROR: " + msg;
  }

  std::string on_index(VertexAdaptor<op_index> index) final {
    return visit(index->array()) + '[' + (index->has_key() ? visit(index->key()) : std::string{}) + ']';
  }
};

struct CanGenerateRawArray final
  : ConstManipulations<bool> {
public:
  static bool is_raw(VertexAdaptor<op_array> v) {
    static CanGenerateRawArray can_generate_raw;
    return can_generate_raw.visit(v);
  }

protected:
  bool on_trivial(VertexPtr v) override {
    return vk::any_of_equal(v->type(), op_int_const, op_float_const);
  }

  bool on_unary(VertexAdaptor<meta_op_unary> v) override {
    return visit(v->expr());
  }

  bool on_binary(VertexAdaptor<meta_op_binary> v) override {
    VertexPtr lhs = VertexUtil::get_actual_value(v->lhs());
    VertexPtr rhs = VertexUtil::get_actual_value(v->rhs());
    return visit(lhs) && visit(rhs);
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) override {
    auto ith_element = v->args()[ind];
    if (auto double_arrow = ith_element.try_as<op_double_arrow>()) {
      auto key = VertexUtil::get_actual_value(double_arrow->key()).try_as<op_int_const>();
      VertexPtr value = VertexUtil::get_actual_value(double_arrow->value());
      return key && key->str_val == std::to_string(ind) && visit(value);
    }
    return visit(VertexUtil::get_actual_value(ith_element));
  }

  bool on_array_finish(VertexAdaptor<op_array>) override {
    return true;
  }
};

