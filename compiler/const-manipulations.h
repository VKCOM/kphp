//
// Created by eugene on 15.05.18.
//

#pragma once

#include "compiler/vertex.h"
#include "compiler/data_ptr.h"
#include "compiler/utils.h"
#include "compiler/operation.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"

#if __cplusplus < 201103L
#define override
#endif

template<typename T>
struct ConstManipulations {
public:
  virtual ~ConstManipulations() {}

protected:
  virtual T on_trivial(VertexPtr v) { return on_non_const(v); }
  virtual T on_conv(VertexAdaptor<meta_op_unary_op> v) { return on_non_const(v); }
  virtual T on_unary(VertexAdaptor<meta_op_unary_op> v) { return on_non_const(v); }
  virtual T on_binary(VertexAdaptor<meta_op_binary_op> v) { return on_non_const(v); }
  virtual T on_double_arrow(VertexAdaptor<op_double_arrow> v) { return on_non_const(v); }

  virtual bool on_array_double_arrow(VertexAdaptor<op_double_arrow>) { return false; }
  virtual bool on_array_value(VertexPtr array __attribute__((unused)), size_t ind __attribute__((unused))) { return false; }
  virtual T on_array_finish(VertexAdaptor<op_array> v) { return on_non_const(v); }

  virtual T on_func_name(VertexAdaptor<op_func_name> v) { return on_non_const(v); }
  virtual T on_var(VertexPtr v) { return on_non_const(v); }
  virtual T on_define_val(VertexAdaptor<op_define_val> v) { return on_non_const(v); }

  virtual T on_non_const(VertexPtr) { return T(); }

  virtual T on_array(VertexAdaptor<op_array> v) {
    bool has_key = false;
    bool no_key = false;

    VertexRange arr = v->args();
    for (size_t i = 0; i < arr.size(); ++i) {
      VertexPtr cur = arr[i];
      if (cur->type() == op_double_arrow) {
        has_key = true;

        if (!on_array_double_arrow(cur)) {
          return on_non_const(v);
        }
      } else {
        no_key = true;

        if (!on_array_value(v, i)) {
          return on_non_const(v);
        }
      }
    }

    if (has_key && no_key) {
      return on_non_const(v);
    } else {
      return on_array_finish(v.as<op_array>());
    }
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
        return on_conv(v.as<meta_op_unary_op>());

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
        return on_unary(v.as<meta_op_unary_op>());

      case op_add:
      case op_mul:
      case op_sub:
      case op_div:
      case op_mod:
      case op_and:
      case op_or:
      case op_xor:
      case op_shl:
      case op_shr:
        return on_binary(v.as<meta_op_binary_op>());

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
  : ConstManipulations<bool>
{
public:
  static bool is_const(VertexPtr v) {
    static CheckConst check_const;
    return check_const.visit(v);
  }

protected:
  bool on_trivial(VertexPtr) override {
    return true;
  }

  bool on_conv(VertexAdaptor<meta_op_unary_op> v) override {
    return visit(v->expr());
  }

  bool on_unary(VertexAdaptor<meta_op_unary_op> v) override {
    return visit(v->expr());
  }

  bool on_binary(VertexAdaptor<meta_op_binary_op> v) override {
    VertexPtr lhs = GenTree::get_actual_value(v->lhs());
    VertexPtr rhs = GenTree::get_actual_value(v->rhs());
    return visit(lhs) && visit(rhs);
  }

  bool on_array_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());
    return visit(key) && visit(value);
  }

  bool on_array_value(VertexPtr v, size_t ind) override {
    return visit(GenTree::get_actual_value(v->ith(ind)));
  }

