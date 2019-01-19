#include "compiler/pipes/type-inferer.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"
#include "compiler/inferring/ifi.h"
#include "compiler/inferring/lvalue.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/restriction-isset.h"
#include "compiler/inferring/restriction-less.h"
#include "compiler/inferring/rvalue.h"
#include "compiler/inferring/type-node.h"
#include "compiler/scheduler/scheduler-base.h"
#include "compiler/scheduler/task.h"

class CollectMainEdgesPass : public FunctionPassBase {
private:
  tinf::Node *node_from_rvalue(const RValue &rvalue) {
    if (rvalue.node == nullptr) {
      kphp_assert (rvalue.type != nullptr);
      return new tinf::TypeNode(rvalue.type);
    }

    return rvalue.node;
  }

  void require_node(const RValue &rvalue) {
    if (rvalue.node != nullptr) {
      tinf::get_inferer()->add_node(rvalue.node);
    }
  }

  void create_set(const LValue &lvalue, const RValue &rvalue) {
    tinf::Edge *edge = new tinf::Edge();
    edge->from = lvalue.value;
    edge->from_at = lvalue.key;
    edge->to = node_from_rvalue(rvalue);
    tinf::get_inferer()->add_edge(edge);
    tinf::get_inferer()->add_node(edge->from);
  }

  void create_less(const RValue &lhs, const RValue &rhs) {
    tinf::Node *a = node_from_rvalue(lhs);
    tinf::Node *b = node_from_rvalue(rhs);
    tinf::get_inferer()->add_node(a);
    tinf::get_inferer()->add_node(b);
    tinf::get_inferer()->add_restriction(new RestrictionLess(a, b));
  }

  void create_isset_check(const RValue &rvalue) {
    tinf::Node *a = node_from_rvalue(rvalue);
    tinf::get_inferer()->add_node(a);
    tinf::get_inferer()->add_restriction(new RestrictionIsset(a));
  }


