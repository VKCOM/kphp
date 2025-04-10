// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/early-optimization.h"

#include "auto/compiler/rewrite-rules/early_opt.h"
#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/function-pass.h"
#include "compiler/vertex-util.h"

// EarlyOptimizationPass applies optimizations by rewriting the AST.
// It's called "early" because it happens before the type inference.
// The optimizations can take various forms: improved type information
// via specialized calls, reduced allocations via simplified expressions, etc.
//
// Some optimizations are done manually, some of them are performed by
// the rewrite rules. It's an implementation detail, and it should not be
// of any interest to the compiler pipeline.
//
// If you want to add a new pre-typeinfer optimization, see the
// rewrite rules documentation to see whether you should do it using our DSL or not.
class EarlyOptimizationPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Early compile-time optimizations";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override {
    if (root->type() == op_list) {
      return on_list(root.as<op_list>());
    }
    if (root->type() == op_func_call) {
      return on_func_call(root.as<op_func_call>());
    }
    return root;
  }

private:
  VertexPtr on_list(VertexAdaptor<op_list> root) {
    auto rhs = root->array().try_as<op_func_call>();
    if (!rhs || rhs->get_string() != std::string_view{"explode"} || vk::none_of_equal(rhs->args().size(), 2, 3)) {
      return root;
    }
    // keys could have gaps (when list lhs has empty elements)
    // or be unsorted (when explicit keys are used),
    // so we can't get away by just using root->list().size()
    int64_t max_index = 0;
    int64_t mask = 0;
    for (auto list_elem : root->list()) {
      auto kv = list_elem.as<op_list_keyval>();
      auto int_key_vertex = kv->key().try_as<op_int_const>();
      if (!int_key_vertex) {
        return root; // could be a string key, give up
      }
      int64_t int_key = parse_int_from_string(int_key_vertex);
      mask |= (1 << int_key);
      max_index = std::max(max_index, int_key);
    }
    if (max_index >= 4 || max_index <= 0) {
      return root; // we don't have specializations for 5 or more tuple elems
    }
    std::vector<VertexPtr> new_call_args;
    new_call_args.reserve(rhs->args().size() + 1);
    new_call_args.push_back(rhs->args()[0]);                                        // delimiter
    new_call_args.push_back(rhs->args()[1]);                                        // string
    new_call_args.push_back(VertexUtil::create_int_const(mask).set_location(root)); // result mask
    if (rhs->args().size() == 3) {
      new_call_args.push_back(rhs->args()[2]); // limit
    }
    auto new_call = VertexAdaptor<op_func_call>::create(new_call_args).set_location(root);
    new_call->set_string("_explode_tuple" + std::to_string(max_index + 1));
    new_call->func_id = G->get_function(new_call->str_val);
    new_call->auto_inserted = true;
    root->array() = new_call;
    return root;
  }

  VertexPtr on_func_call(VertexAdaptor<op_func_call> root) {
    return insert_tmp_string(root);
  }

  VertexPtr insert_tmp_string(VertexAdaptor<op_func_call> root) {
    // for every f(substr(...)) replace substr(...) with _tmp_substr(...)
    // if f() marks its associated parameter with readonly attribute

    FunctionPtr fn = root->func_id;

    // fast path: skip functions that have no readonly params
    if (!fn || fn->readonly_param_index == -1) {
      return root;
    }
    if (root->args().size() <= fn->readonly_param_index) {
      return root;
    }
    auto v = VertexUtil::unwrap_string_value(root->args()[fn->readonly_param_index]);
    auto substr_arg = v.try_as<op_func_call>();
    if (!substr_arg) {
      return root;
    }
    vk::string_view specialization_func = substr_arg->func_id->get_tmp_string_specialization();
    if (specialization_func.empty()) {
      return root;
    }
    substr_arg->str_val = specialization_func;
    substr_arg->func_id = G->get_function(substr_arg->str_val);
    substr_arg->auto_inserted = true;
    root->args()[fn->readonly_param_index] = substr_arg;
    return root;
  }
};

void EarlyOptimizationF::execute(FunctionPtr f, DataStream<FunctionPtr>& os) {
  if (!f->is_extern()) {
    run_early_opt_rules_pass(f);

    EarlyOptimizationPass pass{};
    run_function_pass(f, &pass);
  }

  os << f;
}
