// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-bad-vars.h"

#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/kphp_assert.h"
#include "compiler/pipes/calc-func-dep.h"
#include "compiler/utils/idmap.h"

/*** Common algorithm ***/
// Graph G
// Each node have Data
// Data have to be merged with all descendant's Data.
template<class VertexT>
class MergeReachalbeCallback {
public:
  virtual void for_component(const std::vector<VertexT> &component, const std::vector<VertexT> &edges) = 0;

  virtual ~MergeReachalbeCallback() {}
};

template<class VertexT>
class MergeReachalbe {
private:
  using GraphT = IdMap<std::vector<VertexT>>;

  void dfs(const VertexT &vertex, const GraphT &graph, IdMap<int> *was, std::vector<VertexT> *topsorted) {
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
                     std::vector<int> *was_color, std::vector<VertexT> *component, std::vector<VertexT> *edges) {
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
  void run(const GraphT &graph, const GraphT &rev_graph, const std::vector<VertexT> &vertices,
           MergeReachalbeCallback<VertexT> *callback) {
    int vertex_n = (int)vertices.size();
    IdMap<int> was(vertex_n);
    std::vector<VertexT> topsorted;
    topsorted.reserve(vertex_n);
    for (const auto &vertex : vertices) {
      dfs(vertex, rev_graph, &was, &topsorted);
    }

    std::fill(was.begin(), was.end(), 0);
    std::vector<int> was_color(vertex_n + 1, 0);
    int current_color = 0;
    for (auto vertex_it = topsorted.rbegin(); vertex_it != topsorted.rend(); ++vertex_it) {
      auto &vertex = *vertex_it;
      if (was[vertex]) {
        continue;
      }
      std::vector<VertexT> component;
      std::vector<VertexT> edges;
      dfs_component(vertex, graph, ++current_color, &was,
                    &was_color, &component, &edges);

      callback->for_component(component, edges);
    }
  }
};

struct FuncCallGraph {
  int n;
  std::vector<FunctionPtr> functions;
  IdMap<std::vector<FunctionPtr>> graph;
  IdMap<std::vector<FunctionPtr>> rev_graph;
  std::vector<std::pair<FunctionPtr, std::vector<FunctionPtr>>> temporarily_removed;

  FuncCallGraph(std::vector<FunctionPtr> other_functions, std::vector<DepData> &dep_datas) :
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

  void remove(FunctionPtr to_remove) {
    for (FunctionPtr caller : rev_graph[to_remove]) {
      graph[caller].erase(std::find(graph[caller].begin(), graph[caller].end(), to_remove));
    }
    temporarily_removed.emplace_back(to_remove, std::move(rev_graph[to_remove]));
  };

  void restore_all_removed() {
    for (auto &pair : temporarily_removed) {
      for (FunctionPtr caller : pair.second) {
        graph[caller].emplace_back(pair.first);
      }
      rev_graph[pair.first] = std::move(pair.second);
    }
    temporarily_removed.clear();
  };
};

class CallstackOfColoredFunctions {
  std::vector<FunctionPtr> stack;                       // functions placed in order, only colored functions
  std::vector<function_palette::color_t> colors_chain;  // blended colors of stacked functions, one-by-one
  std::unordered_set<FunctionPtr> index_set;            // a quick index for contains(), has the same elements as stack
  function_palette::colors_mask_t colors_mask{0};       // mask of all colors_chain

  void recalc_mask() {
    colors_mask = 0;
    for (function_palette::color_t c : colors_chain) {
      colors_mask |= c;
    }
  }

public:
  CallstackOfColoredFunctions() {
    stack.reserve(10);
    index_set.reserve(10);
  }

  size_t size() const {
    return stack.size();
  }

  const std::vector<FunctionPtr> &as_vector() const {
    return stack;
  }

  const std::vector<function_palette::color_t> &get_colors_chain() const {
    return colors_chain;
  }

  function_palette::colors_mask_t get_colors_mask() const {
    return colors_mask;
  }

