/*
    This file is part of VK/KittenPHP-DB-Engine.

    VK/KittenPHP-DB-Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    VK/KittenPHP-DB-Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VK/KittenPHP-DB-Engine.  If not, see <http://www.gnu.org/licenses/>.

    This program is released under the GPL with the additional exemption
    that compiling, linking, and/or using OpenSSL is allowed.
    You are free to remove this exemption from derived works.

    Copyright 2012-2013 Vkontakte Ltd
              2012-2013 Arseny Smirnov
              2012-2013 Aliaksei Levin

    Copyright 2014 Telegraph Inc
              2014 Arseny Smirnov
              2014 Aliaksei Levin
*/

#include "stage.h"
#include "data.h"
#include "function-pass.h"

class CheckNestedForeachPass : public FunctionPassBase {
  vector <VarPtr> foreach_vars;
  vector <VarPtr> forbidden_vars;
  int in_unset;
public:
  struct LocalT : public FunctionPassBase::LocalT {
    int to_remove;
    VarPtr to_forbid;
  };

  void init() {
    in_unset = 0;
    forbidden_vars = vector<VarPtr>();
    foreach_vars = vector<VarPtr>();
  }

  string get_description() {
    return "Try to detect common errors: nested foreach";
  }

  bool check_function (FunctionPtr function) {
    return default_check_function (function) && function->type() != FunctionData::func_extern;
  }


  VertexPtr on_enter_vertex (VertexPtr vertex, LocalT *local) {
    local->to_remove = 0;
    local->to_forbid = VarPtr();
    if (vertex->type() == op_foreach) {
      VertexAdaptor <op_foreach> foreach_v = vertex;
      VertexAdaptor <op_foreach_param> params = foreach_v->params();
      VertexPtr xs = params->xs();
      while (xs->type() == op_index) {
        xs = xs.as <op_index>()->array();
      }
      if (xs->type() == op_var) {
        VarPtr xs_var = xs.as<op_var>()->get_var_id();
        foreach_vars.push_back(xs_var);
        local->to_remove++;
      }
      VertexPtr x = params->x();
      kphp_assert (x->type() == op_var);
      VarPtr x_var = x.as<op_var> ()->get_var_id ();
      for (int i = 0; i < foreach_vars.size(); i++) {
        if (x_var->name == foreach_vars[i]->name) {
          kphp_warning (dl_pstr("Foreach value \"%s\" shadows array, key or value of outer foreach", x_var->name.c_str()));
        }
      }
      foreach_vars.push_back(x_var);
      local->to_remove++;
      if (x->ref_flag) {
        local->to_forbid = x_var;
        for (int i = 0; i < forbidden_vars.size(); i++) {
          if (forbidden_vars[i]->name == x_var->name) {
            swap(forbidden_vars[i], forbidden_vars.back());
            forbidden_vars.pop_back();
            break;
          }
        }
      }
      if (params->has_key()) {
        local->to_remove++;
        VertexPtr key = params->key();
        kphp_assert (key->type() == op_var);
        VarPtr key_var = key.as<op_var>()->get_var_id();
        for (int i = 0; i < foreach_vars.size(); i++) {
          if (key_var->name == foreach_vars[i]->name) {
            kphp_warning (dl_pstr("Foreach key \"%s\" shadows array, key or value of outer foreach", key_var->name.c_str()));
          }
        }
        foreach_vars.push_back(key_var);
      }
    }
    if (vertex->type() == op_unset)
      in_unset++;
    if (vertex->type() == op_var && !in_unset) {
      VarPtr var = vertex.as <op_var>()->get_var_id();
      for (int i = 0; i < forbidden_vars.size(); i++) {
        if (var->name == forbidden_vars[i]->name) {
          kphp_warning (dl_pstr("Reference foreach value \"%s\" is used after foreach", var->name.c_str()));
          swap(forbidden_vars[i], forbidden_vars.back());
          forbidden_vars.pop_back();
          break;
        }
      }
    }
    return vertex;
  }


  VertexPtr on_exit_vertex (VertexPtr vertex, LocalT *local) {
    for (int i = 0; i < local->to_remove; i++) {
      kphp_assert(foreach_vars.size());
      foreach_vars.pop_back();
    }
    if (local->to_forbid.not_null()) {
      forbidden_vars.push_back(local->to_forbid);
    }
    if (vertex->type() == op_unset)
      in_unset--;
    return vertex;
  }
};

void analize_foreach (FunctionPtr function) {
  if (function->root->type() != op_function) {
    return;
  }
  CheckNestedForeachPass pass;
  run_function_pass (function, &pass);
}

class CheckArraysPass : public FunctionPassBase {
public:

  string get_description() {
    return "Try to detect common errors: arrays";
  }

  bool check_function (FunctionPtr function) {
    return default_check_function (function) && function->type() != FunctionData::func_extern;
  }


  VertexPtr on_enter_vertex (VertexPtr vertex, LocalT *local __attribute__((unused))) {
    VertexPtr to_check;
    if (vertex->type() == op_array) {
      to_check = vertex;
    } else if (vertex->type() == op_var) {
      VarPtr var = vertex.as<op_var>()->get_var_id ();
      if (var->is_constant) {
        VertexPtr init = var->init_val;
        if (init->type () == op_array) {
          to_check = init;
        }
      }
    }
    if (to_check.is_null()) {
      return vertex;
    }
    bool have_arrow = false;
    bool have_int_key = false;
    set<string> used_keys;
    int id = 0;
    for (VertexRange i = all(to_check); !i.empty(); i.next()) {
      VertexPtr v = (*i);
      if (v->type() == op_double_arrow) {
        have_arrow = true;
        VertexPtr key = v.as<op_double_arrow>()->key();
        have_int_key |= key->type() == op_int_const;
        if (key->type() == op_string || key->type() == op_int_const) {
          const string& str = key->get_string();
          if (used_keys.find(str) != used_keys.end()) {
            kphp_warning (dl_pstr("Duplicate key '%s' in array", str.c_str()));
          }
          used_keys.insert(str);
        }
      } else {
        if (have_arrow && have_int_key) {
          return vertex;
        }
        const string& str = int_to_str(id++);
        used_keys.insert(str);
      }
    }
    return vertex;
  }
};

void analize_arrays (FunctionPtr function) {
  if (function->root->type() != op_function) {
    return;
  }
  CheckArraysPass pass;
  run_function_pass (function, &pass);
}

