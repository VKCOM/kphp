// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/tracing-autogen.h"

#include <algorithm>
#include <set>

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/kphp-tracing-tags.h"


std::vector<FunctionPtr> TracingAutogen::all_with_aggregate;
std::vector<vk::string_view> TracingAutogen::all_aggregate_names;
std::mutex TracingAutogen::mu;


// from BuiltinFuncID in runtime
constexpr int _shift_for_branch = 16;
constexpr int _shift_for_aggregate = 18;

void TracingAutogen::compile(CodeGenerator &W) const {
  W << OpenFile{"_tracing_autogen.cpp"};
  W << ExternInclude{G->settings().runtime_headers.get()};

  auto add_value = [&W](int func_id, vk::string_view title, vk::string_view comment) {
    W << fmt_format("out.set_value({}, string(R\"({})\", {})); // {}", func_id, title, title.size(), comment) << NL;
  };

  W << "void kphp_tracing_autogen_func_call_strings(array<string> &out) noexcept " << BEGIN;

  W << "// user-defined functions with @kphp-tracing aggregate" << NL;
  int f_idx = 1;
  for (FunctionPtr f : all_with_aggregate) {
    add_value(f_idx, f->kphp_tracing->span_title, f->name);
    if (!f->kphp_tracing->branches.empty()) {
      for (const auto &[branch_num, cond_title] : f->kphp_tracing->branches) {
        add_value((branch_num << _shift_for_branch) + f_idx, cond_title, std::to_string(branch_num));
      }
    }
    f_idx++;
  }

  W << NL << "// aggregate names" << NL;
  int agg_idx = 1;
  for (vk::string_view agg_name : all_aggregate_names) {
    add_value((agg_idx << _shift_for_aggregate), agg_name, std::to_string(agg_idx));
    agg_idx++;
  }

  W << END << NL;
  W << CloseFile{};
}

void TracingAutogen::register_function_marked_kphp_tracing(FunctionPtr f) {
  if (f->kphp_tracing->is_aggregate()) {
    std::lock_guard<std::mutex> lock{mu};
    all_with_aggregate.emplace_back(f);
  }
}

void TracingAutogen::codegen_runtime_func_guard_declaration(CodeGenerator &W, FunctionPtr f) {
  kphp_assert(f->kphp_tracing);

  if (!f->kphp_tracing->is_aggregate()) {
    W << "KphpTracingFuncCallGuard _tr_f;" << NL;
  } else {
    W << "KphpTracingAggregateGuard _tr_f;" << NL;
  }
}

void TracingAutogen::codegen_runtime_func_guard_start(CodeGenerator &W, FunctionPtr f) {
  if (!f->kphp_tracing->is_aggregate()) {
    std::string title = replace_characters(f->kphp_tracing->span_title, '\\', '/');
    W << "_tr_f.start(\"" << title << "\"," << title.size() << "," << f->kphp_tracing->level << ");" << NL;
    return;
  }

  auto it = std::lower_bound(all_with_aggregate.begin(), all_with_aggregate.end(), f,
                             [](FunctionPtr a, FunctionPtr b) { return a->name.compare(b->name) < 0; });
  int func_id = it == all_with_aggregate.end() ? 0 : it - all_with_aggregate.begin() + 1;

  auto it_agg = std::lower_bound(all_aggregate_names.begin(), all_aggregate_names.end(), f->kphp_tracing->aggregate_name);
  int agg_id = it_agg == all_aggregate_names.end() ? 0 : it_agg - all_aggregate_names.begin() + 1;
  W << "_tr_f.start(" << func_id << " | " << (agg_id << _shift_for_aggregate) << ");";
  W << " // " << f->kphp_tracing->span_title << " | " << f->kphp_tracing->aggregate_name << NL;
}

void TracingAutogen::finished_appending_and_prepare() {
  std::sort(all_with_aggregate.begin(), all_with_aggregate.end(),
            [](FunctionPtr a, FunctionPtr b) { return a->name.compare(b->name) < 0; });

  std::set<vk::string_view> uniq_aggregates_sorted;
  for (FunctionPtr f : all_with_aggregate) {
    uniq_aggregates_sorted.insert(f->kphp_tracing->aggregate_name);
  }

  all_aggregate_names.reserve(uniq_aggregates_sorted.size());
  for (vk::string_view agg_name : uniq_aggregates_sorted) {
    all_aggregate_names.emplace_back(agg_name);
  }
}
