#pragma once

#include "compiler/common.h"
#include "compiler/data.h"
#include "compiler/function-pass.h"
#include "compiler/io.h"
#include "compiler/name-gen.h"
#include "compiler/stage.h"
#include "compiler/type-inferer-core.h"

tinf::Node *get_tinf_node(VertexPtr vertex);
tinf::Node *get_tinf_node(VarPtr var);
void init_functions_tinf_nodes(FunctionPtr function);
tinf::Node *get_tinf_node(FunctionPtr function, int id);

const TypeData *get_type(VertexPtr vertex, tinf::TypeInferer *inferer);
const TypeData *get_type(VarPtr var, tinf::TypeInferer *inferer);
const TypeData *get_type(FunctionPtr function, int id, tinf::TypeInferer *inferer);
const TypeData *fast_get_type(VertexPtr vertex);
const TypeData *fast_get_type(VarPtr var);
const TypeData *fast_get_type(FunctionPtr function, int id);
//TODO: remove extra CREATE_VERTEX?

enum is_func_id_t {
  ifi_error = -1,
  ifi_unset = 1,
  ifi_isset = 1 << 1,
  ifi_is_bool = 1 << 2,
  ifi_is_numeric = 1 << 3,
  ifi_is_scalar = 1 << 4,
  ifi_is_null = 1 << 5,
  ifi_is_integer = 1 << 6,
  ifi_is_long = 1 << 7,
  ifi_is_float = 1 << 8,
  ifi_is_double = 1 << 9,
  ifi_is_real = 1 << 10,
  ifi_is_string = 1 << 11,
  ifi_is_array = 1 << 12,
  ifi_is_object = 1 << 13
};

inline is_func_id_t get_ifi_id_(VertexPtr v) {
  if (v->type() == op_unset) {
    return ifi_unset;
  }
  if (v->type() == op_isset) {
    return ifi_isset;
  }
  if (v->type() == op_func_call) {
    const string &name = v->get_func_id()->name;
    if (name[0] == 'i' && name[1] == 's') {
      if (name == "is_bool") {
        return ifi_is_bool;
      }
      if (name == "is_numeric") {
        return ifi_is_numeric;
      }
      if (name == "is_scalar") {
        return ifi_is_scalar;
      }
      if (name == "is_null") {
        return ifi_is_null;
      }
      if (name == "is_bool") {
        return ifi_is_bool;
      }
      if (name == "is_integer") {
        return ifi_is_integer;
      }
      if (name == "is_long") {
        return ifi_is_long;
      }
      if (name == "is_float") {
        return ifi_is_float;
      }
      if (name == "is_double") {
        return ifi_is_double;
      }
      if (name == "is_real") {
        return ifi_is_real;
      }
      if (name == "is_string") {
        return ifi_is_string;
      }
      if (name == "is_array") {
        return ifi_is_array;
      }
      if (name == "is_object") {
        return ifi_is_object;
      }
    }
  }
  return ifi_error;
}

class Restriction : public tinf::RestrictionBase {
public:
  Location location;

  Restriction() :
    location(stage::get_location()) {}

  bool check_broken_restriction() override {
    bool err = check_broken_restriction_impl();
    if (err) {
      stage::set_location(location);
      if (is_broken_restriction_an_error()) {
        kphp_error (0, get_description());
      } else {
        kphp_warning (get_description());
      }
    }

    return err;
  }
};

class RestrictionLess : public Restriction {
private:
  std::vector<string> descriptions_;
  std::vector<tinf::Node *> node_path_;
  static const unsigned long max_cnt_nodes_in_path = 30;

public:
  tinf::Node *a_, *b_;
  string desc;

  RestrictionLess(tinf::Node *a, tinf::Node *b) :
    a_(a),
    b_(b) {

  }

  const char *get_description() {
    return dl_pstr("%s <%s <= %s>", desc.c_str(),
                   a_->get_description().c_str(), b_->get_description().c_str());
  }

protected:
  bool check_broken_restriction_impl() override {
    const TypeData *a_type = a_->get_type();
    const TypeData *b_type = b_->get_type();

    if (is_less(a_type, b_type)) {
      desc = "type inference error ";

      find_call_trace_with_error(a_);
      desc += "\n";

      for (const auto &description : descriptions_) {
        desc += description + " \n";
      }

      desc += "[" + type_out(a_type) + " <= " + type_out(b_type) + "]";

      return true;
    }

    return false;
  }

