#pragma once
#ifndef __PASS_UB_CPP__
#include "compiler/compiler-core.h"
struct DepData {
  vector <FunctionPtr> dep;
  vector <VarPtr> used_global_vars;

  vector <VarPtr> used_ref_vars;
  vector <std::pair <VarPtr, VarPtr> > ref_ref_edges;
  vector <std::pair <VarPtr, VarPtr> > global_ref_edges;

  vector <FunctionPtr> forks;
};
inline void swap (DepData &a, DepData &b) {
  swap (a.dep, b.dep);
  swap (a.used_global_vars, b.used_global_vars);
  swap (a.used_ref_vars, b.used_ref_vars);
  swap (a.ref_ref_edges, b.ref_ref_edges);
  swap (a.global_ref_edges, b.global_ref_edges);
  swap (a.forks, b.forks);
}
class CalcFuncDepPass : public FunctionPassBase {
  private:
    AUTO_PROF (calc_func_dep);
    DepData data;
  public:
    string get_description() {
      return "Calc function depencencies";
    }

    bool check_function (FunctionPtr function) {
      return default_check_function (function) && function->type() != FunctionData::func_extern;
    }
    VertexPtr on_enter_vertex (VertexPtr vertex, LocalT *local __attribute__((unused))) {
      //NB: There is no user functions in default values of any kind.
      if (vertex->type() == op_func_call) {
        VertexAdaptor <op_func_call> call = vertex;
        FunctionPtr other_function = call->get_func_id();
        data.dep.push_back (other_function);
        if (other_function->type() != FunctionData::func_extern && !other_function->varg_flag) {
          int ii = 0;
          for (auto val : call->args()) {
            VarPtr to_var = other_function->param_ids[ii];
            if (to_var->is_reference) { //passed as reference
              while (val->type() == op_index) {
                val = val.as <op_index>()->array();
              }
              kphp_assert (val->type() == op_var || val->type() == op_instance_prop);
              VarPtr from_var = val->get_var_id();
              if (from_var->type() == VarData::var_global_t) {
                data.global_ref_edges.push_back (std::make_pair (from_var, to_var));
              } else if (from_var->is_reference) {
                data.ref_ref_edges.push_back (std::make_pair (from_var, to_var));
              }
            }
            ii++;
          }
        }
      } else if (vertex->type() == op_func_ptr) {
        data.dep.push_back (vertex.as <op_func_ptr>()->get_func_id());
      } else if (vertex->type() == op_constructor_call) {
        data.dep.push_back (vertex.as <op_constructor_call>()->get_func_id());
      } else if (vertex->type() == op_var/* && vertex->rl_type == val_l*/) {
        VarPtr var = vertex.as <op_var>()->get_var_id();
        if (var->type() == VarData::var_global_t) {
          data.used_global_vars.push_back (var);
        }
      } else if (vertex->type() == op_func_param) {
        VarPtr var = vertex.as <op_func_param>()->var()->get_var_id();
        if (var->is_reference) {
          data.used_ref_vars.push_back (var);
        }
      } else if (vertex->type() == op_fork) {
        VertexPtr func_call = vertex.as <op_fork>()->func_call();
        kphp_error (func_call->type() == op_func_call, "Fork can be called only for function call.");
        data.forks.push_back (func_call->get_func_id());
      }

      return vertex;
    }

    void on_finish (void) {
      my_unique (&data.dep);
      my_unique (&data.used_global_vars);
    }

    DepData *get_data_ptr() {
      return &data;
    }
};

/*** Common algorithm ***/
// Graph G
// Each node have Data
// Data have to be merged with all descendant's Data.
template <class VertexT>
class MergeReachalbeCallback {
  public:
    virtual void for_component (const vector <VertexT> &component, const vector <VertexT> &edges) = 0;
    virtual ~MergeReachalbeCallback(){}
};
template <class VertexT>
class MergeReachalbe {
  private:
    typedef IdMap <vector <VertexT> > GraphT;
    void dfs (const VertexT &vertex, const GraphT &graph, IdMap <int> *was, vector <VertexT> *topsorted) {
      if ((*was)[vertex]) {
        return;
      }
      (*was)[vertex] = 1;
      for (auto const &next_vertex : graph[vertex]) {
        dfs (next_vertex, graph, was, topsorted);
      }
      topsorted->push_back (vertex);
    }

    void dfs_component (VertexT vertex, const GraphT &graph, int color, IdMap <int> *was, 
               vector <int> *was_color, vector <VertexT> *component, vector <VertexT> *edges) {
      int other_color = (*was)[vertex];
      if (other_color == color) {
        return;
      }
      if (other_color != 0) {
        if ((*was_color)[other_color] != color) {
          (*was_color)[other_color] = color;
          edges->push_back (vertex);
        }
        return;
      }
      (*was)[vertex] = color;
      component->push_back (vertex);
      for (auto const &next_vertex : graph[vertex]) {
        dfs_component (next_vertex, graph, color, was, was_color, component, edges);
      }
    }

