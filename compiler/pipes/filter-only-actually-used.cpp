#include "compiler/pipes/filter-only-actually-used.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/threading/profiler.h"

namespace {

void calc_throws_dfs(FunctionPtr callee, const IdMap<std::vector<FunctionPtr>> &throws_graph) {
  for (const FunctionPtr &caller : throws_graph[callee]) {
    if (!caller->root->throws_flag) {
      caller->root->throws_flag = true;
      calc_throws_dfs(caller, throws_graph);
    }
  }
}

void calc_non_empty_body_dfs(FunctionPtr callee, const IdMap<std::vector<FunctionPtr>> &non_empty_body_graph) {
  for (const FunctionPtr &caller : non_empty_body_graph[callee]) {
    kphp_assert_msg(caller->body_seq != FunctionData::body_value::empty, "Body can't be empty!");
    if (caller->body_seq == FunctionData::body_value::unknown) {
      caller->body_seq = FunctionData::body_value::non_empty;
      calc_non_empty_body_dfs(caller, non_empty_body_graph);
    }
  }
}

void calc_throws_and_body_value_through_call_edges(const vector<FunctionAndEdges> &all) {
  IdMap<std::vector<FunctionPtr>> throws_graph(static_cast<int>(all.size()));
  IdMap<std::vector<FunctionPtr>> non_empty_body_graph(static_cast<int>(all.size()));

  for (const auto &f_and_e : all) {
    for (const auto &edge : *f_and_e.edges) {
      if (!edge.inside_try) {
        throws_graph[edge.called_f].emplace_back(f_and_e.function);
      }
      if (f_and_e.function->body_seq == FunctionData::body_value::unknown
          && edge.called_f->body_seq != FunctionData::body_value::empty) {
        non_empty_body_graph[edge.called_f].emplace_back(f_and_e.function);
      }
    }
  }

  for (const auto &f_and_e : all) {
    if (f_and_e.function->root->throws_flag) {
      calc_throws_dfs(f_and_e.function, throws_graph);
    }
    if (f_and_e.function->body_seq == FunctionData::body_value::non_empty) {
      calc_non_empty_body_dfs(f_and_e.function, non_empty_body_graph);
    }
  }

  for (const auto &f_and_e : all) {
    if (f_and_e.function->body_seq == FunctionData::body_value::unknown) {
      f_and_e.function->body_seq = FunctionData::body_value::empty;
    }
  }
}

void calc_actually_used_dfs(FunctionPtr from, IdMap<FunctionPtr> &used_functions,
                            const IdMap<std::vector<FunctionAndEdges::EdgeInfo>> &call_graph) {
  used_functions[from] = from;

  if (from->is_constructor()) {
    from->class_id->was_constructor_invoked = true;
  }
  for (const auto &to : call_graph[from]) {
    if (!used_functions[to.called_f]) {
      calc_actually_used_dfs(to.called_f, used_functions, call_graph);
    }
  }
}

IdMap<FunctionPtr> calc_actually_used_having_call_edges(const std::vector<FunctionAndEdges> &all) {
  IdMap<FunctionPtr> used_functions(static_cast<int>(all.size()));
  IdMap<std::vector<FunctionAndEdges::EdgeInfo>> call_graph(static_cast<int>(all.size()));

  for (const auto &f_and_e : all) {
    call_graph[f_and_e.function] = std::move(*f_and_e.edges);
    delete f_and_e.edges;
  }

  for (const auto &f_and_e : all) {
    const bool should_be_used_apriori =
      f_and_e.function->type() == FunctionData::func_global ||
      f_and_e.function->type() == FunctionData::func_class_holder ||   // классы нужно прокинуть по пайплайну
     (f_and_e.function->type() == FunctionData::func_extern && f_and_e.function->name == "wait") ||
      f_and_e.function->kphp_lib_export;
    if (should_be_used_apriori && !used_functions[f_and_e.function]) {
      calc_actually_used_dfs(f_and_e.function, used_functions, call_graph);
    }
  }
  return used_functions;
}

void remove_unused_class_methods(const std::vector<FunctionAndEdges> &all, const IdMap<FunctionPtr> &used_functions) {
  for (const auto &f_and_e : all) {
    if (f_and_e.function->type() == FunctionData::func_class_holder) {
      f_and_e.function->class_id->members.remove_if(
        [&used_functions](const ClassMemberStaticMethod &m) {
          return get_index(m.function) == -1 || !used_functions[m.function];
        });
      f_and_e.function->class_id->members.remove_if(
        [&used_functions](const ClassMemberInstanceMethod &m) {
          return get_index(m.function) == -1 || !used_functions[m.function];
        });
    }
  }
}

} // anonymous namespace

void FilterOnlyActuallyUsedFunctionsF::on_finish(DataStream<FunctionPtr> &os) {
  std::vector<FunctionAndEdges> all = tmp_stream.get_as_vector();

  // присваиваем FunctionData::id
  for (int id = 0; id < all.size(); ++id) {
    kphp_assert(get_index(all[id].function) == -1);
    set_index(all[id].function, id);
  }

  stage::set_name("Calc throws and body value");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  // 1) set 'throws_flag' for functions which calls other functions with explicit throw
  // 2) calc 'body_seq' for functions with unknown body value which calls non empty functions
  calc_throws_and_body_value_through_call_edges(all);

  stage::set_name("Calc actual calls");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  // вычисляем реально достижимые функции
  auto used_functions = calc_actually_used_having_call_edges(all);

  // удаляем неиспользуемые методы классов
  remove_unused_class_methods(all, used_functions);
  stage::die_if_global_errors();

  // прокидываем в os реально достижимые фукнции, должен идти последним
  for (const auto &f : used_functions) {
    if (f) {
      os << f;
    }
  }
}

void FilterOnlyActuallyUsedFunctionsF::execute(FunctionAndEdges f, DataStream<FunctionPtr> &) {
  tmp_stream << f;
}

FilterOnlyActuallyUsedFunctionsF::FilterOnlyActuallyUsedFunctionsF() {
  tmp_stream.set_sink(true);
}
