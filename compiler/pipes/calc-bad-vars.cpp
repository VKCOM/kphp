// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <queue>
#include "compiler/pipes/calc-bad-vars.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
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
  using GraphT = IdMap<vector<VertexT>>;

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

using CallstackColors = std::vector<function_palette::color_t>;

class Callstack {
  using raw = std::vector<FunctionPtr>;
  using raw_set = std::unordered_set<FunctionPtr>;
  using it = std::vector<FunctionPtr>::iterator;
  using element = FunctionPtr;

private:
  raw data;
  raw_set data_set;
  it begin;
  it end;

public:
  Callstack(size_t cap) {
    data.reserve(cap);
    data_set.reserve(cap);
    begin = data.begin();
    end = data.end();
  }

public:
  size_t size() const {
    return data.size();
  }

  element &front() {
    return *begin;
  }

  element &back() {
    return *(end - 1);
  }

  void push_back(const element &el) {
    data.push_back(el);
    begin = data.begin();
    end = data.end();
    data_set.insert(el);
  }

  void pop_back() {
    data_set.erase(data.back());
    data.pop_back();
  }

  bool contains(const element &el) const {
    return vk::contains(data_set, el);
  }

  size_t hash() {
    return front()->id + (back()->id * 100);
  }

  void reset_narrow() {
    begin = data.begin();
    end = data.end();
  }

  void narrow(const function_palette::Rule &rule) {
    const auto start_color = rule.colors.front();
    const auto end_color = rule.colors.back();

    while (begin != data.end()) {
      if ((*begin)->colors.contains(start_color)) {
        break;
      }
      ++begin;
    }

    while (end != data.begin()) {
      --end;

      if ((*end)->colors.contains(end_color)) {
        break;
      }
    }

    if (rule.colors.size() == 1) {
      --begin;
    }

    if (end != data.end()) {
      ++end;
    }
  }
};

ProfilerRaw &get_p_check_func() {
  static CachedProfiler profiler{"!!! check color func"};
  return *profiler;
}

class CheckFunctionsColors {
  FuncCallGraph call_graph;
  function_palette::Palette palette;
  std::unordered_set<size_t> handled;

public:
  CheckFunctionsColors(FuncCallGraph& call_graph)
    : call_graph(call_graph), palette(G->get_function_palette()) {}

public:
  void check() {
    Callstack callstack(50);
    CallstackColors colors;
    colors.reserve(50);

    const auto main_func = G->get_main_file()->main_function;
    if (!need_check(callstack, main_func)) {
      return;
    }

    {
      AutoProfiler pp{get_p_check_func()};
      check_func(colors, callstack, main_func);
    }
  }

  bool need_check(const Callstack &callstack, const FunctionPtr &func) {
    if (callstack.size() >= 50) {
      return false;
    }
    if (callstack.contains(func)) {
      return false;
    }
    return true;
  }

  void check_func(CallstackColors &colors, Callstack &callstack, const FunctionPtr &func) {
    callstack.push_back(func);

    const auto sep_colors = func->colors.sep_colors();
    for (const auto &sep_color : sep_colors) {
      colors.push_back(sep_color);
    }

    auto need_check_subtree = true;

    if (callstack.size() > 1) {
      const auto matched_rules = match(colors);

      if (!matched_rules.empty()) {
        for (const auto &rule : matched_rules) {
          error(callstack, rule);
        }

        // we do not continue checking the subtree further, since an error was found
        need_check_subtree = false;
      }
    }

    if (need_check_subtree) {
      for (const auto &next : func->next_with_colors) {
        if (!need_check(callstack, next)) {
          continue;
        }

        check_func(colors, callstack, next);
      }
    }

    for (int i = 0; i < sep_colors.size(); ++i) {
      colors.pop_back();
    }
    callstack.pop_back();
  }

  static function_palette::colors_t calc_mask(const CallstackColors &colors) {
    function_palette::colors_t mask = 0;
    for (const auto &color : colors) {
      mask |= color;
    }
    return mask;
  }

  static bool match_rule(const function_palette::Rule& rule, CallstackColors& colors) {
    const auto match_mask = calc_mask(colors);
    if (!rule.contains_in(match_mask)) {
      return false;
    }

    return match_two_vectors(rule.colors, colors);
  }

