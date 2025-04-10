// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/tl2cpp/tl-function.h"

#include "compiler/code-gen/files/tl2cpp/tl-combinator.h"
#include "compiler/code-gen/naming.h"

namespace tl2cpp {
void TlFunctionDecl::compile(CodeGenerator& W) const {
  W << "/* ~~~~~~~~~ " << f->name << " ~~~~~~~~~ */\n" << NL;
  const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);

  std::string struct_name = cpp_tl_struct_name("f_", f->name);
  W << "struct " + struct_name + " : tl_func_base " << BEGIN;
  for (const auto& arg : f->args) {
    if (arg->is_optional()) {
      W << "tl_exclamation_fetch_wrapper " << arg->name << ";" << NL;
    } else if (arg->var_num != -1) {
      W << "int64_t " << arg->name << "{0};" << NL;
    }
  }
  if (G->get_untyped_rpc_tl_used()) {
    FunctionSignatureGenerator(W) << "static std::unique_ptr<tl_func_base> store(const mixed &tl_object)" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "mixed fetch()" << SemicolonAndNL();
  } else {
    W << "mixed fetch() noexcept final { return {}; }" << NL;
  }
  if (needs_typed_fetch_store) {
    FunctionSignatureGenerator(W) << "static std::unique_ptr<tl_func_base> typed_store(const " << get_php_runtime_type(f) << " *tl_object)" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "class_instance<" << G->settings().tl_classname_prefix.get() << "RpcFunctionReturnResult> typed_fetch()"
                                  << SemicolonAndNL();
  }
  if (f->is_kphp_rpc_server_function() && needs_typed_fetch_store) {
    check_kphp_function(f);
    FunctionSignatureGenerator(W) << "static std::unique_ptr<tl_func_base> rpc_server_typed_fetch(" << get_php_runtime_type(f) << " *tl_object)"
                                  << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "void rpc_server_typed_store(const class_instance<" << G->settings().tl_classname_prefix.get()
                                  << "RpcFunctionReturnResult> &tl_object_)" << SemicolonAndNL();
  }
  W << END << ";" << NL << NL;
}

void TlFunctionDef::compile(CodeGenerator& W) const {
  const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);
  std::string struct_name = cpp_tl_struct_name("f_", f->name);

  if (G->get_untyped_rpc_tl_used()) {
    FunctionSignatureGenerator(W) << "std::unique_ptr<tl_func_base> " << struct_name << "::store(const mixed& tl_object) " << BEGIN;
    W << "auto tl_func_state = make_unique_on_script_memory<" << struct_name << ">();" << NL;
    W << CombinatorStore(f, CombinatorPart::LEFT, false);
    W << "return std::move(tl_func_state);" << NL;
    W << END << NL << NL;

    FunctionSignatureGenerator(W) << "mixed " << struct_name << "::fetch() " << BEGIN;
    W << CombinatorFetch(f, CombinatorPart::RIGHT, false);
    W << END << NL << NL;
  }
  if (needs_typed_fetch_store) {
    FunctionSignatureGenerator(W) << "std::unique_ptr<tl_func_base> " << struct_name << "::typed_store(const " << get_php_runtime_type(f) << " *tl_object) "
                                  << BEGIN;
    W << "auto tl_func_state = make_unique_on_script_memory<" << struct_name << ">();" << NL;
    W << CombinatorStore(f, CombinatorPart::LEFT, true);
    W << "return std::move(tl_func_state);" << NL;
    W << END << NL << NL;

    FunctionSignatureGenerator(W) << "class_instance<" << G->settings().tl_classname_prefix.get() << "RpcFunctionReturnResult> " << struct_name
                                  << "::typed_fetch() " << BEGIN;
    W << CombinatorFetch(f, CombinatorPart::RIGHT, true);
    W << END << NL << NL;
  }
  if (f->is_kphp_rpc_server_function() && needs_typed_fetch_store) {
    FunctionSignatureGenerator(W) << "std::unique_ptr<tl_func_base> " << struct_name << "::rpc_server_typed_fetch(" << get_php_runtime_type(f)
                                  << " *tl_object) " << BEGIN;
    W << "CurrentTlQuery::get().set_current_tl_function(" << register_tl_const_str(f->name) << ");" << NL;
    W << "auto tl_func_state = make_unique_on_script_memory<" << struct_name << ">();" << NL;
    W << CombinatorFetch(f, CombinatorPart::LEFT, true);
    W << "CurrentTlQuery::get().reset();" << NL;
    W << "return std::move(tl_func_state);" << NL;
    W << END << NL << NL;
    FunctionSignatureGenerator(W) << "void " << struct_name << "::rpc_server_typed_store(const class_instance<" << G->settings().tl_classname_prefix.get()
                                  << "RpcFunctionReturnResult> &tl_object_) " << BEGIN;
    W << "CurrentTlQuery::get().set_current_tl_function(" << register_tl_const_str(f->name) << ");" << NL;
    W << "auto tl_object = tl_object_.template cast_to<" << get_php_runtime_type(f, false) << "_result>().get();" << NL;
    W << CombinatorStore(f, CombinatorPart::RIGHT, true);
    W << "CurrentTlQuery::get().reset();" << NL;
    W << END << NL << NL;
  }
}
} // namespace tl2cpp
