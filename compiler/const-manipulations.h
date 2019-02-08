//
// Created by eugene on 15.05.18.
//

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/define-data.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/operation.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

#if __cplusplus < 201103L
#define override
#endif

template<typename T>
struct ConstManipulations {
public:
  virtual ~ConstManipulations() {}

protected:
  virtual T on_trivial(VertexPtr v) { return on_non_const(v); }

  virtual T on_conv(VertexAdaptor<meta_op_unary> v) { return on_non_const(v); }

  virtual T on_unary(VertexAdaptor<meta_op_unary> v) { return on_non_const(v); }

  virtual T on_binary(VertexAdaptor<meta_op_binary> v) { return on_non_const(v); }

  virtual T on_double_arrow(VertexAdaptor<op_double_arrow> v) { return on_non_const(v); }

  virtual bool on_array_double_arrow(VertexAdaptor<op_double_arrow>) { return false; }

  virtual bool on_array_value(VertexAdaptor<op_array> array __attribute__((unused)), size_t ind __attribute__((unused))) { return false; }

  virtual T on_array_finish(VertexAdaptor<op_array> v) { return on_non_const(v); }

  virtual T on_func_name(VertexAdaptor<op_func_name> v) { return on_non_const(v); }

  virtual T on_var(VertexPtr v) { return on_non_const(v); }

  virtual T on_define_val(VertexAdaptor<op_define_val> v) { return on_non_const(v); }

  virtual T on_non_const(VertexPtr) { return T(); }

  virtual T on_array(VertexAdaptor<op_array> v) {
    VertexRange arr = v->args();
    for (size_t i = 0; i < arr.size(); ++i) {
      VertexPtr cur = arr[i];
      if (cur->type() == op_double_arrow) {
        if (!on_array_double_arrow(cur.as<op_double_arrow>())) {
          return on_non_const(v);
        }
      } else if (!on_array_value(v, i)) {
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
      case op_conv_array:
      case op_conv_array_l:
      case op_conv_object:
      case op_conv_bool:
      case op_conv_var:
      case op_conv_uint:
      case op_conv_long:
      case op_conv_ulong:
      case op_conv_regexp:
        return on_conv(v.as<meta_op_unary>());

      case op_int_const:
      case op_uint_const:
      case op_long_const:
      case op_ulong_const:
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
        return on_var(v);

      case op_func_name:
        return on_func_name(v.as<op_func_name>());

      case op_define_val:
        return on_define_val(v.as<op_define_val>());

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

  bool on_array_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());
    return visit(key) && visit(value);
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) override {
    return visit(GenTree::get_actual_value(v->args()[ind]));
  }

  bool on_var(VertexPtr v) override {
    if (v->get_var_id() && (v->extra_type == op_ex_var_const || v->get_var_id()->is_constant())) {
      return visit(v->get_var_id()->init_val);
    }

    return false;
  }

  bool on_array_finish(VertexAdaptor<op_array>) override {
    return true;
  }
};

struct CheckConstWithDefines
  : CheckConst {
public:
  bool is_const(VertexPtr v) {
    return visit(v);
  }

protected:
  bool on_trivial(VertexPtr v) override {
    return in_concat == 0 || v->has_get_string();
  }

  bool on_func_name(VertexAdaptor<op_func_name> v) override {
    std::string name = resolve_define_name(v->str_val);
    DefinePtr define = G->get_define(name);
    if (define) {
      return visit(define->val);
    }
    return false;
  }

  bool on_non_const(VertexPtr v) override {
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


struct MakeConst
  : ConstManipulations<VertexPtr> {
public:
  VertexPtr make_const(VertexPtr v) {
    return visit(v);
  }

protected:
  VertexPtr on_trivial(VertexPtr v) override {
    return v;
  }

  VertexPtr on_unary(VertexAdaptor<meta_op_unary> v) override {
    v->expr() = make_const(v->expr());
    return v;
  }

  VertexPtr on_binary(VertexAdaptor<meta_op_binary> v) override {
    v->lhs() = make_const(v->lhs());
    v->rhs() = make_const(v->rhs());
    return v;
  }

  bool on_array_value(VertexAdaptor<op_array> v, size_t ind) override {
    v->args()[ind] = make_const(v->args()[ind]);
    return true;
  }

  bool on_array_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    v->key() = make_const(v->key());
    v->value() = make_const(v->value());
    return true;
  }

  VertexPtr on_array_finish(VertexAdaptor<op_array> v) override {
    return v;
  }

  VertexPtr on_conv(VertexAdaptor<meta_op_unary> v) override {
    return make_const(v->expr());
  }

  VertexPtr on_func_name(VertexAdaptor<op_func_name> v) override {
    std::string name = resolve_define_name(v->str_val);
    return G->get_define(name)->val;
  }

  VertexPtr on_non_const(VertexPtr v) override {
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

    return VertexPtr();
  }
};

struct ArrayHash
  : ConstManipulations<void> {
  static long long calc_hash(VertexPtr v) {
    ArrayHash array_hash;
    array_hash.visit(GenTree::get_actual_value(v));
    return array_hash.cur_hash;
  }

  void feed_hash(long long val) {
    cur_hash = cur_hash * HASH_MULT + val;
  }

  void feed_hash_string(const string &s) {
    feed_hash(string_hash(s.c_str(), static_cast<int>(s.size())));
  }

protected:
  void on_trivial(VertexPtr v) override {
    string s = OpInfo::str(v->type());

    if (v->has_get_string()) {
      s += v->get_string();
    }

    feed_hash_string(s);
  }

  void on_conv(VertexAdaptor<meta_op_unary> v) override {
    return visit(v->expr());
  }

  void on_unary(VertexAdaptor<meta_op_unary> v) override {
    string type_str = OpInfo::str(v->type());
    feed_hash_string(type_str);

    return visit(v->expr());
  }

  void on_define_val(VertexAdaptor<op_define_val> v) override {
    return visit(GenTree::get_actual_value(v));
  }

  void on_binary(VertexAdaptor<meta_op_binary> v) override {
    VertexPtr key = v->lhs();
    VertexPtr value = v->rhs();

    visit(key);
    feed_hash_string(OpInfo::str(v->type()));
    visit(value);
  }

  void on_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());

    visit(key);
    feed_hash_string("=>");
    visit(value);
  }