  bool is_broken_restriction_an_error() override {
    return true;
  }

private:
  bool is_parent_node(tinf::Node const *node) {
    return std::find(node_path_.begin(), node_path_.end(), node) != node_path_.end();
  }

  /* эвристика упорядочивания ребер для поиска наиболее подходящего, для вывода ошибки на экран, полученная эмпирическим путем */
  struct ComparatorByEdgePriorityRelativeToExpectedType {
  private:
    enum {
      e_default_priority = 4
    };

    const TypeData *expected;

    static bool is_same_vars(const tinf::Node *node, VertexPtr vertex) {
      if (const tinf::VarNode *var_node = dynamic_cast<const tinf::VarNode *>(node)) {
        if (vertex->type() == op_var) {
          if (!vertex->get_var_id() && !var_node->var_) {
            return true;
          }

          return vertex->get_var_id() && var_node->var_ &&
                 vertex->get_var_id()->name == var_node->var_->name;
        }
      }

      return false;
    }

    int get_priority(const tinf::Edge *edge) const {
      const tinf::Node *from_node = edge->from;
      const tinf::Node *to_node = edge->to;
      const TypeData *to_type = to_node->get_type();
      bool different_types = is_less(to_type, expected, edge->from_at);

      if (const tinf::ExprNode *expr_node = dynamic_cast<const tinf::ExprNode *>(to_node)) {
        VertexPtr expr_vertex = expr_node->get_expr();

        if (OpInfo::arity(expr_vertex->type()) == binary_opp) {
          VertexAdaptor<meta_op_binary_op> binary_vertex = expr_vertex.as<meta_op_binary_op>();
          VertexPtr lhs = binary_vertex->lhs();
          VertexPtr rhs = binary_vertex->rhs();

          if (is_same_vars(from_node, lhs)) {
            to_type = tinf::get_type(rhs);
          } else if (is_same_vars(from_node, rhs)) {
            to_type = tinf::get_type(lhs);
          }
        } else {
          to_type = tinf::get_type(expr_vertex);
        }
      } else {
        if (const tinf::VarNode *var_node = dynamic_cast<const tinf::VarNode *>(to_node)) {
          if (var_node->is_argument_of_function()) {
            return different_types ? 1 : (e_default_priority + 1);
          }
        } else if (dynamic_cast<const tinf::TypeNode *>(to_node)) {
          return different_types ? 2 : (e_default_priority + 2);
        }
      }

      if (to_type->ptype() == tp_array && to_type->lookup_at(Key::any_key()) == nullptr) {
        return different_types ? 3 : (e_default_priority + 3);
      }

      if (is_less(to_type, expected, edge->from_at)) {
        return 0;
      }

      return e_default_priority;
    }

  public:
    explicit ComparatorByEdgePriorityRelativeToExpectedType(const TypeData *expected) :
      expected(expected) {}

    bool is_priority_less_than_default(tinf::Node *cur_node) const {
      tinf::Edge edge_with_cur_node = {
        .from    = nullptr,
        .to      = cur_node,
        .from_at = nullptr
      };

      return get_priority(&edge_with_cur_node) < ComparatorByEdgePriorityRelativeToExpectedType::e_default_priority;
    }

    bool operator()(const tinf::Edge *lhs, const tinf::Edge *rhs) const {
      return get_priority(lhs) < get_priority(rhs);
    }
  };