  public:
    void run(const GraphT &graph, const GraphT &rev_graph, const vector <VertexT> &vertices, 
        MergeReachalbeCallback <VertexT> *callback) {
      int vertex_n = (int)vertices.size();
      IdMap <int> was (vertex_n, 0);
      vector <VertexT> topsorted;
      for (auto const &vertex : vertices) {
        dfs (vertex, rev_graph, &was, &topsorted);
      }

      std::fill (was.begin(), was.end(), 0);
      vector <int> was_color (vertex_n + 1, 0);
      int current_color = 0;
      for (auto vertex_it = topsorted.rbegin(); vertex_it != topsorted.rend(); ++vertex_it) {
        auto &vertex = *vertex_it;
        if (was[vertex]) {
          continue;
        }
        vector <VertexT> component;
        vector <VertexT> edges;
        dfs_component (vertex, graph, ++current_color, &was,
            &was_color, &component, &edges);

        callback->for_component (component, edges);
      }
    }
};

struct FuncCallGraph {
    int n;
    vector <FunctionPtr> functions;
    IdMap < vector <FunctionPtr> > graph;
    IdMap < vector <FunctionPtr> > rev_graph;

    FuncCallGraph (vector <FunctionPtr> &other_functions, const vector <DepData *> dep_datas) :
      n ((int)other_functions.size()),
      functions(),
      graph (n),
      rev_graph (n) {

      std::swap (functions, other_functions);

      for (int cur_id = 0, i = 0; i < n; i++, cur_id++) {
        set_index (&functions[i], cur_id);
      }

      for (int i = 0; i < n; i++) {
        FunctionPtr to = functions[i];
        DepData *data = dep_datas[i];

        for (auto const &from : data->dep) {
          rev_graph[from].push_back (to);
        }
        std::swap (graph[to], data->dep);
      }
    }
};

class CalcBadVars {
  private:
    class MergeBadVarsCallback : public MergeReachalbeCallback <FunctionPtr> {
      public:
        void for_component (const vector <FunctionPtr> &component, const vector <FunctionPtr> &edges) {
          vector <VarPtr> *bad_vars = new vector <VarPtr>();
          for (int i = (int)component.size() - 1; i >= 0; i--) {
            FunctionPtr function = component[i];
            bad_vars->insert (bad_vars->end(), function->tmp_vars.begin(), function->tmp_vars.end());
          }

          for (int i = (int)edges.size() - 1; i >= 0; i--) {
            FunctionPtr function = edges[i];
            if (function->bad_vars == NULL) {
              fprintf (stderr, "%s\n", function->name.c_str());
            }
            assert (function->bad_vars != NULL);
            bad_vars->insert (bad_vars->end(), function->bad_vars->begin(), function->bad_vars->end());
          }
          my_unique (bad_vars);
          for (int i = (int)component.size() - 1; i >= 0; i--) {
            FunctionPtr function = component[i];
            function->bad_vars = bad_vars;
          }
        }
    };

    void stupid_dfs (FunctionPtr function, set <FunctionPtr> *was, set <VarPtr> *vars) {
      if (was->count (function)) {
        return;
      }
      was->insert (function);
      vars->insert (function->global_var_ids.begin(), function->global_var_ids.end());
      for (int i = (int)function->dep.size() - 1; i >= 0; i--) {
        stupid_dfs (function->dep[i], was, vars);
      }
    }

    void generate_bad_vars (FuncCallGraph &call_graph, vector <DepData *> &dep_datas) {
      FunctionPtr wait_func = G->get_function_unsafe ("unserialize");

      for (int i = 0; i < call_graph.n; i++) {
        FunctionPtr func = call_graph.functions[i];
        swap (func->tmp_vars, dep_datas[i]->used_global_vars);

        if (func->root->resumable_flag) {
          if (G->env().get_verbosity() > 1) {
            fprintf(stderr, "Resumable [%s]\n", func->name.c_str());
          }
          call_graph.graph[wait_func].push_back (func);
          call_graph.rev_graph[func].push_back (wait_func);

          call_graph.graph[func].push_back (wait_func);
          call_graph.rev_graph[wait_func].push_back (func);
        }
      }

      MergeBadVarsCallback callback;
      MergeReachalbe <FunctionPtr> merge_bad_vars;
      merge_bad_vars.run (call_graph.graph, call_graph.rev_graph, call_graph.functions, &callback);
    }

    template <class GraphT, class WasT, class VertexT, class ParentT>
    void mark (const GraphT &graph, WasT &was, VertexT vertex, ParentT &parents) {
      if (was[vertex]) {
        return;
      }
      was[vertex] = 1;
      for (auto const &next : graph[vertex]) {
        if (!was[next]) {
          parents[next] = vertex;
          mark (graph, was, next, parents);
        }
      }
    }