  void push_back(const FunctionPtr &el) {
    stack.push_back(el);
    index_set.insert(el);
    for (const auto &sep_color : el->colors) {
      colors_chain.push_back(sep_color);
    }
    recalc_mask();
  }

  void pop_back() {
    FunctionPtr pop = stack.back();
    index_set.erase(pop);
    stack.pop_back();
    for (const auto &sep_color __attribute__((unused)) : pop->colors) {
      colors_chain.pop_back();
    }
    recalc_mask();
  }

  bool contains(const FunctionPtr &el) const {
    return index_set.find(el) != index_set.end();
  }
};

// here we check all palette rules like "api has-curl" or "messages-module messages-internals"
// start from the root, make dfs expanding next_with_colors and check all rules for each dfs chain
// these chains are left-to-right, e.g. colored f1->f2->f3, we find and check f1, then f1->f2, then f1->f2->f3
class CheckFunctionsColors {
  static constexpr int max_shown_errors_count = 10;

  const FuncCallGraph &call_graph;
  const function_palette::Palette &palette;
  std::unordered_set<std::string> shown_errors;   // the easiest way to avoid duplicated errors

public:
  explicit CheckFunctionsColors(const FuncCallGraph &call_graph)
    : call_graph(call_graph)
    , palette(G->get_function_palette()) {}

  void check_func_dfs(CallstackOfColoredFunctions &callstack, const FunctionPtr &func) {
    callstack.push_back(func);
    bool was_any_error = false;

    for (const auto &ruleset : palette.get_rulesets()) {
      for (auto it = ruleset.rbegin(); it != ruleset.rend(); ++it) {  // reverse is important
        const function_palette::PaletteRule &rule = *it;
        if (!match_rule(callstack, rule)) {
          continue;
        }

        if (rule.is_error()) {
          show_error_on_rule_broken(callstack, rule);
          was_any_error = true;
        }
        break;
      }
    }

    if (!was_any_error && func->next_with_colors != nullptr) {
      for (FunctionPtr next : *func->next_with_colors) {
        if (callstack.size() < 50 && !callstack.contains(next)) {
          check_func_dfs(callstack, next);
        }
      }
    }
    callstack.pop_back();
  }

  static bool match_rule(const CallstackOfColoredFunctions &callstack, const function_palette::PaletteRule &rule) {
    const auto match_mask = callstack.get_colors_mask();
    if (match_mask == 0 || !rule.contains_in(match_mask)) {
      return false;
    }

    return match_two_vectors(rule.colors, callstack.get_colors_chain());
  }

  static bool match_two_vectors(const std::vector<function_palette::colors_mask_t> &rule_chain, const std::vector<function_palette::color_t> &actual_chain) {
    auto rule_it = rule_chain.end() - 1;
    auto actual_it = actual_chain.end() - 1;

    while (true) {
      bool rightmost_matched = *rule_it == *actual_it;

      if (rightmost_matched) {
        if (rule_it == rule_chain.begin()) {
          return true;
        }
        if (actual_it == actual_chain.begin()) {
          return false;
        }
        --rule_it;
        --actual_it;
      } else {
        if (actual_it == actual_chain.begin()) {
          return false;
        }
        --actual_it;
      }
    }
  }