  bool find_call_trace_with_error_impl(tinf::Node *cur_node, const TypeData *expected) {
    static int limit_calls = 1000000;
    limit_calls--;
    if (limit_calls <= 0) {
      return true;
    }

    ComparatorByEdgePriorityRelativeToExpectedType comparator(expected);

    auto &node_next = cur_node->get_next();

    if (node_next.empty()) {
      return comparator.is_priority_less_than_default(cur_node);
    }

    std::vector<tinf::Edge *> ordered_edges{node_next};
    std::sort(ordered_edges.begin(), ordered_edges.end(), comparator);

    for (auto e : ordered_edges) {
      tinf::Node *from = e->from;
      tinf::Node *to = e->to;

      assert(from == cur_node);

      if (to == cur_node) {
        string warn_message = "loop edge: from: " + from->get_description() + "; to: " + to->get_description() + "\n";
        kphp_warning(warn_message.c_str());
        continue;
      }

      if (is_parent_node(to)) {
        continue;
      }

      if (node_path_.size() == max_cnt_nodes_in_path) {
        return false;
      }

      node_path_.push_back(to);

      const TypeData *expected_type_in_level_of_multi_key = expected;
      if (e->from_at) {
        expected_type_in_level_of_multi_key = expected->const_read_at(*e->from_at);
      }

      if (find_call_trace_with_error_impl(to, expected_type_in_level_of_multi_key)) {
        node_path_.pop_back();
        descriptions_.push_back(to->get_description());

        return true;
      }

      node_path_.pop_back();
    }

    return false;
  }

  static bool is_less(const TypeData *given, const TypeData *expected, const MultiKey *from_at = nullptr) {
    std::unique_ptr<TypeData> type_of_to_node(expected->clone());

    if (from_at) {
      type_of_to_node->set_lca_at(*from_at, given);
    } else {
      type_of_to_node->set_lca(given);
    }

    return type_out(type_of_to_node.get()) != type_out(expected);
  }

  void find_call_trace_with_error(tinf::Node *cur_node) {
    assert(cur_node != nullptr);

    descriptions_.clear();
    node_path_.clear();

    descriptions_.reserve(max_cnt_nodes_in_path);
    node_path_.reserve(max_cnt_nodes_in_path);

    find_call_trace_with_error_impl(cur_node, b_->get_type());

    descriptions_.push_back(cur_node->get_description());
    std::reverse(descriptions_.begin(), descriptions_.end());
  }
};

class RestrictionIsset : public Restriction {
public:
  tinf::Node *a_;
  string desc;

  explicit RestrictionIsset(tinf::Node *a);
  const char *get_description();
  void find_dangerous_isset_warning(const vector<tinf::Node *> &bt, tinf::Node *node, const string &msg);
  bool isset_is_dangerous(int isset_flags, const TypeData *tp);
  bool find_dangerous_isset_dfs(int isset_flags, tinf::Node *node,
                                vector<tinf::Node *> *bt);

protected:
  bool check_broken_restriction_impl() override;
};

struct LValue {
  tinf::Node *value;
  const MultiKey *key;
  inline LValue();
  inline LValue(tinf::Node *value, const MultiKey *key);
  inline LValue(const LValue &from);
  inline LValue &operator=(const LValue &from);
};

struct RValue {
  const TypeData *type;
  tinf::Node *node;
  const MultiKey *key;
  bool drop_or_false;
  inline RValue();
  inline RValue(const TypeData *type, const MultiKey *key = nullptr);
  inline RValue(tinf::Node *node, const MultiKey *key = nullptr);
  inline RValue(const RValue &from);
  inline RValue &operator=(const RValue &from);
};

LValue::LValue() :
  value(nullptr),
  key(nullptr) {
}

LValue::LValue(tinf::Node *value, const MultiKey *key) :
  value(value),
  key(key) {
}

LValue::LValue(const LValue &from) :
  value(from.value),
  key(from.key) {
}

LValue &LValue::operator=(const LValue &from) {
  value = from.value;
  key = from.key;
  return *this;
}

RValue::RValue() :
  type(nullptr),
  node(nullptr),
  key(nullptr),
  drop_or_false(false) {
}

RValue::RValue(const TypeData *type, const MultiKey *key) :
  type(type),
  node(nullptr),
  key(key),
  drop_or_false(false) {
}

RValue::RValue(tinf::Node *node, const MultiKey *key) :
  type(nullptr),
  node(node),
  key(key),
  drop_or_false(false) {
}

RValue::RValue(const RValue &from) :
  type(from.type),
  node(from.node),
  key(from.key),
  drop_or_false(from.drop_or_false) {
}

