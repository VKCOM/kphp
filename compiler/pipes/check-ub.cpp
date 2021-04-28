// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-ub.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/gentree.h"

/*
 * C++ undefined behaviour fixes.
 *
 * What is UB and where does it come from? See this:
 *  function f1() { global $arr; $arr[0][] = 1; return 1; }
 *  function f2() { global $arr; $arr[0][0] = f1(); }
 * Without fixes, f2() is codegenerated as
 *  (v$arr[0]).set_value(0, f$f1());
 * But during f1() evaluation, v$arr[0] can become reallocated.
 * With fixes, f2() is codegenerated as
 *  SAFE_SET_VALUE(v$arr[0], 0, int, f$f1(), int)
 * Which generally means creating a temporary variable for f$f1() result storing and assigning it only after.
 *
 * Another example:
 *  $s .= f($s);
 * When one variable is on the left and on the right, it's safer to extract rhs as a separate variable.
 *
 * Another example:
 *  $r = $s++ + $s++;
 * This is a warning (undefined behavior).
 *
 * Another example:
 *  function f($v) { A::$ids[] = $v; }
 *  foreach (A::$ids as &$v) f($v);
 * This is an UB, because f() reallocates A::$ids, that can invalid the &$v reference.
 *
 * So, this pipe finds such occations and either fixes them with safe operations or produces a warning.
 */
const int UB_SIGSEGV = 3;
TLS<VarPtr> last_ub_error;


static inline bool in_set(const std::unordered_set<VarPtr> *a, VarPtr elem) {
  if (a != nullptr && a->find(elem) != a->end()) {
    *last_ub_error = elem;
    return true;
  }
  return false;
}

static bool sets_intersect(const std::unordered_set<VarPtr> *a, const std::unordered_set<VarPtr> *b) {
  if (a == nullptr || b == nullptr) {
    return false;
  }
  for (VarPtr a_elem : *a) {
    if (in_set(b, a_elem)) {
      return true;
    }
  }
  return false;
}

bool is_same_var(const VarPtr &a, const VarPtr &b) {
  if (a == b) {
    *last_ub_error = a;
    return true;
  }
  return in_set(a->bad_vars, b) || in_set(b->bad_vars, a) ||
         sets_intersect(a->bad_vars, b->bad_vars);
}

bool is_var_written(const FunctionPtr &function, const VarPtr &var) {
  if (function->bad_vars == nullptr || (!var->is_in_global_scope() && var->bad_vars == nullptr)) {
    return false;
  }
  if (in_set(function->bad_vars, var)) {
    return true;
  }
  return sets_intersect(var->bad_vars, function->bad_vars);
}


class UBMergeData {
private:
  vector<VarPtr> writes_;
  vector<VarPtr> reads_;
  vector<VarPtr> index_refs_;
  vector<FunctionPtr> functions_;

  template<class A, class B, class F>
  static void check(
    const vector<A> &first,
    const vector<B> &second,
    F f,
    int error_mask, int *res_error) {

    if ((*res_error & error_mask) == error_mask) {
      return;
    }

    for (const A &a : first) {
      for (const B &b : second) {
        if (f(a, b)) {
          *res_error |= error_mask;
          return;
        }
      }
    }
  }

  static int check(const UBMergeData &first, const UBMergeData &second) {
    int err = 0;
    //sigsegv
    check(first.functions_, second.index_refs_, is_var_written, UB_SIGSEGV, &err);
    check(second.functions_, first.index_refs_, is_var_written, UB_SIGSEGV, &err);
    check(first.writes_, second.writes_, is_same_var, UB_SIGSEGV, &err);

    //just ub
    check(first.reads_, second.writes_, is_same_var, UB_SIGSEGV, &err);
    check(first.writes_, second.reads_, is_same_var, UB_SIGSEGV, &err);

    return err;
  }

  static int check(const UBMergeData &data, FunctionPtr function) {
    int err = 0;
    vector<FunctionPtr> tmp(1, function);
    check(tmp, data.index_refs_, is_var_written, UB_SIGSEGV, &err);
    return err;
  }

public:
  UBMergeData() {
  };

  int merge_with(const UBMergeData &other, bool no_check_flag) {
    int err = 0;
    if (!no_check_flag) {
      err = check(*this, other);
    }
    reads_.insert(reads_.end(), other.reads_.begin(), other.reads_.end());
    writes_.insert(writes_.end(), other.writes_.begin(), other.writes_.end());
    index_refs_.insert(index_refs_.end(), other.index_refs_.begin(), other.index_refs_.end());
    functions_.insert(functions_.end(), other.functions_.begin(), other.functions_.end());
    return err;
  }

