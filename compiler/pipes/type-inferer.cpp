#include "compiler/pipes/type-inferer.h"

#include "compiler/scheduler/scheduler-base.h"
#include "compiler/scheduler/task.h"
#include "compiler/type-inferer-core.h"
#include "compiler/type-inferer.h"

class CollectMainEdgesCallback {
private:
  tinf::TypeInferer *inferer_;

public:
  CollectMainEdgesCallback(tinf::TypeInferer *inferer) :
    inferer_(inferer) {
  }

  tinf::Node *node_from_rvalue(const RValue &rvalue) {
    if (rvalue.node == nullptr) {
      kphp_assert (rvalue.type != nullptr);
      return new tinf::TypeNode(rvalue.type);
    }

    return rvalue.node;
  }

  virtual void require_node(const RValue &rvalue) {
    if (rvalue.node != nullptr) {
      inferer_->add_node(rvalue.node);
    }
  }

  virtual void create_set(const LValue &lvalue, const RValue &rvalue) {
    tinf::Edge *edge = new tinf::Edge();
    edge->from = lvalue.value;
    edge->from_at = lvalue.key;
    edge->to = node_from_rvalue(rvalue);
    inferer_->add_edge(edge);
    inferer_->add_node(edge->from);
  }

  virtual void create_less(const RValue &lhs, const RValue &rhs) {
    tinf::Node *a = node_from_rvalue(lhs);
    tinf::Node *b = node_from_rvalue(rhs);
    inferer_->add_node(a);
    inferer_->add_node(b);
    inferer_->add_restriction(new RestrictionLess(a, b));
  }

  virtual void create_isset_check(const RValue &rvalue) {
    tinf::Node *a = node_from_rvalue(rvalue);
    inferer_->add_node(a);
    inferer_->add_restriction(new RestrictionIsset(a));
  }
};


class CollectMainEdgesPass : public FunctionPassBase {
private:
  CollectMainEdgesCallback *callback_;

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

  void on_func_param_callback(VertexAdaptor<op_func_call> call, int id) {
    const FunctionPtr call_function = call->get_func_id();
    const VertexPtr ith_argument_of_call = call->args()[id];
    VertexAdaptor<op_func_param_callback> callback_param = call_function->get_params()[id];

    FunctionPtr callback_function;
    if (ith_argument_of_call->type() == op_func_ptr) {
      callback_function = ith_argument_of_call->get_func_id();
    }

    kphp_assert(callback_function);

    // not now
    // // restriction on return type
    // bool is_any = false;
    // if (auto rule = callback_param->type_rule.try_as<op_common_type_rule>()) {
    //   if (auto son = rule->expr().try_as<op_type_rule>()) {
    //     is_any = son->type_help == tp_Any;
    //   }
    // }

    // if (!is_any) {
    //   auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
    //   fake_func_call->type_rule = callback_param->type_rule;
    //   fake_func_call->set_func_id(call_function);
    //   create_less(as_rvalue(callback_function, -1), fake_func_call);
    // }

    VertexRange callback_args = get_function_params(callback_param);
    for (int i = 0; i < callback_args.size(); ++i) {
      VertexAdaptor<op_func_param> callback_ith_arg = callback_args[i];

      if (VertexPtr type_rule = callback_ith_arg->type_rule) {
         auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
         fake_func_call->type_rule = type_rule;
         fake_func_call->set_func_id(call_function);

         int id_of_callbak_argument = callback_function->is_lambda() ? i + 1 : i;
         create_set(as_lvalue(callback_function, id_of_callbak_argument), fake_func_call);
      }
    }
  }

  void on_func_call(VertexAdaptor<op_func_call> call) {
    FunctionPtr function = call->get_func_id();
    VertexRange function_params = function->get_params();

    //hardcoded hack
    if (function->name == "array_unshift" || function->name == "array_push") {
      VertexRange args = call->args();
      LValue val = as_lvalue(args[0]);

      MultiKey *key = new MultiKey(*val.key);
      key->push_back(Key::any_key());
      val.key = key;

      for (auto i : VertexRange(args.begin() + 1, args.end())) {
        create_set(val, i);
      }
    }


    if (!(function->varg_flag && function->is_extern)) {
      for (int i = 0; i < call->args().size(); ++i) {
        VertexPtr arg = call->args()[i];
        VertexAdaptor<meta_op_func_param> param = function_params[i];

        if (param->type() == op_func_param_callback) {
          on_func_param_callback(call, i);
        } else {
          if (!function->is_extern) {
            create_set(as_lvalue(function, i), arg);
          }

          if (param->var()->ref_flag) {
            create_set(arg, as_rvalue(function, i));
          }
        }
      }
    }
  }

  void on_constructor_call(VertexAdaptor<op_constructor_call> call) {
    FunctionPtr function = call->get_func_id();
    VertexRange function_params = function->get_params();

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

      if (cur->type() == op_var && (ifi_tp == ifi_unset || ifi_tp == ifi_isset)) {
        create_set(cur, tp_var);
      }

      if ((cur->type() == op_var && ifi_tp != ifi_unset) || (ifi_tp > ifi_isset && cur->type() == op_index)) {
        tinf::Node *node = get_tinf_node(cur);
        if (node->isset_flags == 0) {
          create_isset_check(node);
        }
        node->isset_flags |= ifi_tp;
      }
    }
  }

  void on_function(FunctionPtr function) {
    VertexRange params = function->get_params();
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
  explicit CollectMainEdgesPass(CollectMainEdgesCallback *callback) :
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


TypeInfererF::TypeInfererF() {
  tinf::register_inferer(new tinf::TypeInferer());
}

void TypeInfererF::execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os) {
  AUTO_PROF (tinf_infer_gen_dep);
  CollectMainEdgesCallback callback(tinf::get_inferer());
  CollectMainEdgesPass pass(&callback);
  run_function_pass(input.function, &pass);
  os << input;
}

void TypeInfererF::on_finish(DataStream<FunctionAndCFG> &) {
  //FIXME: rebalance Queues
  vector<Task *> tasks = tinf::get_inferer()->get_tasks();
  for (int i = 0; i < (int)tasks.size(); i++) {
    register_async_task(tasks[i]);
  }
}

void TypeInfererEndF::on_finish(DataStream<FunctionAndCFG> &os) {
  tinf::get_inferer()->check_restrictions();
  tinf::get_inferer()->finish();

  for (auto &f_and_cfg : tmp_stream.get_as_vector()) {
    os << f_and_cfg;
  }
}