RValue &RValue::operator=(const RValue &from) {
  type = from.type;
  node = from.node;
  key = from.key;
  drop_or_false = from.drop_or_false;
  return *this;
}

inline RValue drop_or_false(RValue rvalue) {
  rvalue.drop_or_false = true;
  return rvalue;
}

inline RValue as_rvalue(PrimitiveType primitive_type) {
  return RValue(TypeData::get_type(primitive_type));
}

inline RValue as_rvalue(VertexPtr v, const MultiKey *key = nullptr) {
  return RValue(get_tinf_node(v), key);
}

inline RValue as_rvalue(FunctionPtr function, int id) {
  return RValue(get_tinf_node(function, id));
}

inline RValue as_rvalue(VarPtr var) {
  return RValue(get_tinf_node(var));
}

inline RValue as_rvalue(const TypeData *type, const MultiKey *key = nullptr) {
  return RValue(type, key);
}

inline const RValue &as_rvalue(const RValue &rvalue) {
  return rvalue;
}

class CollectMainEdgesCallbackBase {
public:
  virtual ~CollectMainEdgesCallbackBase() {
  }

  virtual void require_node(const RValue &rvalue) = 0;
  virtual void create_set(const LValue &lvalue, const RValue &rvalue) = 0;
  virtual void create_less(const RValue &lhs, const RValue &rhs) = 0;
  virtual void create_isset_check(const RValue &rvalue) = 0;
};

class CollectMainEdgesPass : public FunctionPassBase {
private:
  CollectMainEdgesCallbackBase *callback_;

  RValue as_set_value(VertexPtr v) {
    if (v->type() == op_set) {
      return as_rvalue(v.as<op_set>()->rhs());
    }

    if (v->type() == op_prefix_inc ||
        v->type() == op_prefix_dec ||
        v->type() == op_postfix_dec ||
        v->type() == op_postfix_inc) {
      VertexAdaptor<meta_op_unary_op> unary = v;
      auto one = VertexAdaptor<op_int_const>::create();
      auto res = VertexAdaptor<op_add>::create(unary->expr(), one);
      set_location(one, stage::get_location());
      set_location(res, stage::get_location());
      return as_rvalue(res);
    }

    if (OpInfo::arity(v->type()) == binary_opp) {
      VertexAdaptor<meta_op_binary_op> binary = v;
      VertexPtr res = create_vertex(OpInfo::base_op(v->type()), binary->lhs(), binary->rhs());
      set_location(res, stage::get_location());
      return as_rvalue(res);
    }

    kphp_fail();
    return RValue();
  }

  LValue as_lvalue(VertexPtr v) {
    if (v->type() == op_instance_prop) {
      return as_lvalue(v->get_var_id());
    }

    int depth = 0;
    if (v->type() == op_foreach_param) {
      depth++;
      v = v.as<op_foreach_param>()->xs();
    }
    while (v->type() == op_index) {
      depth++;
      v = v.as<op_index>()->array();
    }

    tinf::Node *value = nullptr;
    if (v->type() == op_var) {
      value = get_tinf_node(v->get_var_id());
    } else if (v->type() == op_conv_array_l || v->type() == op_conv_int_l) {
      kphp_assert (depth == 0);
      return as_lvalue(v.as<meta_op_unary_op>()->expr());
    } else if (v->type() == op_array) {
      kphp_fail();
    } else if (v->type() == op_func_call) {
      value = get_tinf_node(v.as<op_func_call>()->get_func_id(), -1);
    } else if (v->type() == op_instance_prop) {       // при $a->arr[] = 1; когда не работает верхнее условие
      value = get_tinf_node(v->get_var_id());
    } else {
      kphp_error (0, dl_pstr("Bug in compiler: Trying to use [%s] as lvalue", OpInfo::str(v->type()).c_str()));
      kphp_fail();
    }

    kphp_assert (value != 0);
    return LValue(value, &MultiKey::any_key(depth));
  }

