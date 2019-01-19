#include "compiler/pipes/calc-bad-vars.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"
#include "compiler/utils/idmap.h"

/*** Common algorithm ***/
// Graph G
// Each node have Data
// Data have to be merged with all descendant's Data.
template<class VertexT>
class MergeReachalbeCallback {
public:
  virtual void for_component(const vector<VertexT> &component, const vector<VertexT> &edges) = 0;

  virtual ~MergeReachalbeCallback() {}
};

template<class VertexT>
class MergeReachalbe {
private:
  typedef IdMap<vector<VertexT>> GraphT;

  void dfs(const VertexT &vertex, const GraphT &graph, IdMap<int> *was, vector<VertexT> *topsorted) {
    if ((*was)[vertex]) {
      return;
    }
    (*was)[vertex] = 1;
    for (const auto &next_vertex : graph[vertex]) {
      dfs(next_vertex, graph, was, topsorted);
    }
    topsorted->push_back(vertex);
  }

  void dfs_component(VertexT vertex, const GraphT &graph, int color, IdMap<int> *was,
                     vector<int> *was_color, vector<VertexT> *component, vector<VertexT> *edges) {
    int other_color = (*was)[vertex];
    if (other_color == color) {
      return;
    }
    if (other_color != 0) {
      if ((*was_color)[other_color] != color) {
        (*was_color)[other_color] = color;
        edges->push_back(vertex);
      }
      return;
    }
    (*was)[vertex] = color;
    component->push_back(vertex);
    for (const auto &next_vertex : graph[vertex]) {
      dfs_component(next_vertex, graph, color, was, was_color, component, edges);
    }
  }

public:
  void run(const GraphT &graph, const GraphT &rev_graph, const vector<VertexT> &vertices,
           MergeReachalbeCallback<VertexT> *callback) {
    int vertex_n = (int)vertices.size();
    IdMap<int> was(vertex_n);
    vector<VertexT> topsorted;
    for (const auto &vertex : vertices) {
      dfs(vertex, rev_graph, &was, &topsorted);
    }

    std::fill(was.begin(), was.end(), 0);
    vector<int> was_color(vertex_n + 1, 0);
    int current_color = 0;
    for (auto vertex_it = topsorted.rbegin(); vertex_it != topsorted.rend(); ++vertex_it) {
      auto &vertex = *vertex_it;
      if (was[vertex]) {
        continue;
      }
      vector<VertexT> component;
      vector<VertexT> edges;
      dfs_component(vertex, graph, ++current_color, &was,
                    &was_color, &component, &edges);

      callback->for_component(component, edges);
    }
  }
};

struct FuncCallGraph {
  int n;
  vector<FunctionPtr> functions;
  IdMap<vector<FunctionPtr>> graph;
  IdMap<vector<FunctionPtr>> rev_graph;

  FuncCallGraph(vector<FunctionPtr> other_functions, vector<DepData> &dep_datas) :
    n((int)other_functions.size()),
    functions(std::move(other_functions)),
    graph(n),
    rev_graph(n) {

    for (int cur_id = 0, i = 0; i < n; i++, cur_id++) {
      set_index(functions[i], cur_id);
    }

    for (int i = 0; i < n; i++) {
      FunctionPtr to = functions[i];
      DepData &data = dep_datas[i];

      for (const auto &from : data.dep) {
        rev_graph[from].push_back(to);
      }
      graph[to] = std::move(data.dep);
    }
  }
};

class CalcBadVars {
private:
  class MergeBadVarsCallback : public MergeReachalbeCallback<FunctionPtr> {
  public:
    void for_component(const vector<FunctionPtr> &component, const vector<FunctionPtr> &edges) override {
      std::unordered_set<VarPtr, typename VarPtr::Hash> bad_vars_uniq;

      for (FunctionPtr f : component) {
        bad_vars_uniq.insert(f->tmp_vars.begin(), f->tmp_vars.end());
      }

      for (FunctionPtr f : edges) {
        kphp_assert(f->bad_vars != nullptr);
        bad_vars_uniq.insert(f->bad_vars->begin(), f->bad_vars->end());
      }

      auto bad_vars = new vector<VarPtr>(bad_vars_uniq.begin(), bad_vars_uniq.end());
      for (FunctionPtr f : component) {
        f->bad_vars = bad_vars;
      }
    }
  };

  void stupid_dfs(FunctionPtr function, set<FunctionPtr> *was, set<VarPtr> *vars) {
    if (was->count(function)) {
      return;
    }
    was->insert(function);
    vars->insert(function->global_var_ids.begin(), function->global_var_ids.end());
    for (int i = (int)function->dep.size() - 1; i >= 0; i--) {
      stupid_dfs(function->dep[i], was, vars);
    }
  }

  void generate_bad_vars(FuncCallGraph &call_graph, vector<DepData> &dep_datas) {
    FunctionPtr wait_func = G->get_function("wait");
    if (!wait_func) {     // когда functions.txt пустой или отключенный для dev
      return;
    }

    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr func = call_graph.functions[i];
      func->tmp_vars = std::move(dep_datas[i].used_global_vars);

      if (func->root->resumable_flag) {
        if (G->env().get_verbosity() > 1) {
          fprintf(stderr, "Resumable [%s]\n", func->name.c_str());
        }
        call_graph.graph[wait_func].push_back(func);
        call_graph.rev_graph[func].push_back(wait_func);

        call_graph.graph[func].push_back(wait_func);
        call_graph.rev_graph[wait_func].push_back(func);
      }
    }

