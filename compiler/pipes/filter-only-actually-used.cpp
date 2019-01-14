#include "compiler/pipes/filter-only-actually-used.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/threading/profiler.h"

namespace {

void calc_throws_dfs(FunctionPtr callee, const IdMap<vector<FunctionPtr>> &throws_graph) {
  for (const FunctionPtr &caller : throws_graph[callee]) {
    if (!caller->root->throws_flag) {
      caller->root->throws_flag = true;
      calc_throws_dfs(caller, throws_graph);
    }
  }
}

void calc_non_empty_body_dfs(FunctionPtr callee, const IdMap<vector<FunctionPtr>> &non_empty_body_graph) {
  for (const FunctionPtr &caller : non_empty_body_graph[callee]) {
    kphp_assert_msg(caller->body_seq != FunctionData::body_value::empty, "Body can't be empty!");
    if (caller->body_seq == FunctionData::body_value::unknown) {
      caller->body_seq = FunctionData::body_value::non_empty;
      calc_non_empty_body_dfs(caller, non_empty_body_graph);
    }
  }
}

void calc_throws_and_body_value_through_call_edges(const vector<FunctionAndEdges> &all) {
  IdMap<vector<FunctionPtr>> throws_graph((int)all.size());
  IdMap<vector<FunctionPtr>> non_empty_body_graph((int)all.size());

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

} // anonymous namespace

void FilterOnlyActuallyUsedFunctionsF::calc_actually_used_having_call_edges(const std::vector<FunctionAndEdges> &all) {
  used_functions.reserve(all.size());
  call_graph.reserve(all.size());

  for (const auto &f_and_e : all) {
    call_graph.emplace(f_and_e.function, std::move(*f_and_e.edges));
    delete f_and_e.edges;
  }

  for (const auto &f_and_e : all) {
    const bool should_be_used_apriori =
      f_and_e.function->type() == FunctionData::func_global ||
      f_and_e.function->type() == FunctionData::func_class_holder ||   // классы нужно прокинуть по пайплайну
     (f_and_e.function->type() == FunctionData::func_extern && f_and_e.function->name == "wait") ||
      f_and_e.function->kphp_lib_export;
    if (should_be_used_apriori && !used_functions.count(f_and_e.function)) {
      calc_actually_used_dfs(f_and_e.function);
    }
  }
}

void FilterOnlyActuallyUsedFunctionsF::calc_actually_used_dfs(FunctionPtr from) {
  actually_called_func_cnt++;
  used_functions.emplace(from);

  if (from->is_constructor()) {
    from->class_id->was_constructor_invoked = true;
  }
  for (const auto &to : call_graph[from]) {
    if (!used_functions.count(to.called_f)) {
      calc_actually_used_dfs(to.called_f);
    }
  }
}

void FilterOnlyActuallyUsedFunctionsF::remove_unused_class_methods(const std::vector<FunctionAndEdges> &all) {
  for (const auto &f_and_e : all) {
    if (f_and_e.function->type() == FunctionData::func_class_holder) {
      f_and_e.function->class_id->members.remove_if(
        [this](const ClassMemberStaticMethod &m) {
          return !used_functions.count(m.function);
        });
      f_and_e.function->class_id->members.remove_if(
        [this](const ClassMemberInstanceMethod &m) {
          return !used_functions.count(m.function);
        });
    }
  }
}

void FilterOnlyActuallyUsedFunctionsF::on_finish(DataStream<FunctionPtr> &os) {
  std::vector<FunctionAndEdges> all = tmp_stream.get_as_vector();

  // присваиваем FunctionData::id
  int cur_id = -1;
  for (auto &i : all) {
    set_index(i.function, ++cur_id);
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
  calc_actually_used_having_call_edges(all);

  // удаляем неиспользуемые методы классов
  remove_unused_class_methods(all);
  stage::die_if_global_errors();

  // прокидываем в os реально достижимые фукнции, должен идти последним
  for (const FunctionPtr &f : used_functions) {
    os << f;
  }
  used_functions.clear();
  call_graph.clear();
}

void FilterOnlyActuallyUsedFunctionsF::execute(FunctionAndEdges f, DataStream<FunctionPtr> &) {
  tmp_stream << f;
}

FilterOnlyActuallyUsedFunctionsF::FilterOnlyActuallyUsedFunctionsF() {
  tmp_stream.set_sink(true);
}