  // хотелось сделать, чтобы при записи $a[6][$idx] = ... делался честный multikey (int 6, any), а не AnyKey(2)
  // но у нас в реальном коде очень много числовых индексов на массивах, которые тогда хранятся отдельно,
  // и потом вывод типов отжирает слишком много памяти, т.к. хранит кучу индексов, а не просто any key
  // так что затея провалилась, и, как и раньше, при $a[...] делается AnyKey; поэтому же tuple'ы только read-only
  // но это только запись приводит в any key! с чтением всё в порядке, см. recalc_index() в выводе типов
  /*
  MultiKey *build_real_multikey (VertexPtr v) {
    MultiKey *key = new MultiKey();

    if (v->type() == op_foreach_param) {
      key->push_front(Key::any_key());
      v = v.as <op_foreach_param>()->xs();
    }
    while (v->type() == op_index) {
      if (v.as <op_index>()->has_key() && v.as <op_index>()->key()->type() == op_int_const) {
        key->push_front(Key::int_key(std::atoi(v.as <op_index>()->key()->get_string().c_str())));
      } else {
        key->push_front(Key::any_key());
      }
      v = v.as <op_index>()->array();
    }

    return key;
  }
  */

  LValue as_lvalue(FunctionPtr function, int id) {
    return LValue(get_tinf_node(function, id), &MultiKey::any_key(0));
  }

  LValue as_lvalue(VarPtr var) {
    return LValue(get_tinf_node(var), &MultiKey::any_key(0));
  }

  const LValue &as_lvalue(const LValue &lvalue) {
    return lvalue;
  }

  void do_create_set(const LValue &lvalue, const RValue &rvalue) {
    callback_->create_set(lvalue, rvalue);
  }

  void do_create_less(const RValue &lhs, const RValue &rhs) {
    callback_->create_less(lhs, rhs);
  }

  void do_require_node(const RValue &a) {
    callback_->require_node(a);
  }

  void do_create_isset_check(const RValue &a) {
    callback_->create_isset_check(a);
  }

  template<class A, class B>
  void create_set(const A &a, const B &b) {
    do_create_set(as_lvalue(a), as_rvalue(b));
  }

  template<class A, class B>
  void create_less(const A &a, const B &b) {
    do_create_less(as_rvalue(a), as_rvalue(b));
  }

  template<class A>
  void require_node(const A &a) {
    do_require_node(as_rvalue(a));
  }

  template<class A>
  void create_isset_check(const A &a) {
    do_create_isset_check(as_rvalue(a));
  }

  void add_type_rule(VertexPtr v) {
    if (v->type() == op_function ||
        v->type() == op_func_decl ||
        v->type() == op_extern_func) {
      return;
    }

    switch (v->type_rule->type()) {
      case op_common_type_rule:
        create_set(v, v->type_rule);
        break;
      case op_gt_type_rule:
        create_less(v->type_rule, v);
        break;
      case op_lt_type_rule:
        create_less(v, v->type_rule);
        break;
      case op_eq_type_rule:
        create_less(v->type_rule, v);
        create_less(v, v->type_rule);
        break;
      default:
        assert (0 && "unreachable");
    }
  }

  void add_type_help(VertexPtr v) {
    if (v->type() != op_var) {
      return;
    }
    create_set(v, v->type_help);
  }

  void on_func_call(VertexAdaptor<op_func_call> call) {
    FunctionPtr function = call->get_func_id();
    VertexRange function_params = get_function_params(function->root);

    //hardcoded hack
    if (function->name == "array_unshift" || function->name == "array_push") {
      VertexRange args = call->args();
      LValue val = as_lvalue(args[0]);

      MultiKey *key = new MultiKey(*val.key);
      key->push_back(Key::any_key());
      val.key = key;

      ++args.begin_;
      for (auto i : args) {
        create_set(val, i);
      }
    }


    if (!function->varg_flag || !function->is_extern) {
      int ii = 0;
      for (auto arg : call->args()) {
        if (!function->is_extern) {
          create_set(as_lvalue(function, ii), arg);
        }

        VertexAdaptor<meta_op_func_param> param = function_params[ii];
        if (param->var()->ref_flag) {
          create_set(arg, as_rvalue(function, ii));
        }

        ii++;
      }
    }
  }

