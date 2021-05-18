// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/check-requires.h"

#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"
#include "compiler/function-pass.h"

struct ResolveFunctionsAndConstants final : public FunctionPassBase {
  DataStream<FunctionPtr> &function_stream_;

  ResolveFunctionsAndConstants(FunctionPtr function, DataStream<FunctionPtr> &os):
    function_stream_{os} {
    setup(function);
  }

  string get_description() override {
    return "Resolve function and constant names";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override {
    auto call = v.try_as<op_func_call>();
    if (!call || call->extra_type == op_ex_func_call_arrow) {
      return v;
    }

    auto func_name = resolve_func_name(current_function, call, '\\');
    if (func_name.is_class_member()) {
      // Do nothing.
    } else if (func_name.is_resolved() || current_function->file_id->namespace_name.empty()) {
      call->set_string("\\" + func_name.value);
    } else {
      kphp_assert(!current_function->file_id->namespace_name.empty());
      std::string namespaced_name = current_function->file_id->namespace_name + "\\" + call->get_string();
      if (G->get_function(replace_backslashes(namespaced_name))) {
        call->set_string("\\" + namespaced_name);
      }
    }

    return call;
  }
};

bool CheckRequires::forward_to_next_pipe(const FunctionPtr &f) {
  return !(f->class_id && f->class_id->is_trait());
}

void CheckRequires::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  if (vk::contains(function->name, "test")) {
    printf("execute on %s\n", function->name.c_str());
  }
  ResolveFunctionsAndConstants pass(function, os);
  run_function_pass(function->root->cmd_ref(), &pass);

  os << function;
}

//void CheckRequires::resolve_function_calls(DataStream<FunctionPtr> &os) {
//  auto all = tmp_stream.flush_as_vector();
//
//  for (int id = 0; id < all.size(); ++id) {
//    FunctionPtr f = all[id];
//    if (!f->file_id->namespace_name.empty()) {
//      std::string namespaced_name = f->file_id->namespace_name + "\\" + f->name;
//      if (G->get_function(namespaced_name)) {
//
//      }
//    }
//    os << f;
//  }
//}
