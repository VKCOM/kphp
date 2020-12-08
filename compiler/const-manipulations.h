// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/php-functions.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/define-data.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/operation.h"
#include "compiler/pipes/check-access-modifiers.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

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

  virtual bool on_array_value(VertexAdaptor<op_array> array __attribute__((unused)), size_t ind __attribute__((unused))) { return false; }

  virtual T on_array_finish(VertexAdaptor<op_array> v) { return on_non_const(v); }

  virtual T on_func_name(VertexAdaptor<op_func_name> v) { return on_non_const(v); }

  virtual T on_var(VertexAdaptor<op_var> v) { return on_non_const(v); }

  virtual T on_instance_prop(VertexAdaptor<op_instance_prop> v) { return on_non_const(v); }

  virtual T on_non_const(VertexPtr) { return T(); }

  virtual T on_array(VertexAdaptor<op_array> v) {
    VertexRange arr = v->args();
    for (size_t i = 0; i < arr.size(); ++i) {
      if (!on_array_value(v, i)) {
        return on_non_const(v);
      }
    }
    return on_array_finish(v);
  }

protected:

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

      case op_var:
        return on_var(v.as<op_var>());

      case op_instance_prop:
        return on_instance_prop(v.as<op_instance_prop>());

      case op_func_name:
        return on_func_name(v.as<op_func_name>());

      case op_double_arrow:
        return on_double_arrow(v.as<op_double_arrow>());

      default:
        return on_non_const(v);
    }
  }

};

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
    VertexPtr lhs = GenTree::get_actual_value(v->lhs());
    VertexPtr rhs = GenTree::get_actual_value(v->rhs());
    return visit(lhs) && visit(rhs);
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) override {
    auto ith_element = v->args()[ind];
    if (auto double_arrow = ith_element.try_as<op_double_arrow>()) {
      VertexPtr key = GenTree::get_actual_value(double_arrow->key());
      VertexPtr value = GenTree::get_actual_value(double_arrow->value());
      return visit(key) && visit(value);
    }
    return visit(GenTree::get_actual_value(ith_element));
  }

  bool on_var(VertexAdaptor<op_var> v) override {
    if (v->var_id && (v->extra_type == op_ex_var_const || v->var_id->is_constant())) {
      return visit(v->var_id->init_val);
    }

    return false;
  }

  bool on_array_finish(VertexAdaptor<op_array>) override {
    return true;
  }
};

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
    return G->get_define(name)->val;
  }

  VertexPtr on_non_const(VertexPtr v) final {
    if (v->type() == op_concat || v->type() == op_string_build) {
      auto new_val = VertexAdaptor<op_string>::create();
      new_val->location = v->get_location();

      for (auto i : *v) {
        VertexPtr res = visit(i);
        kphp_error(res->has_get_string(), ("expected type convertible to string, but got: " + OpInfo::str(res->type())).c_str());
        new_val->str_val += res->get_string();
      }

      return new_val;
    }

    return {};
  }
};

struct ArrayHash final
  : ConstManipulations<void> {
  static uint64_t calc_hash(VertexPtr v) {
    ArrayHash array_hash;
    array_hash.visit(GenTree::get_actual_value(v));
    return array_hash.cur_hash;
  }

  void feed_hash(uint64_t val) {
    cur_hash = cur_hash * HASH_MULT + val;
  }

  void feed_hash_string(const string &s) {
    feed_hash(string_hash(s.c_str(), s.size()));
  }

protected:
  void on_trivial(VertexPtr v) final {
    string s = OpInfo::str(v->type());

    if (v->has_get_string()) {
      s += v->get_string();
    }

    feed_hash_string(s);
  }

  void on_conv(VertexAdaptor<meta_op_unary> v) final {
    feed_hash_string(OpInfo::str(v->type()));
    return visit(v->expr());
  }

  void on_unary(VertexAdaptor<meta_op_unary> v) final {
    string type_str = OpInfo::str(v->type());
    feed_hash_string(type_str);

    return visit(v->expr());
  }

  void on_binary(VertexAdaptor<meta_op_binary> v) final {
    VertexPtr key = v->lhs();
    VertexPtr value = v->rhs();

    visit(key);
    feed_hash_string(OpInfo::str(v->type()));
    visit(value);
  }

  void on_double_arrow(VertexAdaptor<op_double_arrow> v) final {
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());

    visit(key);
    feed_hash_string("=>");
    visit(value);
  }

  void on_array(VertexAdaptor<op_array> v) final {
    feed_hash(v->args().size());
    feed_hash(MAGIC1);

    for (auto it : *v) {
      visit(GenTree::get_actual_value(it));
    }

    feed_hash(MAGIC2);
  }

  void on_var(VertexAdaptor<op_var> v) final {
    return visit(GenTree::get_actual_value(v));
  }

  void on_non_const(VertexPtr v) final {
    string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
  }

private:
  uint64_t cur_hash = 0;
  static const uint64_t HASH_MULT = 56235515617499ULL;
  static const uint64_t MAGIC1 = 536536536536960ULL;
  static const uint64_t MAGIC2 = 288288288288069ULL;
};

struct VertexPtrFormatter final
  : ConstManipulations<std::string> {
  static std::string to_string(VertexPtr v) {
    static VertexPtrFormatter serializer;
    return serializer.visit(GenTree::get_actual_value(v));
  }

protected:
  std::string on_trivial(VertexPtr v) final {
    string s;

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
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());

    return visit(key) + "=>" + visit(value);
  }

  std::string on_array(VertexAdaptor<op_array> v) final {
    std::string res;

    for (auto it : *v) {
      res += visit(GenTree::get_actual_value(it)) + ", ";
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

    string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
    return "ERROR: " + msg;
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
    VertexPtr lhs = GenTree::get_actual_value(v->lhs());
    VertexPtr rhs = GenTree::get_actual_value(v->rhs());
    return visit(lhs) && visit(rhs);
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) override {
    auto ith_element = v->args()[ind];
    if (auto double_arrow = ith_element.try_as<op_double_arrow>()) {
      auto key = GenTree::get_actual_value(double_arrow->key()).try_as<op_int_const>();
      VertexPtr value = GenTree::get_actual_value(double_arrow->value());
      return key && key->str_val == std::to_string(ind) && visit(value);
    }
    return visit(GenTree::get_actual_value(ith_element));
  }

  bool on_array_finish(VertexAdaptor<op_array>) override {
    return true;
  }
};