  void on_constructor_call(VertexAdaptor<op_constructor_call> call) {
    FunctionPtr function = call->get_func_id();
    VertexRange function_params = get_function_params(function->root);

    int ii = 0;
    for (auto arg : call->args()) {

      create_set(as_lvalue(function, ii), arg);

      VertexAdaptor<meta_op_func_param> param = function_params[ii];
      if (param->var()->ref_flag) {
        create_set(arg, as_rvalue(function, ii));
      }

      ii++;
    }
  }

  void on_return(VertexAdaptor<op_return> v) {
    create_set(as_lvalue(stage::get_function(), -1), v->expr());
  }

  void on_foreach(VertexAdaptor<op_foreach> foreach_op) {
    VertexAdaptor<op_foreach_param> params = foreach_op->params();
    VertexPtr xs, x, key, temp_var;
    xs = params->xs();
    x = params->x();
    temp_var = params->temp_var();
    if (params->has_key()) {
      key = params->key();
    }
    if (x->ref_flag) {
      LValue xs_tinf = as_lvalue(xs);
      create_set(xs_tinf, tp_array);
      create_set(params, x->get_var_id());
    } else {
      create_set(temp_var->get_var_id(), xs);
    }
    create_set(x->get_var_id(), params);
    if (key) {
      create_set(key->get_var_id(), tp_var);
    }
  }

  void on_index(VertexAdaptor<op_index> index __attribute__ ((unused))) {
    // здесь было безусловное { create_set (index->array(), tp_array) }, но это не верно:
    // при наличии $s[idx] переменная $s это не обязательно массив: это может быть строка или кортеж
    // если rvalue $s[idx], то вызовется const_read_at()
    // если lvalue $s[idx], то вызовется make_structured()
  }

  void on_list(VertexAdaptor<op_list> list) {
    int i = 0;
    for (auto cur : list->list()) {
      if (cur->type() != op_lvalue_null) {
        // делаем $cur = $list_array[$i]; хотелось бы array[i] выразить через rvalue multikey int_key, но
        // при составлении edges (from_node[from_at] = to_node) этот key теряется, поэтому через op_index
        auto ith_index = VertexAdaptor<op_int_const>::create();
        ith_index->set_string(int_to_str(i));
        auto new_v = VertexAdaptor<op_index>::create(list->array(), ith_index);
        set_location(new_v, stage::get_location());
        create_set(cur, new_v);
      }
      i++;
    }
  }

  void on_fork(VertexAdaptor<op_fork> fork) {
    // fork(f()) — f() не должна возвращать instance/tuple, т.е. то, что не кастится к var (см. wait_result())
    create_less(fork->func_call(), tp_var);
  }

  void on_throw(VertexAdaptor<op_throw> throw_op) {
    create_less(tp_Exception, throw_op->expr());
    create_less(throw_op->expr(), tp_Exception);
  }

  void on_set_op(VertexPtr v) {
    VertexPtr lval;
    if (OpInfo::arity(v->type()) == binary_opp) {
      lval = v.as<meta_op_binary_op>()->lhs();
    } else if (OpInfo::arity(v->type()) == unary_opp) {
      lval = v.as<meta_op_unary_op>()->expr();
    } else {
      kphp_fail();
    }
    create_set(lval, as_set_value(v));
  }


  void ifi_fix(VertexPtr v) {
    is_func_id_t ifi_tp = get_ifi_id_(v);
    if (ifi_tp == ifi_error) {
      return;
    }
    for (auto cur : *v) {
      if (cur->type() == op_var && cur->get_var_id()->type() == VarData::var_const_t) {
        continue;
      }
      if (cur->type() == op_var || (ifi_tp > ifi_isset && cur->type() == op_index)) {
        if (ifi_tp != ifi_is_object) {      // чтобы is_object() не обобщал класс до var
          create_set(cur, tp_var);
        }
      }

      if (cur->type() == op_var && ifi_tp != ifi_unset) {
        tinf::Node *node = get_tinf_node(cur);
        if (node->isset_flags == 0) {
          create_isset_check(node);
        }
        node->isset_flags |= ifi_tp;
      }
    }
  }