  // on error (colored chain breaks some rule), we want to find an actual chain of calling
  // this is launched only on error, that's why we don't care about performance and just use bfs
  // todo think about using such approach for fork chains, throws chains, etc (and drop fork_prev, wait_prev, throws_reason from FunctionData)
  std::vector<FunctionPtr> find_callstack_between_two_functions_bfs(FunctionPtr from, FunctionPtr target, const std::unordered_set<FunctionPtr> &shouldnt_appear) {
    std::unordered_map<FunctionPtr, int> visited_level;
    std::queue<FunctionPtr> bfs_queue;

    bfs_queue.push(from);
    visited_level[from] = 0;

    while (!bfs_queue.empty()) {
      FunctionPtr cur = bfs_queue.front();
      bfs_queue.pop();
      int next_level = visited_level[cur] + 1;

      if (cur == target) {
        break;
      }

      for (FunctionPtr called : call_graph.graph[cur]) {
        if (shouldnt_appear.find(called) == shouldnt_appear.end() && visited_level.emplace(called, next_level).second) {
          bfs_queue.push(called);
        }
      }
    }

    if (visited_level.find(target) == visited_level.end()) {    // couldn't find, just return [from, target]
      return {from, target};
    }

    std::vector<FunctionPtr> callstack;
    callstack.emplace_back(target);
    for (FunctionPtr cur = target; cur != from;) {
      int prev_level = visited_level[cur] - 1;
      for (FunctionPtr callee : call_graph.rev_graph[cur]) {
        auto found = visited_level.find(callee);
        if (found != visited_level.end() && found->second == prev_level) {
          callstack.push_back(callee);
          cur = callee;
          break;
        }
      }
      kphp_assert(visited_level.find(cur)->second == prev_level);
    }

    std::reverse(callstack.begin(), callstack.end());
    return callstack;
  }

  void show_error_on_rule_broken(const CallstackOfColoredFunctions &color_callstack, const function_palette::PaletteRule &rule) {
    kphp_assert(rule.is_error());
    if (shown_errors.size() > max_shown_errors_count) {
      return;
    }

    auto vector = color_callstack.as_vector();    // f1, f2, f3 — all of them are colored, and their chain breaks the rule
    std::vector<FunctionPtr> full_callstack;      // will be: src_main -> ... -> f1 -> ... -> f2 -> ... -> f3
    for (int i = 0; i < vector.size() - 1; ++i) {
      FunctionPtr cur = vector[i], next = vector[i+1];
      std::unordered_set<FunctionPtr> shouldnt_appear;
      kphp_assert(cur->next_with_colors);
      for (FunctionPtr exclude : *cur->next_with_colors) {
        if (exclude != next && exclude != cur) {
          shouldnt_appear.insert(exclude);
        }
      }
      auto next_callstack_part = find_callstack_between_two_functions_bfs(cur, next, shouldnt_appear);
      full_callstack.insert(full_callstack.end(), next_callstack_part.begin(), next_callstack_part.end() - 1);
    }
    full_callstack.emplace_back(vector.back());

    // having full callstack like "src_main -> main -> init -> apiFn@api -> .... -> curlFn@curl"
    // we want to show a slice only containing a subchain that breaks the rule
    auto first_item_to_show = full_callstack.begin();
    while (!(*first_item_to_show)->colors.contains(rule.colors.front())) {
      ++first_item_to_show;
    }
    auto last_item_to_show = full_callstack.end() - 1;
    while (!(*last_item_to_show)->colors.contains(rule.colors.back())) {
      --last_item_to_show;
    }
    if (first_item_to_show == last_item_to_show) {
      first_item_to_show = full_callstack.begin();
    }

    std::vector<FunctionPtr> call_chain_to_show(first_item_to_show, last_item_to_show + 1);
    std::string callstack_str = vk::join(call_chain_to_show, " -> ", [&](FunctionPtr f) {
      return f->as_human_readable() + TermStringFormat::paint(f->colors.to_string(palette, rule.mask), TermStringFormat::cyan);
    });

    stage::set_location((*first_item_to_show)->root->location);
    kphp_error(!shown_errors.insert(callstack_str).second,
               fmt_format("{} => {}\n  This color rule is broken, call chain:\n{}",
                          TermStringFormat::paint(rule.as_human_readable(palette), TermStringFormat::cyan),
                          TermStringFormat::paint_red(rule.error),
                          callstack_str));
  }
};

class CalcBadVars {
  class MergeBadVarsCallback : public MergeReachalbeCallback<FunctionPtr> {
    IdMap<std::vector<VarPtr>> modified_global_vars;
  public:
    explicit MergeBadVarsCallback(IdMap<std::vector<VarPtr>> &&modified_global_vars) :
      modified_global_vars(std::move(modified_global_vars)) {}