  bool on_var(VertexPtr v) override {
    if (v->get_var_id().not_null() && (v->extra_type == op_ex_var_const || v->get_var_id()->is_constant)) {
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
  explicit CheckConstWithDefines(const std::map<std::string, VertexPtr> &define_vertex)
    : in_concat(0)
    , define_vertex(define_vertex)
  {}

  bool is_const(VertexPtr v) {
    return visit(v);
  }

protected:
  bool on_trivial(VertexPtr v) override {
    return in_concat == 0 || v->has_get_string();
  }

  bool on_func_name(VertexAdaptor<op_func_name> v) override {
    std::string name = resolve_define_name(v->str_val);
    auto it = define_vertex.find(name);
    if (it != define_vertex.end()) {
      return visit(it->second.as<op_define>()->value());
    }
    return false;
  }

  bool on_non_const(VertexPtr v) override {
    if (v->type() == op_concat || v->type() == op_string_build) {
      in_concat++;

      for(auto i : *v) {
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
  int in_concat;
  const std::map<std::string, VertexPtr> &define_vertex;
};


struct MakeConst
  : ConstManipulations<VertexPtr> {
public:
  explicit MakeConst(const std::map<std::string, VertexPtr> &define_vertex)
    : define_vertex(define_vertex)
  {}

  VertexPtr make_const(VertexPtr v) {
    return visit(v);
  }

protected:
  VertexPtr on_trivial(VertexPtr v) override {
    return v;
  }

  VertexPtr on_unary(VertexAdaptor<meta_op_unary_op> v) override {
    v->expr() = make_const(v->expr());
    return v;
  }

  VertexPtr on_binary(VertexAdaptor<meta_op_binary_op> v) override {
    v->lhs() = make_const(v->lhs());
    v->rhs() = make_const(v->rhs());
    return v;
  }

  bool on_array_value(VertexPtr v, size_t ind) override {
    v->ith(ind) = make_const(v->ith(ind));
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

  VertexPtr on_conv(VertexAdaptor<meta_op_unary_op> v) override {
    return make_const(v->expr());
  }

  VertexPtr on_func_name(VertexAdaptor<op_func_name> v) override {
    std::string name = resolve_define_name(v->str_val);
    auto iter = define_vertex.find(name);
    kphp_assert(iter != define_vertex.end());
    return iter->second.as<op_define>()->value();
  }

  VertexPtr on_non_const(VertexPtr v) override {
    if (v->type() == op_concat || v->type() == op_string_build) {
      CREATE_VERTEX(new_val, op_string);
      new_val->location = v->get_location();

      for(auto i : *v) {
        VertexPtr res = visit(i);
        kphp_error(res->has_get_string(), ("expected type convertible to string, but got: " + OpInfo::str(res->type())).c_str());
        new_val->str_val += res->get_string();
      }

      return new_val;
    }

    return VertexPtr();
  }

private:
  const std::map<std::string, VertexPtr> &define_vertex;
};

struct ArrayHash
  : ConstManipulations<long long>
{
  static long long calc_hash(VertexPtr v) {
    static ArrayHash array_hash;
    return array_hash.visit(GenTree::get_actual_value(v));
  }

protected:
  long long on_trivial(VertexPtr v) override {
    string s = OpInfo::str(v->type());

    if (v->has_get_string()) {
      s += v->get_string();
    }

    return string_hash(s.c_str(), static_cast<int>(s.size()));
  }

  long long on_conv(VertexAdaptor<meta_op_unary_op> v) override {
    return visit(v->expr());
  }

  long long on_unary(VertexAdaptor<meta_op_unary_op> v) override {
    string type_str = OpInfo::str(v->type());
    long long hash_of_type = string_hash(type_str.c_str(), static_cast<int>(type_str.size()));

    return hash_of_type * HASH_MULT + visit(v->expr());
  }

  long long on_define_val(VertexAdaptor<op_define_val> v) override {
    return visit(GenTree::get_actual_value(v));
  }

  long long on_binary(VertexAdaptor<meta_op_binary_op> v) override {
    VertexPtr key = v->lhs();
    VertexPtr value = v->rhs();

    return (visit(key) * HASH_MULT + OpInfo::str(v->type())[0]) * HASH_MULT + visit(value);
  }

  long long on_double_arrow(VertexAdaptor<op_double_arrow> v) override {
    VertexPtr key = GenTree::get_actual_value(v->key());
    VertexPtr value = GenTree::get_actual_value(v->value());

    return (visit(key) * HASH_MULT + '-') * HASH_MULT + visit(value);
  }

  long long on_array(VertexAdaptor<op_array> v) override {
    long long res_hash = v->args().size();
    res_hash = res_hash * HASH_MULT + MAGIC1;

    for(auto it : *v) {
      res_hash = res_hash * HASH_MULT + visit(GenTree::get_actual_value(it));
    }

    res_hash = res_hash * HASH_MULT + MAGIC2;

    return res_hash;
  }

  long long on_var(VertexPtr v) override {
    return on_unary(GenTree::get_actual_value(v));
  }

  long long on_non_const(VertexPtr v) override {
    string msg = "unsupported type for hashing: " + OpInfo::str(v->type());
    kphp_assert_msg(false, msg.c_str());
    return 0;
  }

private:
  static const long long HASH_MULT = 56235515617499LL;
  static const long long MAGIC1 = 536536536536960LL;
  static const long long MAGIC2 = 288288288288069LL;
};

struct VertexPtrFormatter
  : ConstManipulations<std::string>
{
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

  std::string on_conv(VertexAdaptor<meta_op_unary_op> v) override {
    return visit(v->expr());
  }

  std::string on_unary(VertexAdaptor<meta_op_unary_op> v) override {
    return visit(v->expr()) + ':' + OpInfo::str(v->type());
  }

  std::string on_define_val(VertexAdaptor<op_define_val> v) override {
    return visit(GenTree::get_actual_value(v));
  }

  std::string on_binary(VertexAdaptor<meta_op_binary_op> v) override {
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

    for(auto it : *v) {
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