  void on_array(VertexAdaptor<op_array> v) override {
    feed_hash(v->args().size());
    feed_hash(MAGIC1);

    for (auto it : *v) {
      visit(GenTree::get_actual_value(it));
    }

    feed_hash(MAGIC2);
  }

  void on_var(VertexPtr v) override {
    return visit(GenTree::get_actual_value(v));
  }

  void on_non_const(VertexPtr v) override {
    string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
  }

private:
  long long cur_hash = 0;
  static const long long HASH_MULT = 56235515617499LL;
  static const long long MAGIC1 = 536536536536960LL;
  static const long long MAGIC2 = 288288288288069LL;
};

struct VertexPtrFormatter
  : ConstManipulations<std::string> {
  static std::string to_string(VertexPtr v) {
    static VertexPtrFormatter serializer;
    return serializer.visit(GenTree::get_actual_value(v));
  }

protected:
  std::string on_trivial(VertexPtr v) override {
    string s;

    if (v->has_get_string()) {
      s += v->get_string() + ':';
    }

    return s + OpInfo::str(v->type());
  }

  std::string on_conv(VertexAdaptor<meta_op_unary> v) override {
    return visit(v->expr());
  }

  std::string on_unary(VertexAdaptor<meta_op_unary> v) override {
    return visit(v->expr()) + ':' + OpInfo::str(v->type());
  }

  std::string on_define_val(VertexAdaptor<op_define_val> v) override {
    return visit(GenTree::get_actual_value(v));
  }

  std::string on_binary(VertexAdaptor<meta_op_binary> v) override {
    VertexPtr key = v->lhs();
    VertexPtr value = v->rhs();

    return '(' + visit(key) + OpInfo::str(v->type()) + visit(value) + ')';
  }

  std::string on_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());

    return visit(key) + "=>" + visit(value);
  }

  std::string on_array(VertexAdaptor<op_array> v) override {
    std::string res;

    for (auto it : *v) {
      res += visit(GenTree::get_actual_value(it)) + ", ";
    }

    return res;
  }

  std::string on_var(VertexPtr v) override {
    return v->get_string() + OpInfo::str(v->type());
  }

  std::string on_non_const(VertexPtr v) override {
    string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
    return "ERROR: " + msg;
  }
};