  void on_function(FunctionPtr function) {
    VertexRange params = get_function_params(function->root);
    int params_n = (int)params.size();

    for (int i = -1; i < params_n; i++) {
      require_node(as_rvalue(function, i));
    }

    if (function->is_extern) {
      PrimitiveType ret_type = function->root->type_help;
      if (ret_type == tp_Unknown) {
        ret_type = tp_var;
      }
      create_set(as_lvalue(function, -1), ret_type);

      for (int i = 0; i < params_n; i++) {
        PrimitiveType ptype = params[i]->type_help;
        if (ptype == tp_Unknown) {
          ptype = tp_Any;
        }
        //FIXME: type is const...
        create_set(as_lvalue(function, i), TypeData::get_type(ptype, tp_Any));
      }
    } else {
      for (int i = 0; i < params_n; i++) {
        //FIXME?.. just use pointer to node?..
        create_set(as_lvalue(function, i), function->param_ids[i]);
        create_set(function->param_ids[i], as_rvalue(function, i));
        if (function->is_callback) {
          create_set(as_lvalue(function, i), tp_var);
        }
      }
      if (function->used_in_source) {
        for (int i = -1; i < params_n; i++) {
          PrimitiveType x = tp_Unknown;

          if (i == -1) {
            if (function->root->type_rule) {
              kphp_error_act (function->root->type_rule->type() == op_common_type_rule, "...", continue);
              VertexAdaptor<op_common_type_rule> common_type_rule = function->root->type_rule;
              VertexPtr rule = common_type_rule->expr();
              kphp_error_act (rule->type() == op_type_rule, "...", continue);
              x = rule->type_help;
            }
          } else {
            x = params[i]->type_help;
            if (function->varg_flag && x == tp_Unknown) {
              x = tp_array;
            }
          }

          if (x == tp_Unknown) {
            x = tp_var;
          }
          const TypeData *tp = TypeData::get_type(x, tp_var);
          create_less(as_rvalue(function, i), tp);
          create_set(as_lvalue(function, i), tp);
        }

      }

      if (function->root->void_flag) {
        const TypeData *tp = TypeData::get_type(tp_var);
        create_less(as_rvalue(function, -1), tp);
        create_set(as_lvalue(function, -1), tp);
      }
    }
  }

  template<class CollectionT>
  void call_on_var(const CollectionT &collection) {
    for (const auto &el: collection) {
      on_var(el);
    }
  }

public:
  explicit CollectMainEdgesPass(CollectMainEdgesCallbackBase *callback) :
    callback_(callback) {
  }

  string get_description() {
    return "Collect main tinf edges";
  }

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }
    on_function(function);
    if (function->type() == FunctionData::func_extern) {
      return false;
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {
    if (v->type_rule) {
      add_type_rule(v);
    }
    //FIXME: type_rule should be used indead of type_help
    if (v->type_help != tp_Unknown) {
      add_type_help(v);
    }

    switch (v->type()) {
      //FIXME: varg_flag, is_callback
      case op_func_call:
        on_func_call(v);
        break;
      case op_constructor_call:
        on_constructor_call(v);
        break;
      case op_return:
        on_return(v);
        break;
      case op_foreach:
        on_foreach(v);
        break;
      case op_index:
        on_index(v);
        break;
      case op_list:
        on_list(v);
        break;
      case op_throw:
        on_throw(v);
        break;
      case op_fork:
        on_fork(v);
        break;
      default:
        break;
    }
    if (OpInfo::rl(v->type()) == rl_set ||
        v->type() == op_prefix_inc ||
        v->type() == op_postfix_inc ||
        v->type() == op_prefix_dec ||
        v->type() == op_postfix_dec) {
      on_set_op(v);
    }

    ifi_fix(v);

    return v;
  }

  void on_var(VarPtr var) {
    if (var->tinf_flag) {
      return;
    }
    //FIXME: use holder_function!!!
    if (!__sync_bool_compare_and_swap(&var->tinf_flag, false, true)) {
      return;
    }
    require_node(var);
    if (var->init_val) {
      create_set(var, var->init_val);
    }
  }

  void on_define(DefinePtr def) {
    require_node(def->val);
  }

  void on_finish() {
    call_on_var(current_function->local_var_ids);
    call_on_var(current_function->global_var_ids);
    call_on_var(current_function->static_var_ids);
    call_on_var(current_function->header_global_var_ids);
    call_on_var(current_function->const_var_ids);
    call_on_var(current_function->header_const_var_ids);
    call_on_var(current_function->param_ids);

    for (auto def : current_function->define_ids) {
      on_define(def);
    }
  }
};

