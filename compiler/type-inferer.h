#pragma once

#include "compiler/function-pass.h"
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
  if (v->type() == op_eq3 || v->type() == op_neq3) {
    VertexPtr b = v.as<meta_op_binary>()->rhs();
    if (b->type() == op_var || b->type() == op_index) {
      b = v.as<meta_op_binary>()->lhs();
    }

    if (b->type() == op_false) {
      return ifi_is_bool;
    }
    if (b->type() == op_string && b->get_string() == "") {
      return ifi_is_string;
    }
    if (b->type() == op_int_const && b->get_string() == "0") {
      return ifi_is_integer;
    }
    if (b->type() == op_float_const) {
      return ifi_is_float;
    }
    if (b->type() == op_array && b->empty()) {
      return ifi_is_array;
    }
  }
  if (v->type() == op_func_call) {
    const string &name = v->get_func_id()->name;
    if (name.size() > 2 && name[0] == 'i' && name[1] == 's') {
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
  std::vector<tinf::Node *> stacktrace;
  std::vector<tinf::Node *> node_path_;
  static const unsigned long max_cnt_nodes_in_path = 30;
  string desc;

  struct row {
    string col[3];

    row() = default;

    row(string const &s1, string const &s2, string const &s3) {
      col[0] = s1;
      col[1] = s2;
      col[2] = s3;
    }
  };

  row parse_description(string const &description);

public:
  tinf::Node *actual_, *expected_;

  RestrictionLess(tinf::Node *a, tinf::Node *b) :
    actual_(a),
    expected_(b) {

  }

  const char *get_description() {
    return dl_pstr("%s", desc.c_str());
  }

  string get_actual_error_message();
  string get_stacktrace_text();

protected:
  bool check_broken_restriction_impl() override;

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

    static bool is_same_vars(const tinf::Node *node, VertexPtr vertex);

    int get_priority(const tinf::Edge *edge) const {
      const tinf::Node *from_node = edge->from;
      const tinf::Node *to_node = edge->to;
      const TypeData *to_type = to_node->get_type();
      bool different_types = is_less(to_type, expected, edge->from_at);

      if (const tinf::ExprNode *expr_node = dynamic_cast<const tinf::ExprNode *>(to_node)) {
        VertexPtr expr_vertex = expr_node->get_expr();

        if (OpInfo::arity(expr_vertex->type()) == binary_opp) {
          VertexAdaptor<meta_op_binary> binary_vertex = expr_vertex.as<meta_op_binary>();
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
        stacktrace.push_back(to);

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

    stacktrace.clear();
    node_path_.clear();

    stacktrace.reserve(max_cnt_nodes_in_path);
    node_path_.reserve(max_cnt_nodes_in_path);

    find_call_trace_with_error_impl(cur_node, expected_->get_type());

    stacktrace.push_back(cur_node);
    std::reverse(stacktrace.begin(), stacktrace.end());
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
  void apply_type_rule_func(VertexAdaptor<op_type_rule_func> func_type_rule, VertexPtr expr);
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
  void recalc_conv_array(VertexAdaptor<meta_op_unary> conv);
  void recalc_min_max(VertexAdaptor<meta_op_builtin_func> func);
  void recalc_array(VertexAdaptor<op_array> array);
  void recalc_tuple(VertexAdaptor<op_tuple> tuple);
  void recalc_plus_minus(VertexAdaptor<meta_op_unary> expr);
  void recalc_inc_dec(VertexAdaptor<meta_op_unary> expr);
  void recalc_noerr(VertexAdaptor<op_noerr> expr);
  void recalc_arithm(VertexAdaptor<meta_op_binary> expr);
  void recalc_define_val(VertexAdaptor<op_define_val> define_val);
  void recalc_expr(VertexPtr expr);
public:
  ExprNodeRecalc(tinf::ExprNode *node, tinf::TypeInferer *inferer);
  tinf::ExprNode *get_node();
  void do_recalc();

  bool auto_edge_flag();
};


