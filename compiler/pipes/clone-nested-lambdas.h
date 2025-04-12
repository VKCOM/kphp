// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/compiler-core.h"
#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"
#include "compiler/name-gen.h"
#include "compiler/vertex.h"

// note! this pipe is NOT a part of the compilation pipeline:
// instead, it's launched manually, after function cloning, to clone nested lambdas as separate functions also

// This pass takes functions that were cloned from existing ones (base$$fname$$context, trait methods, etc).
// Every op_lambda inside must point to a new function instead of an old one.
class CloneNestedLambdasPass final : public FunctionPassBase {
  DataStream<FunctionPtr> *function_stream;

public:
  std::string get_description() override {
    return "Clone nested lambdas";
  }

  explicit CloneNestedLambdasPass(DataStream<FunctionPtr> *function_stream) :
    function_stream(function_stream) {}

  VertexPtr on_enter_vertex(VertexPtr root) override {
    if (auto as_op_lambda = root.try_as<op_lambda>()) {
      FunctionPtr cur_f_lambda = as_op_lambda->func_id;
      std::string new_func_name = gen_anonymous_function_name(current_function);

      FunctionPtr new_f_lambda = FunctionData::clone_from(cur_f_lambda, new_func_name);
      new_f_lambda->outer_function = current_function;
      as_op_lambda->func_id = new_f_lambda;
      if (function_stream) {
        G->register_and_require_function(new_f_lambda, *function_stream, true);
      }

      CloneNestedLambdasPass::run_if_lambdas_inside(new_f_lambda, function_stream);
    }

    return root;
  }

  static inline void run_if_lambdas_inside(FunctionPtr cloned_function, DataStream<FunctionPtr> *function_stream) {
    if (cloned_function->has_lambdas_inside) {
      CloneNestedLambdasPass pass(function_stream);
      run_function_pass(cloned_function, &pass);
    }
  }
};