    void calc_resumable (FuncCallGraph &call_graph, vector <DepData *> &dep_data) {
      for (int i = 0; i < call_graph.n; i++) {
        for (auto const &fork : dep_data[i]->forks) {
          fork->root->resumable_flag = true;
        }
      }
      IdMap <char> from_resumable (call_graph.n);
      IdMap <char> into_resumable (call_graph.n);
      IdMap <FunctionPtr> from_parents (call_graph.n);
      IdMap <FunctionPtr> to_parents (call_graph.n);
      for (auto const &func : call_graph.functions) {
        if (func->root->resumable_flag) {
          mark (call_graph.graph, from_resumable, func, from_parents);
          mark (call_graph.rev_graph, into_resumable, func, to_parents);
        }
      }
      for (auto const &func : call_graph.functions) {
        if (from_resumable[func] && into_resumable[func]) {
          func->root->resumable_flag = true;
          func->fork_prev = from_parents[func];
          func->wait_prev = to_parents[func];
        }
      }
      for (auto const &func : call_graph.functions) {
        if (func->root->resumable_flag && func->should_be_sync) {
          kphp_error (0, dl_pstr ("Function [%s] marked with @kphp-sync, but turn up to be resumable\n"
            "Function is resumable because of calls chain:\n%s\n", func->name.c_str(), func->get_resumable_path().c_str()));
        }
      }
      if (G->env().get_print_resumable_graph()) {
        for (auto const &func : call_graph.functions) {
          if (!func->root->resumable_flag) continue;
          for (auto const &next : call_graph.graph[func]) {
            if (!next->root->resumable_flag) continue;
            fprintf(stderr, "%s -> %s\n", func->name.c_str(), next->name.c_str());
          }
        }
      }
    }

    void save_func_dep (FuncCallGraph &call_graph) {
      for (int i = 0; i < call_graph.n; i++) {
        FunctionPtr function = call_graph.functions[i];
        std::swap (function->dep, call_graph.graph[function]);
      }
    }

    class MergeRefVarsCallback : public MergeReachalbeCallback <VarPtr> {
      private:
        const IdMap <vector <VarPtr> > &to_merge_;
      public:
        MergeRefVarsCallback (const IdMap <vector <VarPtr> > &to_merge) :
          to_merge_ (to_merge) {
        }
        void for_component (const vector <VarPtr> &component, const vector <VarPtr> &edges) {
          vector <VarPtr> *res = new vector <VarPtr>();
          for (auto const &var : component) {
            res->insert (res->end(), to_merge_[var].begin(), to_merge_[var].end());
          }
          for (auto const &var : edges) {
            if (var->bad_vars != NULL) {
              res->insert (res->end(), var->bad_vars->begin(), var->bad_vars->end());
            }
          }
          my_unique (res);
          if (res->empty()) {
            delete res;
            return;
          }
          for (auto const &var : component) {
            var->bad_vars = res;
          }
        }
    };

    void generate_ref_vars (vector <DepData *> &dep_datas) {
      vector <VarPtr> vars;
      for (auto data : dep_datas) {
        vars.insert(vars.end(), data->used_ref_vars.begin(), data->used_ref_vars.end());
      }
      int vars_n = (int)vars.size();
      my_unique (&vars);
      assert ((int)vars.size() == vars_n);
      for (int cur_id = 0, i = 0; i < vars_n; i++, cur_id++) {
        set_index (&vars[i], cur_id);
      }

      IdMap < vector <VarPtr> > rev_graph (vars_n), graph (vars_n), ref_vars (vars_n);
      for (auto const &data : dep_datas) {
        for (auto const &edge : data->global_ref_edges) {
          ref_vars[edge.second].push_back (edge.first);
        }
        for (auto const &edge : data->ref_ref_edges) {
          graph[edge.second].push_back (edge.first);
          rev_graph[edge.first].push_back (edge.second);
        }
      }
      
      MergeRefVarsCallback callback (ref_vars);
      MergeReachalbe <VarPtr> merge_ref_vars;
      merge_ref_vars.run (graph, rev_graph, vars, &callback);

    }
  public:
    void run (const vector <pair <FunctionPtr, DepData *> > &tmp_vec) {
      int all_n = (int)tmp_vec.size();
      vector <FunctionPtr> functions (all_n);
      vector <DepData *> dep_datas (all_n);
      for (int i = 0; i < all_n; i++) {
        functions[i] = tmp_vec[i].first;
        dep_datas[i] = tmp_vec[i].second;
      }

      {
        FuncCallGraph call_graph (functions, dep_datas);
        calc_resumable (call_graph, dep_datas);
        generate_bad_vars (call_graph, dep_datas);
        save_func_dep (call_graph);
      }

      generate_ref_vars (dep_datas);
    }
};
#endif 