class NodeRecalc {
protected:
  TypeData *new_type_;
  tinf::Node *node_;
  tinf::TypeInferer *inferer_;
  vector<TypeData *> types_stack;
public:
  const TypeData *new_type();
  void add_dependency_impl(tinf::Node *from, tinf::Node *to);
  void add_dependency(const RValue &rvalue);
  void set_lca_at(const MultiKey *key, const RValue &rvalue);
  void set_lca_at(const MultiKey *key, VertexPtr expr);
  void set_lca_at(const MultiKey *key, PrimitiveType ptype);
  void set_lca(const RValue &rvalue);
  void set_lca(PrimitiveType ptype);
  void set_lca(FunctionPtr function, int id);
  void set_lca(VertexPtr vertex, const MultiKey *key = nullptr);
  void set_lca(const TypeData *type, const MultiKey *key = nullptr);
  void set_lca(VarPtr var);
  void set_lca(ClassPtr klass);
  NodeRecalc(tinf::Node *node, tinf::TypeInferer *inferer);

  virtual ~NodeRecalc() {}

  void on_changed();
  void push_type();
  TypeData *pop_type();
  virtual bool auto_edge_flag();

  virtual void do_recalc() = 0;

  void run();
};

class VarNodeRecalc : public NodeRecalc {
public:
  VarNodeRecalc(tinf::VarNode *node, tinf::TypeInferer *inferer);
  void do_recalc();
};

class ExprNodeRecalc : public NodeRecalc {
private:
  template<PrimitiveType tp>
  void recalc_ptype();

  void recalc_require(VertexAdaptor<op_require> require);
  void recalc_ternary(VertexAdaptor<op_ternary> ternary);
  void apply_type_rule_func(VertexAdaptor<op_type_rule_func> func, VertexPtr expr);
  void apply_type_rule_type(VertexAdaptor<op_type_rule> rule, VertexPtr expr);
  void apply_arg_ref(VertexAdaptor<op_arg_ref> arg, VertexPtr expr);
  void apply_index(VertexAdaptor<op_index> index, VertexPtr expr);
  void apply_type_rule(VertexPtr rule, VertexPtr expr);
  void recalc_func_call(VertexAdaptor<op_func_call> call);
  void recalc_constructor_call(VertexAdaptor<op_constructor_call> call);
  void recalc_var(VertexAdaptor<op_var> var);
  void recalc_push_back_return(VertexAdaptor<op_push_back_return> pb);
  void recalc_index(VertexAdaptor<op_index> index);
  void recalc_instance_prop(VertexAdaptor<op_instance_prop> index);
  void recalc_set(VertexAdaptor<op_set> set);
  void recalc_double_arrow(VertexAdaptor<op_double_arrow> arrow);
  void recalc_foreach_param(VertexAdaptor<op_foreach_param> param);
  void recalc_conv_array(VertexAdaptor<meta_op_unary_op> conv);
  void recalc_min_max(VertexAdaptor<meta_op_builtin_func> func);
  void recalc_array(VertexAdaptor<op_array> array);
  void recalc_tuple(VertexAdaptor<op_tuple> tuple);
  void recalc_plus_minus(VertexAdaptor<meta_op_unary_op> expr);
  void recalc_inc_dec(VertexAdaptor<meta_op_unary_op> expr);
  void recalc_noerr(VertexAdaptor<op_noerr> expr);
  void recalc_arithm(VertexAdaptor<meta_op_binary_op> expr);
  void recalc_define_val(VertexAdaptor<op_define_val> define_val);
  void recalc_expr(VertexPtr expr);
public:
  ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer);
  tinf::ExprNode *get_node();
  void do_recalc();

  bool auto_edge_flag();
};