  static bool match_two_vectors(const std::vector<function_palette::colors_t> &first, CallstackColors& second) {
    if (first.empty() || second.empty()) {
      return false;
    }

    auto first_it = first.end() - 1;
    auto second_it = second.end() - 1;

    while (true) {
      auto matched = false;

      if (*first_it == *second_it) {
        matched = true;
      }

      if (matched) {
        if (first_it == first.begin()) {
          return true;
        }
        if (second_it == second.begin()) {
          return false;
        }
        --first_it;
        --second_it;
      } else {
        if (second_it == second.begin()) {
          return {};
        }
        --second_it;
      }
    }
  }

  std::vector<function_palette::Rule> match(CallstackColors &colors) {
    std::vector<function_palette::Rule> rules;

    for (const auto &group : palette.groups()) {
      for (auto it = group.rules().rbegin(); it != group.rules().rend(); ++it) {
        const auto rule = *it;
        const auto matched = match_rule(rule, colors);

        if (!matched) {
          continue;
        }

        if (rule.is_error()) {
          rules.push_back(rule);
        }
        break;
      }
    }

    return rules;
  }

  bool find_callstack(FunctionPtr start, FunctionPtr end, std::vector<FunctionPtr> &callstack) {
    if (callstack.size() > 20) {
      return false;
    }

    callstack.push_back(start);

    const auto callees = call_graph.graph[start];
    for (const auto &callee : callees) {
      if (callee->color_status == FunctionData::color_status::non_color) {
        continue;
      }

      if (callee == end) {
        return true;
      }

      const auto found = find_callstack(callee, end, callstack);
      if (!found) {
        continue;
      }

      return true;
    }

    callstack.pop_back();

    return false;
  }

  std::string format_callstack(std::vector<FunctionPtr> &callstack) const {
    if (callstack.empty()) {
      return "";
    }

    std::string desc;

    for (auto it = callstack.begin(); it != callstack.end(); ++it) {
      const auto frame = *it;
      const auto name = frame->get_human_readable_name() + "()";

      desc += "  ";
      desc += TermStringFormat::paint(name, TermStringFormat::yellow);

      if (!frame->colors.empty()) {
        desc += " with following ";
        desc += frame->colors.to_string(palette);
      }

      if (it != callstack.end() - 1) {
        desc += "\n\n";
      }
    }

    return desc;
  }

  void error(Callstack& callstack, const function_palette::Rule &rule) {
    callstack.narrow(rule);

    stage::set_function(callstack.front());

    const auto hash = callstack.hash() * (rule.id + 1);
    const auto was_handled = handled.count(hash) == 1;
    if (was_handled) {
      callstack.reset_narrow();
      return;
    }
    handled.insert(hash);

    std::vector<FunctionPtr> found_callstack;
    found_callstack.reserve(20);
    const auto found = find_callstack(callstack.front(), callstack.back(), found_callstack);
    if (found) {
      found_callstack.push_back(callstack.back());
    }

    const auto parent_name = callstack.front()->get_human_readable_name() + "()";
    const auto child_name = callstack.back()->get_human_readable_name() + "()";
    const auto callstack_format = format_callstack(found_callstack);

    callstack.reset_narrow();

    const auto description = fmt_format("{} ({} call {})\n\n{}\n\nProduced according to the following rule:\n  {}",
                             rule.error,
                             TermStringFormat::paint_green(parent_name),
                             TermStringFormat::paint_green(child_name),
                             callstack_format,
                             rule.to_string(palette)
    );

    kphp_error(0, description);
  }
};