    // here we calculate "bad vars" for a function — they will be used in check-ub.cpp
    // "bad vars" are modified non-primitive globals, that can lead to a potential ub when modified and accessed in subcalls
    // this method just unions all edges' bad_vars to the current function
    // this works in a single thread and uses some heuristic for speedup, but generally it means
    // "for f in component { for e in edge { f.bad_vars += e.bad_vars } }"
    void for_component(const std::vector<FunctionPtr> &component, const std::vector<FunctionPtr> &edges) override {
      // optimization 1: lots of functions don't modify globals at all
      // then we leave f->bad_vars nullptr
      bool empty = vk::all_of(component, [&](FunctionPtr f) { return modified_global_vars[f].empty(); })
                && vk::all_of(edges, [&](FunctionPtr e) { return e->bad_vars == nullptr; });
      if (empty) {
        return;
      }

      // optimization 2: maybe, one of the edges has bad_vars that includes all other edges
      // then we just assign f->bad_vars = largest_edge->bad_vars
      std::unordered_set<VarPtr> *largest_e{nullptr};
      for (FunctionPtr e : edges) {
        if (e->bad_vars != nullptr && (!largest_e || e->bad_vars->size() > largest_e->size())) {
          largest_e = e->bad_vars;
        }
      }
      if (largest_e) {
        bool all_in_largest_e = vk::all_of(component, [&](FunctionPtr f) {
          return vk::all_of(modified_global_vars[f], [&](VarPtr v_in_self) {
            return largest_e->find(v_in_self) != largest_e->end();
          });
        });
        all_in_largest_e &= vk::all_of(edges, [&](FunctionPtr e) {
          return e->bad_vars == nullptr || e->bad_vars == largest_e || vk::all_of(*e->bad_vars, [&](VarPtr v_in_e) {
            return largest_e->find(v_in_e) != largest_e->end();
          });
        });
        if (all_in_largest_e) {
          for (FunctionPtr f : component) {
            f->bad_vars = largest_e;
          }
          return;
        }
      }

      // if not, allocate and fill the set which unites all edges
      // optimization 3: start from largest_e and unite all others into result
      auto *bad_vars = largest_e ? new std::unordered_set<VarPtr>(*largest_e) : new std::unordered_set<VarPtr>();

      for (FunctionPtr f : component) {
        bad_vars->insert(modified_global_vars[f].begin(), modified_global_vars[f].end());
      }
      for (FunctionPtr f : edges) {
        if (f->bad_vars != nullptr && f->bad_vars != largest_e) {
          bad_vars->insert(f->bad_vars->begin(), f->bad_vars->end());
        }
      }
      kphp_assert(!bad_vars->empty());

      for (FunctionPtr f : component) {
        f->bad_vars = bad_vars;
      }
    }
  };

  class MergeNextColoredCallback : public MergeReachalbeCallback<FunctionPtr> {

