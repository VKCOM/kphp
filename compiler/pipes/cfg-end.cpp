// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/cfg-end.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"
#include "compiler/vertex.h"

struct MergeData {
  int id;
  VarPtr var;
};

bool operator<(const MergeData&a, const MergeData&b) {
  // sort by types if they're different;
  // sort by name otherwise (name$vN < name$vM with N<M, name < name$vN)
  const int eq = type_out(tinf::get_type(a.var), gen_out_style::txt).compare(type_out(tinf::get_type(b.var), gen_out_style::txt));
  return eq == 0 ? a.var->name < b.var->name : eq < 0;
}

bool operator==(const MergeData &a, const MergeData &b) {
  return type_out(tinf::get_type(a.var), gen_out_style::txt) ==
         type_out(tinf::get_type(b.var), gen_out_style::txt);
}

bool operator!=(const MergeData &a, const MergeData &b) {
  return !(a == b);
}

static VarPtr merge_vars(FunctionPtr function, const std::vector<VarPtr> &vars, const std::string &new_name) {
  VarPtr new_var = G->create_var(new_name, VarData::var_unknown_t);;
  //new_var->tinf = vars[0]->tinf; //hack, TODO: fix it
  new_var->tinf_node.copy_type_from(tinf::get_type(vars[0]));

  int param_i = -1;
  for (auto var : vars) {
    new_var->is_read_only &= var->is_read_only;
    if (var->type() == VarData::var_param_t) {
      param_i = var->param_i;
    } else if (var->type() == VarData::var_local_t) {
      //FIXME: remember to remove all unused variables
      //func->local_var_ids.erase (*i);
      auto tmp = std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var);
      if (function->local_var_ids.end() != tmp) {
        function->local_var_ids.erase(tmp);
      } else {
        kphp_fail();
      }

    } else {
      kphp_assert_msg(false, "unreachable");
    }
  }
  if (param_i != -1) {
    new_var->type() = VarData::var_param_t;
    function->param_ids[param_i] = new_var;
  } else {
    new_var->type() = VarData::var_local_t;
    function->local_var_ids.push_back(new_var);
  }

  return new_var;
}

static void merge_same_type(FunctionPtr function, const std::forward_list<std::vector<std::vector<VertexAdaptor<op_var>>>> &split_parts_list) {
  std::vector<MergeData> to_merge;
  for (const auto &parts : split_parts_list) {
    to_merge.clear();
    to_merge.reserve(parts.size());
    for (int i = 0; i < parts.size(); i++) {
      to_merge.emplace_back(MergeData{i, parts[i][0]->var_id});
    }
    std::sort(to_merge.begin(), to_merge.end());

    std::vector<int> ids;
    const auto n = parts.size();
    for (size_t i = 0; i <= n; i++) {
      if (i == n || (i > 0 && to_merge[i - 1] != to_merge[i])) {
        std::vector<VarPtr> vars;
        vars.reserve(ids.size());
        for (int id : ids) {
          vars.push_back(parts[id][0]->var_id);
        }

        const std::string &new_name = vars[0]->name; // name or name$vN
        VarPtr new_var = merge_vars(function, vars, new_name);
        for (int id : ids) {
          for (auto v : parts[id]) {
            v->var_id = new_var;
          }
        }

        ids.clear();
      }
      if (i == n) {
        break;
      }
      ids.push_back(to_merge[i].id);
    }
  }
}

static void check_uninited(const std::forward_list<VertexAdaptor<op_var>> &uninited_vars) {
  for (auto v : uninited_vars) {
    if (tinf::get_type(v)->ptype() == tp_mixed) {
      continue;
    }

    stage::set_location(v->get_location());
    kphp_warning (fmt_format("Variable ${} may be used uninitialized", v->var_id->name));
  }
}

void CFGEndF::execute(FunctionAndCFG data, DataStream<FunctionPtr> &os) {
  stage::set_name("Control flow graph. End");
  stage::set_function(data.function);
  check_uninited(data.data.uninited_vars);
  merge_same_type(data.function, data.data.split_parts_list);

  if (stage::has_error()) {
    return;
  }

  os << data.function;
}
