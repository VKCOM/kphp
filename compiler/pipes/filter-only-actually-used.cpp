// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/filter-only-actually-used.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/threading/profiler.h"

namespace {

using EdgeInfo = FilterOnlyActuallyUsedFunctionsF::EdgeInfo;
using FunctionAndEdges = FilterOnlyActuallyUsedFunctionsF::FunctionAndEdges;

struct ThrowGraphNode {
  FunctionPtr f;
  std::vector<VertexAdaptor<op_try>> &try_stack;

  ThrowGraphNode(FunctionPtr f, std::vector<VertexAdaptor<op_try>> &try_stack)
    : f{f}
    , try_stack{try_stack} {}
};

void calc_throws_dfs(FunctionPtr callee, IdMap<std::vector<ThrowGraphNode>> &throws_graph) {
  for (ThrowGraphNode &n : throws_graph[callee]) {
    FunctionPtr caller = n.f;

    for (auto e : callee->exceptions_thrown) {
      if (!CalcActualCallsEdgesPass::handle_exception(n.try_stack, e)) {
        bool exception_added = caller->exceptions_thrown.insert(e).second;
        if (exception_added) {
          caller->throws_reason = callee;
        }
        if ((caller != callee) && exception_added) {
          calc_throws_dfs(caller, throws_graph);
        }
      }
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

void calc_throws_and_body_value_through_call_edges(std::vector<FunctionAndEdges> &all) {
  IdMap<std::vector<ThrowGraphNode>> throws_graph(static_cast<int>(all.size()));
  IdMap<std::vector<FunctionPtr>> non_empty_body_graph(static_cast<int>(all.size()));
  for (const auto &f_and_e : all) {
    for (const auto &edge : f_and_e.second) {
      if (edge.inside_fork) {
        edge.called_f->body_seq = FunctionData::body_value::non_empty;
      }
    }
  }

  for (auto &f_and_e : all) {
    FunctionPtr fun = f_and_e.first;
    for (auto &edge : f_and_e.second) {
      if (edge.can_throw) {
        kphp_assert(edge.called_f);
        throws_graph[edge.called_f].emplace_back(fun, edge.try_stack);
      }
      if (fun->body_seq == FunctionData::body_value::unknown && edge.called_f->body_seq != FunctionData::body_value::empty) {
        non_empty_body_graph[edge.called_f].emplace_back(fun);
      }
    }
  }

  for (const auto &f_and_e : all) {
    FunctionPtr fun = f_and_e.first;
    if (fun->can_throw()) {
      calc_throws_dfs(fun, throws_graph);
    }
    if (fun->body_seq == FunctionData::body_value::non_empty) {
      calc_non_empty_body_dfs(fun, non_empty_body_graph);
    }
  }

  for (const auto &f_and_e : all) {
    FunctionPtr fun = f_and_e.first;
    if (fun->body_seq == FunctionData::body_value::unknown) {
      fun->body_seq = FunctionData::body_value::empty;
    }
  }
}

void calc_actually_used_dfs(FunctionPtr from, IdMap<FunctionPtr> &used_functions, const IdMap<std::vector<EdgeInfo>> &call_graph) {
  used_functions[from] = from;

  for (const auto &to : call_graph[from]) {
    if (!used_functions[to.called_f]) {
      calc_actually_used_dfs(to.called_f, used_functions, call_graph);
    }
  }
}

void mark_profiler_dfs(FunctionPtr caller, const IdMap<std::vector<EdgeInfo>> &call_graph) {
  if (caller->profiler_state == FunctionData::profiler_status::disable) {
    return;
  }
  for (const auto &edge : call_graph[caller]) {
    if (edge.called_f->profiler_state == FunctionData::profiler_status::disable) {
      edge.called_f->profiler_state = FunctionData::profiler_status::enable_as_child;
      mark_profiler_dfs(edge.called_f, call_graph);
    }
  }
}

struct DfsException : std::runtime_error {
  DfsException(FunctionPtr f, std::string &&msg)
    : runtime_error(std::move(msg))
    , function(f) {}

  FunctionPtr function;
};

void mark_performance_inspections_dfs(FunctionPtr caller, const IdMap<std::vector<EdgeInfo>> &call_graph) {
  for (const auto &edge : call_graph[caller]) {
    if (edge.called_f->is_extern()) {
      continue;
    }
    const auto analyse_inherit_res = edge.called_f->performance_inspections_for_analysis.merge_with_caller(caller->performance_inspections_for_analysis);
    if (analyse_inherit_res.first == PerformanceInspections::InheritStatus::conflict) {
      throw DfsException{edge.called_f, fmt_format("@kphp-analyze-performance conflict, one caller enables '{}' while other disables it",
                                                   PerformanceInspections::inspection2string(analyse_inherit_res.second))};
    }
    const auto warning_inherit_res = edge.called_f->performance_inspections_for_warning.merge_with_caller(caller->performance_inspections_for_warning);
    if (warning_inherit_res.first != PerformanceInspections::InheritStatus::no_need) {
      edge.called_f->performance_inspections_for_warning_parents.emplace_front(caller);
    }
    if (warning_inherit_res.first == PerformanceInspections::InheritStatus::conflict) {
      throw DfsException{edge.called_f, fmt_format("@kphp-warn-performance conflict, one caller enables '{}' while other disables it\n"
                                                   "Enabled by: {}\nDisabled by: {}",
                                                   PerformanceInspections::inspection2string(warning_inherit_res.second),
                                                   edge.called_f->get_performance_inspections_warning_chain(warning_inherit_res.second, false),
                                                   edge.called_f->get_performance_inspections_warning_chain(warning_inherit_res.second, true))};
    }
    if (vk::any_of_equal(PerformanceInspections::InheritStatus::ok, analyse_inherit_res.first, warning_inherit_res.first)) {
      mark_performance_inspections_dfs(edge.called_f, call_graph);
    }
  }
}

IdMap<FunctionPtr> calc_actually_used_having_call_edges(std::vector<FunctionAndEdges> &all) {
  IdMap<FunctionPtr> used_functions(static_cast<int>(all.size()));
  IdMap<std::vector<EdgeInfo>> call_graph(static_cast<int>(all.size()));

  for (auto &f_and_e : all) {
    call_graph[f_and_e.first] = std::move(f_and_e.second);
  }

  for (const auto &f_and_e : all) {
    FunctionPtr fun = f_and_e.first;
    const bool should_be_used_apriori = fun->is_main_function() || fun->type == FunctionData::func_class_holder
                                        || // classes should be carried along the pipeline
                                        (fun->is_extern() && vk::any_of_equal(fun->name, "wait", "make_clone")) || fun->kphp_lib_export
                                        || (fun->modifiers.is_instance() && fun->local_name() == ClassData::NAME_OF_TO_STRING)
                                        || (fun->modifiers.is_instance() && fun->local_name() == ClassData::NAME_OF_WAKEUP);
    if (should_be_used_apriori && !used_functions[fun]) {
      calc_actually_used_dfs(fun, used_functions, call_graph);
    }
  }

  try {
    for (auto fun : used_functions) {
      if (fun) {
        if (G->settings().profiler_level.get()) {
          mark_profiler_dfs(fun, call_graph);
        }
        if (fun->performance_inspections_for_analysis.has_their_own_inspections() || fun->performance_inspections_for_warning.has_their_own_inspections()) {
          mark_performance_inspections_dfs(fun, call_graph);
        }
      }
    }
  } catch (const DfsException &ex) {
    stage::set_function(ex.function);
    kphp_error(0, ex.what());
  }

  return used_functions;
}

void remove_unused_class_methods(const std::vector<FunctionAndEdges> &all, const IdMap<FunctionPtr> &used_functions) {
  for (const auto &f_and_e : all) {
    FunctionPtr fun = f_and_e.first;
    if (fun->type == FunctionData::func_class_holder) {
      fun->class_id->members.remove_if(
        [&used_functions](const ClassMemberStaticMethod &m) { return get_index(m.function) == -1 || !used_functions[m.function]; });
      fun->class_id->members.remove_if(
        [&used_functions](const ClassMemberInstanceMethod &m) { return get_index(m.function) == -1 || !used_functions[m.function]; });
    }
  }
}

} // namespace

void FilterOnlyActuallyUsedFunctionsF::on_finish(DataStream<FunctionPtr> &os) {
  if (G->settings().profiler_level.get() == 2) {
    G->get_main_file()->main_function->profiler_state = FunctionData::profiler_status::enable_as_root;
  }
  if (G->settings().enable_full_performance_analyze.get()) {
    G->get_main_file()->main_function->performance_inspections_for_analysis = PerformanceInspections{PerformanceInspections::all_inspections};
  }

  auto all = tmp_stream.flush_as_vector();

  // assigning the FunctionData::id
  for (int id = 0; id < all.size(); ++id) {
    kphp_assert(get_index(all[id].first) == -1);
    set_index(all[id].first, id);
  }

  // uncomment this to debug "invalid index of IdMap: -1"
  //  for (const auto &f_and_e : all) {
  //    for (const auto &edge : f_and_e.second) {
  //      if (get_index(edge.called_f) == -1) {
  //        printf("-1 of %s -> %s\n", f_and_e.first->name.c_str(), edge.called_f->name.c_str());
  //      }
  //    }
  //  }

  stage::set_name("Calc throws and body value");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  // 1) set 'can_throw' for functions which calls other functions with explicit throw
  // 2) calc 'body_seq' for functions with unknown body value which calls non empty functions
  calc_throws_and_body_value_through_call_edges(all);

  stage::set_name("Calc actual calls");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  // calculate the actually reachable functions;
  // it clears the all[i].edges
  auto used_functions = calc_actually_used_having_call_edges(all);

  // remove the unused class methods
  remove_unused_class_methods(all, used_functions);
  stage::die_if_global_errors();

  // forward the reachable functions into the data stream;
  // this should be the last step
  for (const auto &f : used_functions) {
    if (f) {
      os << f;
    }
  }
}