  int check_index_refs(const vector<VarPtr> &vars) {
    int err = 0;
    check(functions_, vars, is_var_written, UB_SIGSEGV, &err);
    check(reads_, vars, is_same_var, UB_SIGSEGV, &err);
    check(writes_, vars, is_same_var, UB_SIGSEGV, &err);
    return err;
  }

  int called_by(FunctionPtr function) {
    int err = check(*this, function);
    functions_.push_back(function);
    return err;
  }

  static UBMergeData create_from_var(VertexAdaptor<op_var> var, bool index_flag) {
    UBMergeData res;
    if (var->rl_type == val_l) {
      res.writes_.push_back(var->var_id);
      if (index_flag) {
        res.index_refs_.push_back(var->var_id);
      }
    } else {
      res.reads_.push_back(var->var_id);
    }
    return res;
  }

  static UBMergeData create_from_func_ptr(VertexAdaptor<op_func_ptr> func_ptr) {
    UBMergeData res;
    res.functions_.push_back(func_ptr->func_id);
    return res;
  }
};

void fix_ub_dfs(VertexPtr v, UBMergeData *data, VertexPtr parent = VertexPtr()) {
  stage::set_location(v->get_location());

  *last_ub_error = VarPtr();
  if (v->type() == op_var) {
    *data = UBMergeData::create_from_var(v.as<op_var>(), parent && parent->type() == op_index);
  } else if (v->type() == op_func_ptr) {
    *data = UBMergeData::create_from_func_ptr(v.as<op_func_ptr>());
  } else {
    Location save_location = stage::get_location();

    int res = 0;
    bool do_not_check = vk::any_of_equal(v->type(), op_ternary, op_log_or, op_log_and, op_log_or_let, op_log_and_let, op_unset);

    for (auto i : *v) {
      UBMergeData node_data;
      VarPtr last_ub_error_save = *last_ub_error;
      fix_ub_dfs(i, &node_data, v);
      *last_ub_error = last_ub_error_save;
      res |= data->merge_with(node_data, do_not_check);
    }
    if (auto call = v.try_as<op_func_call>()) {
      FunctionPtr function = call->func_id;
      res |= data->called_by(function);
    }
    stage::set_location(save_location);

    if (res > 0) {
      bool supported = vk::any_of_equal(v->type(), op_set, op_set_value, op_push_back, op_push_back_return, op_array, op_index)
                       || OpInfo::rl(v->type()) == rl_set;
      if (supported) {
        v->extra_type = op_ex_safe_version;
      } else {
        kphp_warning (fmt_format("Dangerous undefined behaviour {}, [var = {}]", OpInfo::str(v->type()), (*last_ub_error)->get_human_readable_name()));
      }
    }
  }
}

void fix_ub(VertexPtr v, vector<VarPtr> *foreach_vars) {
  if (v->type() == op_global || v->type() == op_static) {
    return;
  }
  if (OpInfo::type(v->type()) == cycle_op ||
      vk::any_of_equal(v->type(), op_list, op_seq_comma, op_seq_rval, op_if, op_try, op_catch, op_seq, op_case, op_default, op_noerr)) {
    if (auto foreach_v = v.try_as<op_foreach>()) {
      auto params = foreach_v->params();
      VertexPtr x = params->x();
      if (x->ref_flag) {
        VertexPtr xs = params->xs();
        while (xs->type() == op_index) {
          xs = xs.as<op_index>()->array();
        }
        VarPtr var = xs->type() == op_var ? xs.as<op_var>()->var_id : xs.as<op_instance_prop>()->var_id;
        foreach_vars->push_back(var);
        fix_ub(foreach_v->cmd(), foreach_vars);
        foreach_vars->pop_back();
        return;
      }
    }
    for (auto i : *v) {
      fix_ub(i, foreach_vars);
    }
    return;
  }

  UBMergeData data;
  fix_ub_dfs(v, &data);
  int err = data.check_index_refs(*foreach_vars);
  if (err > 0) {
    kphp_warning (fmt_format("Dangerous undefined behaviour {}, [foreach var = {}]", OpInfo::str(v->type()), (*last_ub_error)->get_human_readable_name()));
  }
}

void CheckUBF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Check for undefined behaviour");
  stage::set_function(function);

  if (!function->is_extern()) {
    vector<VarPtr> foreach_vars;
    fix_ub(function->root->cmd(), &foreach_vars);
  }

  if (stage::has_error()) {
    return;
  }

  os << function;
}