    // here we calculate next_with_colors for every function
    // we'll use it for perform dfs only for colored nodes of a call graph
    // (this precalculation is needed, because dfs for a whole call graph is too heavy on a large code base)
    void for_component(const std::vector<FunctionPtr> &component, const std::vector<FunctionPtr> &edges) override {
      // (this can be optimized if many functions are marked with colors, for now left simple to me more understandable)
      std::unordered_set<FunctionPtr> next_colored_uniq;

      // if an edge is colored, append this edge
      // if not, append next_with_colors from this edge
      for (FunctionPtr f: edges) {
        if (!f->colors.empty()) {
          next_colored_uniq.insert(f);
        } else if (f->next_with_colors != nullptr) {
          next_colored_uniq.insert(f->next_with_colors->begin(), f->next_with_colors->end());
        }
      }

      // for a recursive bunch of functions (e.g. f1 -> f2 -> f1)
      // assume that "every functions is reachable from any of them" — so, append all colored functions from a recurse
      // this is not true in all cases, but enough for practical usage
      // also, to reduce a number of permutations for next colored path searching, choose only one representative of each color
      bool has_colors_in_component = std::any_of(component.begin(), component.end(), [](FunctionPtr f) { return !f->colors.empty(); });
      if (has_colors_in_component && component.size() > 1) {
        function_palette::colors_mask_t added{0};
        for (FunctionPtr f : component) {
          for (function_palette::color_t c : f->colors) {
            if (!(added & c)) {
              added |= c;
              next_colored_uniq.insert(f);
            }
          }
        }
      }

      if (!next_colored_uniq.empty()) {
        auto *next_with_colors = new std::vector<FunctionPtr>(next_colored_uniq.begin(), next_colored_uniq.end());
        for (FunctionPtr f : component) {
          f->next_with_colors = next_with_colors;
        }
      }
    }
  };

  void generate_bad_vars(const FuncCallGraph &call_graph, std::vector<DepData> &dep_datas) {
    IdMap<std::vector<VarPtr>> modified_global_vars(call_graph.n);

    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr func = call_graph.functions[i];
      modified_global_vars[func] = std::move(dep_datas[i].modified_global_vars);
    }

    MergeBadVarsCallback callback(std::move(modified_global_vars));
    MergeReachalbe<FunctionPtr> merge_bad_vars;
    merge_bad_vars.run(call_graph.graph, call_graph.rev_graph, call_graph.functions, &callback);
  }

  void check_func_colors(FuncCallGraph &call_graph) {
    // to perform calculating next_with_colors and checking color rules from the palette,
    // we need to drop '@kphp-color remover' functions from a call graph completely, like they don't exist at all
    // this special color is for manual cutting connectivity rules, allowing to explicitly separate recursively-joint components
    for (FunctionPtr f : call_graph.functions) {
      if (!f->colors.empty() && f->colors.contains(function_palette::special_color_remover)) {
        call_graph.remove(f);
      }
    }

    MergeNextColoredCallback callback;
    MergeReachalbe<FunctionPtr> merge_next_colored;
    merge_next_colored.run(call_graph.graph, call_graph.rev_graph, call_graph.functions, &callback);
    CallstackOfColoredFunctions callstack;
    CheckFunctionsColors(call_graph).check_func_dfs(callstack, G->get_main_file()->main_function);
    call_graph.restore_all_removed();
  }