    MergeBadVarsCallback callback;
    MergeReachalbe<FunctionPtr> merge_bad_vars;
    merge_bad_vars.run(call_graph.graph, call_graph.rev_graph, call_graph.functions, &callback);
  }

  template<class GraphT, class WasT, class VertexT, class ParentT>
  void mark(const GraphT &graph, WasT &was, VertexT vertex, ParentT &parents) {
    if (was[vertex]) {
      return;
    }
    was[vertex] = 1;
    for (const auto &next : graph[vertex]) {
      if (!was[next]) {
        parents[next] = vertex;
        mark(graph, was, next, parents);
      }
    }
  }


  void calc_resumable(const FuncCallGraph &call_graph, const vector<DepData> &dep_data) {
    for (int i = 0; i < call_graph.n; i++) {
      for (const auto &fork : dep_data[i].forks) {
        fork->root->resumable_flag = true;
      }
    }
    IdMap<char> from_resumable(call_graph.n);
    IdMap<char> into_resumable(call_graph.n);
    IdMap<FunctionPtr> from_parents(call_graph.n);
    IdMap<FunctionPtr> to_parents(call_graph.n);
    for (const auto &func : call_graph.functions) {
      if (func->root->resumable_flag) {
        mark(call_graph.graph, from_resumable, func, from_parents);
        mark(call_graph.rev_graph, into_resumable, func, to_parents);
      }
    }
    for (const auto &func : call_graph.functions) {
      if (from_resumable[func] && into_resumable[func]) {
        func->root->resumable_flag = true;
        func->fork_prev = from_parents[func];
        func->wait_prev = to_parents[func];
      }
    }
    for (const auto &func : call_graph.functions) {
      if (func->root->resumable_flag && func->should_be_sync) {
        kphp_error (0, format("Function [%s] marked with @kphp-sync, but turn up to be resumable\n"
                               "Function is resumable because of calls chain:\n%s\n", func->name.c_str(), func->get_resumable_path().c_str()));
      }
    }
    if (G->env().get_print_resumable_graph()) {
      for (const auto &func : call_graph.functions) {
        if (!func->root->resumable_flag) {
          continue;
        }
        for (const auto &next : call_graph.graph[func]) {
          if (!next->root->resumable_flag) {
            continue;
          }
          fprintf(stderr, "%s -> %s\n", func->name.c_str(), next->name.c_str());
        }
      }
    }
  }

  void save_func_dep(FuncCallGraph &call_graph) {
    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr function = call_graph.functions[i];
      function->dep = std::move(call_graph.graph[function]);
    }
  }

  class MergeRefVarsCallback : public MergeReachalbeCallback<VarPtr> {
  private:
    const IdMap<vector<VarPtr>> &to_merge_;
  public:
    MergeRefVarsCallback(const IdMap<vector<VarPtr>> &to_merge) :
      to_merge_(to_merge) {
    }

    void for_component(const vector<VarPtr> &component, const vector<VarPtr> &edges) {
      vector<VarPtr> *res = new vector<VarPtr>();
      for (const auto &var : component) {
        res->insert(res->end(), to_merge_[var].begin(), to_merge_[var].end());
      }
      for (const auto &var : edges) {
        if (var->bad_vars != nullptr) {
          res->insert(res->end(), var->bad_vars->begin(), var->bad_vars->end());
        }
      }
      my_unique(res);
      if (res->empty()) {
        delete res;
        return;
      }
      for (const auto &var : component) {
        var->bad_vars = res;
      }
    }
  };

  void generate_ref_vars(vector<DepData> &dep_datas) {
    vector<VarPtr> vars;
    for (auto data : dep_datas) {
      vars.insert(vars.end(), data.used_ref_vars.begin(), data.used_ref_vars.end());
    }
    int vars_n = (int)vars.size();
    my_unique(&vars);
    assert ((int)vars.size() == vars_n);
    for (int cur_id = 0, i = 0; i < vars_n; i++, cur_id++) {
      set_index(vars[i], cur_id);
    }

    IdMap<vector<VarPtr>> rev_graph(vars_n), graph(vars_n), ref_vars(vars_n);
    for (const auto &data : dep_datas) {
      for (const auto &edge : data.global_ref_edges) {
        ref_vars[edge.second].push_back(edge.first);
      }
      for (const auto &edge : data.ref_ref_edges) {
        graph[edge.second].push_back(edge.first);
        rev_graph[edge.first].push_back(edge.second);
      }
    }

    MergeRefVarsCallback callback(ref_vars);
    MergeReachalbe<VarPtr> merge_ref_vars;
    merge_ref_vars.run(graph, rev_graph, vars, &callback);
  }

public:
  void run(std::vector<std::pair<FunctionPtr, DepData>> &tmp_vec) {
    int all_n = (int)tmp_vec.size();
    std::vector<FunctionPtr> functions(all_n);
    std::vector<DepData> dep_datas(all_n);
    for (int i = 0; i < all_n; i++) {
      functions[i] = tmp_vec[i].first;
      dep_datas[i] = std::move(tmp_vec[i].second);
    }

    {
      FuncCallGraph call_graph(std::move(functions), dep_datas);
      calc_resumable(call_graph, dep_datas);
      generate_bad_vars(call_graph, dep_datas);
      save_func_dep(call_graph);
    }

    generate_ref_vars(dep_datas);
  }
};

void CalcBadVarsF::execute(std::pair<FunctionPtr, DepData> data, DataStream<FunctionPtr> &) {
  tmp_stream << std::move(data);
}

void CalcBadVarsF::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();

  stage::set_name("Calc bad vars (for UB check)");
  auto tmp_vec = tmp_stream.get_as_vector();
  CalcBadVars{}.run(tmp_vec);
  for (const auto &fun_dep : tmp_vec) {
    os << fun_dep.first;
  }
}
