// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <queue>
#include "compiler/pipes/calc-bad-vars.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
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

struct CallstackColor {
  function_palette::color_t color;
  FunctionPtr func;

  CallstackColor(function_palette::color_t color, const FunctionPtr &func)
    : color(color), func(func) {}
};

class CallstackColors {
  using const_it = std::vector<CallstackColor>::const_iterator;
  using it = std::vector<CallstackColor>::iterator;

  std::vector<CallstackColor> data;
  bool only_none_colors{true};

public:
  CallstackColors(size_t cap) {
    data.reserve(cap);
  }

  void push_back(CallstackColor &&color) {
    if (color.color != function_palette::special_colors::none) {
      only_none_colors = false;
    }
    data.push_back(color);
  }

  void pop_back() {
    data.pop_back();
  }

  const_it begin() const { return data.begin(); }
  const_it end() const { return data.end(); }
  it begin() { return data.begin(); }
  it end() { return data.end(); }

  bool empty() const {
    return data.empty();
  }

  bool contains_only_none() const {
    return only_none_colors;
  }
};

struct MatchedRule {
  function_palette::Rule rule{};
  FunctionPtr start_func{nullptr};
  FunctionPtr end_func{nullptr};
  bool matched{false};

  MatchedRule() = default;

  MatchedRule(const function_palette::Rule &rule, const FunctionPtr &start, const FunctionPtr &end)
    : rule(rule), start_func(start), end_func(end), matched(true) {}

  uint64_t hash() const {
    return (start_func->id + end_func->id + 1000) * (rule.id + 1);
  }
};

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
  size_t size() {
    return data.size();
  }

  element &front() {
    return *begin;
  }

  element &back() {
    return *end;
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

  std::string format(const function_palette::Palette& palette) const {
    std::string desc;

    for (auto it = begin; it != end + 1; ++it) {
      const auto frame = *it;
      const auto name = frame->get_human_readable_name() + "()";

      desc += "  ";
      desc += TermStringFormat::paint(name, TermStringFormat::yellow);

      if (!frame->colors.empty()) {
        desc += " with following ";
        desc += frame->colors.to_string(palette);
      }

      if (it != end) {
        desc += "\n\n";
      }
    }

    return desc;
  }

  void reset_narrow() {
    begin = data.begin();
    end = data.end();
  }

  void narrow(const element &from, const element &to) {
    while (begin != data.end()) {
      if (*begin == from) {
        break;
      }
      ++begin;
    }

    while (end != data.begin()) {
      if (*end == to) {
        break;
      }
      --end;
    }
  }
};

class CheckFunctionsColors {
  FuncCallGraph call_graph;
  function_palette::Palette palette;

  std::map<uint64_t, std::pair<FunctionPtr, std::string>> errors;

public:
  CheckFunctionsColors(const FuncCallGraph &call_graph)
    : call_graph(call_graph), palette(G->get_function_palette()) {}

public:
  void check() {
    Callstack callstack(50);
    CallstackColors colors(50);

    for (auto &func : call_graph.functions) {
      if (func->type != FunctionData::func_local) {
        continue;
      }

      const auto called_in = call_graph.rev_graph[func];

      // we only check functions that are not called anywhere other than the main function
      if (called_in.size() > 0 && !called_in[0]->is_main_function()) {
        continue;
      }

      check_func(colors, callstack, func);
    }

    for (const auto &error : errors) {
      stage::set_function(error.second.first);
      kphp_error(0, error.second.second);
    }
  }