class CalcBadVars {
private:
  class MergeBadVarsCallback : public MergeReachalbeCallback<FunctionPtr> {
    IdMap<vector<VarPtr>> tmp_vars;
  public:
    explicit MergeBadVarsCallback(IdMap<vector<VarPtr>> tmp_vars) : tmp_vars(std::move(tmp_vars)) {}
    void for_component(const vector<FunctionPtr> &component, const vector<FunctionPtr> &edges) override {
      std::unordered_set<VarPtr> bad_vars_uniq;

      for (FunctionPtr f : component) {
        bad_vars_uniq.insert(tmp_vars[f].begin(), tmp_vars[f].end());
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

  void generate_bad_vars(FuncCallGraph &call_graph, vector<DepData> &dep_datas) {
    FunctionPtr wait_func = G->get_function("wait");
    if (!wait_func) { // when functions.txt is empty or disabled for dev
      return;
    }

    IdMap<std::vector<VarPtr>> tmp_vars(call_graph.n);

    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr func = call_graph.functions[i];
      tmp_vars[func] = std::move(dep_datas[i].used_global_vars);

      if (func->is_resumable) {
        call_graph.graph[wait_func].push_back(func);
        call_graph.rev_graph[func].push_back(wait_func);

        call_graph.graph[func].push_back(wait_func);
        call_graph.rev_graph[wait_func].push_back(func);
      }
    }

    MergeBadVarsCallback callback(std::move(tmp_vars));
    MergeReachalbe<FunctionPtr> merge_bad_vars;
    merge_bad_vars.run(call_graph.graph, call_graph.rev_graph, call_graph.functions, &callback);
  }

  static void mark(const IdMap<vector<FunctionPtr>> &graph, IdMap<char> &was, FunctionPtr start, IdMap<FunctionPtr> &parents) {
    was[start] = 1;
    std::queue<FunctionPtr> q;
    q.push(start);
    while (!q.empty()) {
      auto vertex = q.front();
      q.pop();
      for (const auto &next : graph[vertex]) {
        if (!was[next]) {
          parents[next] = vertex;
          was[next] = true;
          q.push(next);
        }
      }
    }
  }

  static void calc_resumable(const FuncCallGraph &call_graph, const vector<DepData> &dep_data) {
    for (int i = 0; i < call_graph.n; i++) {
      for (const auto &fork : dep_data[i].forks) {
        fork->is_resumable = true;
      }
    }
    IdMap<char> from_resumable(call_graph.n); // char to prevent vector<bool> inside
    IdMap<char> into_resumable(call_graph.n);
    IdMap<FunctionPtr> from_parents(call_graph.n);
    IdMap<FunctionPtr> to_parents(call_graph.n);
    for (const auto &func : call_graph.functions) {
      if (func->is_resumable) {
        mark(call_graph.graph, from_resumable, func, from_parents);
        mark(call_graph.rev_graph, into_resumable, func, to_parents);
      }
    }
    for (const auto &func : call_graph.functions) {
      if (from_resumable[func] && into_resumable[func]) {
        func->is_resumable = true;
        func->fork_prev = from_parents[func];
        func->wait_prev = to_parents[func];
      }
    }
    for (const auto &func : call_graph.functions) {
      if (func->is_resumable) {
        if (func->should_be_sync) {
          kphp_error (0, fmt_format("Function [{}] marked with @kphp-sync, but turn up to be resumable\n"
                                    "Function is resumable because of calls chain:\n{}\n", func->name, func->get_resumable_path()));
        }
        if (func->is_inline) {
          func->is_inline = false;
        }
      }
    }
    if (G->settings().print_resumable_graph.get()) {
      for (const auto &func : call_graph.functions) {
        if (!func->is_resumable) {
          continue;
        }
        for (const auto &next : call_graph.graph[func]) {
          if (!next->is_resumable) {
            continue;
          }
          fmt_fprintf(stderr, "{} -> {}\n", func->name, next->name);
        }
      }
    }
  }

  void save_func_dep(FuncCallGraph &call_graph) {
    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr function = call_graph.functions[i];
      function->dep = std::move(call_graph.graph[function]);
    }

    if (!G->settings().is_static_lib_mode()) {
      return;
    }

    auto &main_dep = G->get_main_file()->main_function->dep;
    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr fun = call_graph.functions[i];
      if (fun->kphp_lib_export) {
        main_dep.emplace_back(fun);
      }
    }
    my_unique(&main_dep);
  }

  class MergeRefVarsCallback : public MergeReachalbeCallback<VarPtr> {
  private:
    const IdMap<vector<VarPtr>> &to_merge_;
  public:
    explicit MergeRefVarsCallback(const IdMap<vector<VarPtr>> &to_merge) :
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
    for (const auto &data : dep_datas) {
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
      CheckFunctionsColors(call_graph).check();
      save_func_dep(call_graph);
    }

    generate_ref_vars(dep_datas);
  }
};

void CalcBadVarsF::on_finish(DataStream<FunctionPtr> &os) {
  stage::die_if_global_errors();

  stage::set_name("Calc bad vars (for UB check)");
  auto tmp_vec = tmp_stream.flush_as_vector();
  CalcBadVars{}.run(tmp_vec);
  for (const auto &fun_dep : tmp_vec) {
    os << fun_dep.first;
  }
}