  static void mark(const IdMap<std::vector<FunctionPtr>> &graph, IdMap<char> &was, FunctionPtr start, IdMap<FunctionPtr> &parents) {
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

  static void calc_interruptible(const FuncCallGraph &call_graph) {
    IdMap<char> into_interruptible(call_graph.n);
    IdMap<FunctionPtr> to_parents(call_graph.n);

    for (const auto &func : call_graph.functions) {
      if (func->is_interruptible) {
        mark(call_graph.rev_graph, into_interruptible, func, to_parents);
      }
    }

    for (const auto &func : call_graph.functions) {
      if (into_interruptible[func]) {
        func->is_interruptible = true;
        if (unlikely(func->class_id && func->class_id == G->get_class("ArrayAccess"))) {
          kphp_error(false, fmt_format("Function [{}] is a method of ArrayAccess, it cannot call interruptible functions\n"
                                       "Function transitively calls the interruptible function along the following chain:\n{}\n",
                                       func->as_human_readable(), get_call_path(func, to_parents)));
        }
      }
    }
  }

  static void calc_k2_fork(const FuncCallGraph& call_graph, const std::vector<DepData>& dep_data) {
    for (int i = 0; i < call_graph.n; ++i) {
      for (const auto& fork : dep_data[i].forks) {
        fork->is_interruptible = true;
        if (!std::exchange(fork->is_k2_fork, true)) { // check only once
          for (VarPtr param : fork->param_ids) {
            kphp_error(!param->is_reference, fmt_format("Function '{}' cannot be forked since it has a reference parameter '{}'\n", fork->as_human_readable(),
                                                        param->as_human_readable()));
          }
        }
      }
    }
  }

  static void calc_resumable(const FuncCallGraph &call_graph, const std::vector<DepData> &dep_data) {
    for (int i = 0; i < call_graph.n; i++) {
      for (const auto &fork : dep_data[i].forks) {
        fork->is_resumable = true;
      }
    }
    IdMap<char> from_resumable(call_graph.n); // char to prevent std::vector<bool> inside
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
      func->can_be_implicitly_interrupted_by_other_resumable = into_resumable[func];
      if (from_resumable[func] && into_resumable[func]) {
        func->is_resumable = true;
        func->fork_prev = from_parents[func];
        func->wait_prev = to_parents[func];
      }
    }

    for (const auto &func : call_graph.functions) {
      if (func->class_id && func->class_id == G->get_class("ArrayAccess") && func->can_be_implicitly_interrupted_by_other_resumable) {
        kphp_error(false, fmt_format("Function [{}] is a method of ArrayAccess, it cannot call resumable functions\n"
                                     "Function transitively calls the resumable function along the following chain:\n{}\n",
                                     func->as_human_readable(), get_call_path(func, to_parents)));
      }
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

  void save_func_dep(const FuncCallGraph &call_graph) {
    for (int i = 0; i < call_graph.n; i++) {
      FunctionPtr function = call_graph.functions[i];
      function->dep = std::move(call_graph.graph[function]);
    }

    if (!G->is_output_mode_lib() && !G->is_output_mode_k2_lib()) {
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

  static std::string get_call_path(FunctionPtr func, const IdMap<FunctionPtr> &to_parents) {
    kphp_assert_msg(func, "Unexpected null FunctionPtr in getting call-chain");
    std::vector<std::string> names;
    names.push_back(TermStringFormat::paint(func->as_human_readable(), TermStringFormat::red));

    func = to_parents[func];
    while (func) {
      names.push_back(func->as_human_readable());
      func = to_parents[func];
    }
    return vk::join(names, " -> ");
  };

  class MergeRefVarsCallback : public MergeReachalbeCallback<VarPtr> {
  private:
    const IdMap<std::vector<VarPtr>> &to_merge_;
  public:
    explicit MergeRefVarsCallback(const IdMap<std::vector<VarPtr>> &to_merge) :
      to_merge_(to_merge) {
    }

    void for_component(const std::vector<VarPtr> &component, const std::vector<VarPtr> &edges) {
      auto *res = new std::unordered_set<VarPtr>();
      for (const auto &var : component) {
        res->insert(to_merge_[var].begin(), to_merge_[var].end());
      }
      for (const auto &var : edges) {
        if (var->bad_vars != nullptr) {
          res->insert(var->bad_vars->begin(), var->bad_vars->end());
        }
      }
      if (res->empty()) {
        delete res;
        return;
      }
      for (const auto &var : component) {
        var->bad_vars = res;
      }
    }
  };

  void generate_ref_vars(const std::vector<DepData> &dep_datas) {
    std::vector<VarPtr> vars;
    for (const auto &data : dep_datas) {
      vars.insert(vars.end(), data.ref_param_vars.begin(), data.ref_param_vars.end());
    }
    int vars_n = static_cast<int>(vars.size());
    for (int i = 0; i < vars_n; ++i) {
      set_index(vars[i], i);
    }

    IdMap<std::vector<VarPtr>> rev_graph(vars_n), graph(vars_n), ref_vars(vars_n);
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
      if (G->is_output_mode_k2()) {
        calc_k2_fork(call_graph, dep_datas);
        calc_interruptible(call_graph);
      } else {
        calc_resumable(call_graph, dep_datas);
      }
      generate_bad_vars(call_graph, dep_datas);
      check_func_colors(call_graph);
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