  void check_func(CallstackColors &colors, Callstack &callstack, const FunctionPtr &func) {
    if (func->type != FunctionData::func_local) {
      return;
    }

    callstack.push_back(func);
    const auto sep_colors = func->colors.sep_colors();
    for (const auto &sep_color : sep_colors) {
      colors.push_back(CallstackColor(sep_color, func));
    }

    if (sep_colors.empty()) {
      colors.push_back(CallstackColor(function_palette::special_colors::none, func));
    }

    if (callstack.size() > 1 && !colors.empty() && !colors.contains_only_none()) {
      const auto matched_rules = match(colors);

      for (const auto &rule : matched_rules) {
        error(callstack, rule);
      }
    }

    for (const auto &next : call_graph.graph[func]) {
      if (callstack.contains(next)) {
        continue;
      }
      if (callstack.size() >= 50) {
        break;
      }

      check_func(colors, callstack, next);
    }

    for (int i = 0; i < sep_colors.size(); ++i) {
      colors.pop_back();
    }
    callstack.pop_back();
  }

  static function_palette::colors_t calc_mask(const CallstackColors &colors) {
    function_palette::colors_t mask = 0;
    for (const auto &color : colors) {
      mask |= color.color;
    }
    return mask;
  }

  static MatchedRule match_rule(const function_palette::Rule& rule, CallstackColors& colors) {
    const auto match_mask = calc_mask(colors);
    if (!rule.contains_in(match_mask)) {
      return {};
    }

    return match_two_vectors(rule, rule.colors, colors);
  }

  static MatchedRule match_two_vectors(const function_palette::Rule& rule, const std::vector<function_palette::colors_t> &first, CallstackColors& second) {
    if (first.empty() || second.empty()) {
      return {};
    }

    FunctionPtr* end_func = nullptr;

    auto first_it = first.end() - 1;
    auto second_it = second.end() - 1;

    while (true) {
      auto matched = false;

      if (second_it->color == function_palette::special_colors::none) {
        if (*first_it == function_palette::special_colors::any) {
          matched = true;
        } else {
          matched = false;
        }
      } else if (*first_it == function_palette::special_colors::any) {
        matched = true;
      } else if (*first_it == second_it->color) {
        matched = true;
      }

      if (matched) {
        if (end_func == nullptr) {
          end_func = &second_it->func;
        }

        if (first_it == first.begin()) {
          return MatchedRule(rule, second_it->func, *end_func);
        }
        if (second_it == second.begin()) {
          return {};
        }
        first_it--;
        second_it--;
      } else {
        if (second_it == second.begin()) {
          return {};
        }
        second_it--;
      }
    }
  }

  std::vector<MatchedRule> match(CallstackColors &colors) {
    std::vector<MatchedRule> rules;

    for (const auto &group : palette.groups()) {
      for (auto it = group.rules().rbegin(); it != group.rules().rend(); ++it) {
        const auto rule = *it;
        const auto matched_rule = match_rule(rule, colors);

        if (!matched_rule.matched) {
          continue;
        }

        if (errors.count(matched_rule.hash()) != 0) {
          break;
        }

        if (rule.is_error()) {
          rules.push_back(matched_rule);
        }
        break;
      }
    }

    return rules;
  }

  void error(Callstack& callstack, const MatchedRule &rule) {
    callstack.narrow(rule.start_func, rule.end_func);

    const auto hash = rule.hash();
    const auto parent = callstack.front();
    const auto parent_name = callstack.front()->get_human_readable_name() + "()";
    const auto child_name = callstack.back()->get_human_readable_name() + "()";
    const auto callstack_format = callstack.format(palette);

    callstack.reset_narrow();

    const auto description = fmt_format("{} ({} call {})\n\n{}\n\nProduced according to the following rule:\n  {}",
                             rule.rule.error,
                             TermStringFormat::paint_green(parent_name),
                             TermStringFormat::paint_green(child_name),
                             callstack_format,
                             rule.rule.to_string(palette)
    );

    errors.insert(std::make_pair(hash, std::make_pair(parent, std::move(description))));
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

      function->dep_rev = std::move(call_graph.rev_graph[function]);
      auto& dep = function->dep_rev;
      auto it = dep.begin();
      while (it != dep.end()) {
        if ((*it)->is_main_function() || (*it)->is_extern()) {
          it = dep.erase(it);
          continue;
        }

        ++it;
      }
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
