#include "compiler/pipes/filter-only-actually-used.h"

void FilterOnlyActuallyUsedFunctionsF::calc_throws_having_call_edges(vector<FunctionAndEdges> &all) {
  vector<FunctionPtr> from;

  for (auto &f_and_e : all) {
    if (f_and_e.function->root->throws_flag) {
      from.push_back(f_and_e.function);
    }
  }

  IdMap<vector<FunctionPtr>> graph((int)all.size());
  for (auto &f_and_e : all) {
    for (auto &edge : *f_and_e.edges) {
      if (!edge.inside_try) {
        graph[edge.called_f].push_back(f_and_e.function);
      }
    }
  }

  for (auto &f : from) {
    calc_throws_dfs(f, graph);
  }
}
void FilterOnlyActuallyUsedFunctionsF::calc_throws_dfs(FunctionPtr from, IdMap<vector<FunctionPtr>> &graph) {
  throws_func_cnt++;
  for (const FunctionPtr &to : graph[from]) {
    if (!to->root->throws_flag) {
      to->root->throws_flag = true;
      calc_throws_dfs(to, graph);
    }
  }
}
void FilterOnlyActuallyUsedFunctionsF::calc_actually_used_having_call_edges(vector<FunctionAndEdges> &all, DataStream<FunctionPtr> &os) {
  IdMap<vector<FunctionAndEdges::EdgeInfo>> graph((int)all.size());
  IdMap<int> used((int)all.size());

  for (const auto &f_and_e : all) {
    graph[f_and_e.function] = std::move(*f_and_e.edges);
    delete f_and_e.edges;
  }

  for (const auto &f_and_e : all) {
    bool should_be_used_apriori =
      f_and_e.function->type() == FunctionData::func_global ||
      (f_and_e.function->type() == FunctionData::func_extern && f_and_e.function->name == "wait");
    if (should_be_used_apriori && !used[f_and_e.function]) {
      calc_actually_used_dfs(f_and_e.function, graph, used, os);
    }
  }
}
void FilterOnlyActuallyUsedFunctionsF::calc_actually_used_dfs(FunctionPtr from, IdMap<vector<FunctionAndEdges::EdgeInfo>> &graph, IdMap<int> &used, DataStream<FunctionPtr> &os) {
  actually_called_func_cnt++;
  used[from] = 1;
  os << from;

  if (from->is_constructor()) {
    from->class_id->was_constructor_invoked = true;
  }
  for (const auto &to : graph[from]) {
    if (!used[to.called_f]) {
      calc_actually_used_dfs(to.called_f, graph, used, os);
    }
  }
}
void FilterOnlyActuallyUsedFunctionsF::on_finish(DataStream<FunctionPtr> &os) {
  vector<FunctionAndEdges> all = tmp_stream.get_as_vector();

  // присваиваем FunctionData::id
  int cur_id = -1;
  for (auto &i : all) {
    set_index(i.function, ++cur_id);
  }

  stage::set_name("Calc throw");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();
  AUTO_PROF(calc_throws);

  // устанавливаем throws_flag у функций, которые вызывают те, которые делают явный throw
  calc_throws_having_call_edges(all);

  stage::set_name("Calc actual calls");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();
  AUTO_PROF(calc_actual_calls);

  // вычисляем реально достижимые функции, и по мере вычисления прокидываем в os
  // должен идти последним
  calc_actually_used_having_call_edges(all, os);

  stage::die_if_global_errors();
  //printf("There are %d functions that are really reached in code\n", actually_called_func_cnt);
  //printf("There are %d functions that can potentially throw an exception\n", throws_func_cnt);
}
void FilterOnlyActuallyUsedFunctionsF::execute(FunctionAndEdges f, DataStream<FunctionPtr> &) {
  tmp_stream << f;
}
FilterOnlyActuallyUsedFunctionsF::FilterOnlyActuallyUsedFunctionsF() {
  tmp_stream.set_sink(true);
}