  RValue as_set_value(VertexPtr v) {
    if (v->type() == op_set) {
      return as_rvalue(v.as<op_set>()->rhs());
    }

    if (v->type() == op_prefix_inc ||
        v->type() == op_prefix_dec ||
        v->type() == op_postfix_dec ||
        v->type() == op_postfix_inc) {
      VertexAdaptor<meta_op_unary> unary = v;
      auto one = VertexAdaptor<op_int_const>::create();
      auto res = VertexAdaptor<op_add>::create(unary->expr(), one);
      set_location(one, stage::get_location());
      set_location(res, stage::get_location());
      return as_rvalue(res);
    }

    if (OpInfo::arity(v->type()) == binary_opp) {
      VertexAdaptor<meta_op_binary> binary = v;
      VertexPtr res = create_vertex(OpInfo::base_op(v->type()), binary->lhs(), binary->rhs());
      set_location(res, stage::get_location());
      return as_rvalue(res);
    }

    kphp_fail();
    return RValue();
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

  template<class A, class B>
  void create_set(const A &a, const B &b) {
    create_set(as_lvalue(a), as_rvalue(b));
  }

  template<class A, class B>
  void create_less(const A &a, const B &b) {
    create_less(as_rvalue(a), as_rvalue(b));
  }

  template<class A>
  void require_node(const A &a) {
    require_node(as_rvalue(a));
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

    // // restriction on return type
    bool is_any = false;
    if (auto rule = callback_param->type_rule.try_as<op_common_type_rule>()) {
      if (auto son = rule->rule().try_as<op_type_rule>()) {
        is_any = son->type_help == tp_Any;

        if (!is_any && callback_function && callback_function->is_extern()) {
          if (auto rule_of_callback = callback_function->root->type_rule.try_as<op_common_type_rule>()) {
            if (auto son_of_callback_rule = rule_of_callback->rule().try_as<op_type_rule>()) {
              is_any = son_of_callback_rule->type_help == son->type_help;
            }
          }
        }
      }
    }

    if (!is_any) {
      auto fake_func_call = VertexAdaptor<op_func_call>::create(call->get_next());
      fake_func_call->type_rule = callback_param->type_rule;
      fake_func_call->set_func_id(call_function);
      create_less(as_rvalue(callback_function, -1), fake_func_call);
    }

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


    if (!(function->varg_flag && function->is_extern())) {
      for (int i = 0; i < call->args().size(); ++i) {
        VertexPtr arg = call->args()[i];
        VertexAdaptor<meta_op_func_param> param = function_params[i];

        if (param->type() == op_func_param_callback) {
          on_func_param_callback(call, i);
        } else {
          if (!function->is_extern()) {
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
    create_less(G->get_class("Exception"), throw_op->exception());
    create_less(throw_op->exception(), G->get_class("Exception"));
  }

  void on_try(VertexAdaptor<op_try> try_op) {
    create_set(try_op->exception(), G->get_class("Exception"));
  }

  void on_set_op(VertexPtr v) {
    VertexPtr lval;
    if (OpInfo::arity(v->type()) == binary_opp) {
      lval = v.as<meta_op_binary>()->lhs();
    } else if (OpInfo::arity(v->type()) == unary_opp) {
      lval = v.as<meta_op_unary>()->expr();
    } else {
      kphp_fail();
    }
    create_set(lval, as_set_value(v));
  }


  void ifi_fix(VertexPtr v) {
    is_func_id_t ifi_tp = get_ifi_id(v);
    if (ifi_tp == ifi_error) {
      return;
    }
    for (auto cur : *v) {
      if (cur->type() == op_var && cur->get_var_id()->is_constant()) {
        continue;
      }

      if (cur->type() == op_var && (ifi_tp == ifi_unset || ifi_tp == ifi_isset || ifi_tp == ifi_is_null)) {
        create_set(cur, tp_var);
      }

      if ((cur->type() == op_var && ifi_tp != ifi_unset) || (ifi_tp > ifi_isset && cur->type() == op_index)) {
        tinf::Node *node = tinf::get_tinf_node(cur);
        if (node->isset_flags == 0) {
          create_isset_check(as_rvalue(node));
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

    if (function->is_extern()) {
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
              VertexPtr rule = common_type_rule->rule();
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
      // @kphp-infer hint/check для @param/@return — это less/set на соответствующие tinf_nodes функции
      for (const FunctionData::InferHint &hint : function->infer_hints) {
        switch (hint.infer_type) {
          case FunctionData::InferHint::infer_mask::check:
            create_less(as_rvalue(function, hint.param_i), hint.type_rule);
            break;
          case FunctionData::InferHint::infer_mask::hint:
            create_set(as_lvalue(function, hint.param_i), hint.type_rule);
            break;
          case FunctionData::InferHint::infer_mask::cast:
            // ничего не делаем, т.к. там просто поставился type_help в parse_and_apply_function_kphp_phpdoc()
            break;
        }
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
      case op_list:
        on_list(v);
        break;
      case op_throw:
        on_throw(v);
        break;
      case op_fork:
        on_fork(v);
        break;
      case op_try:
        on_try(v);
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
    if (!__sync_bool_compare_and_swap(&var->tinf_flag, false, true)) {
      return;
    }
    require_node(var);
    if (var->init_val) {
      create_set(var, var->init_val);
    }

    // для всех переменных-инстансов (локальные, параметры и т.п.) делаем restriction'ы, что классы те же что в phpdoc
    if (!current_function->assumptions_for_vars.empty()) {
      ClassPtr cl;
      AssumType assum = assumption_get_for_var(current_function, var->name, cl);
      if (assum == assum_instance) {                  // var == cl
        create_less(var, cl->type_data);
        create_less(cl->type_data, var);
      } else if (assum == assum_instance_array) {     // cl[] <= var <= OrFalse<cl[]>
        create_less(var, TypeData::create_array_type_data(cl->type_data, true));
        create_less(TypeData::create_array_type_data(cl->type_data), var);
      }
    }
  }

  nullptr_t on_finish() {
    call_on_var(current_function->local_var_ids);
    call_on_var(current_function->global_var_ids);
    call_on_var(current_function->static_var_ids);
    call_on_var(current_function->header_global_var_ids);
    call_on_var(current_function->const_var_ids);
    call_on_var(current_function->header_const_var_ids);
    call_on_var(current_function->param_ids);
    return {};
  }
};


void TypeInfererF::execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os) {
  CollectMainEdgesPass pass;
  run_function_pass(input.function, &pass);
  os << input;
}

void TypeInfererF::on_finish(DataStream<FunctionAndCFG> &) {
  vector<Task *> tasks = tinf::get_inferer()->get_tasks();
  for (int i = 0; i < (int)tasks.size(); i++) {
    register_async_task(tasks[i]);
  }
}
