#include "compiler/code-gen/files/tl2cpp/tl-function.h"

#include "compiler/code-gen/files/tl2cpp/tl-combinator.h"

namespace tl2cpp {
void TlFunctionDecl::compile(CodeGenerator &W) const {
  W << "/* ~~~~~~~~~ " << f->name << " ~~~~~~~~~ */\n" << NL;
  const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);

  std::string struct_name = cpp_tl_struct_name("f_", f->name);
  W << "struct " + struct_name + " : tl_func_base " << BEGIN;
  for (const auto &arg : f->args) {
    if (arg->is_optional()) {
      W << "tl_exclamation_fetch_wrapper " << arg->name << ";" << NL;
    } else if (arg->var_num != -1) {
      W << "int " << arg->name << "{0};" << NL;
    }
  }
  W << "static std::unique_ptr<tl_func_base> store(const var &tl_object);" << NL;
  W << "var fetch();" << NL;
  if (needs_typed_fetch_store) {
    W << "static std::unique_ptr<tl_func_base> typed_store(const " << get_php_runtime_type(f) << " *tl_object);" << NL;
    W << "class_instance<" << G->env().get_tl_classname_prefix() << "RpcFunctionReturnResult> typed_fetch();" << NL;
  }
  if (f->is_kphp_rpc_server_function()) {
    if (needs_typed_fetch_store) {
      check_kphp_function(f);
      W << "static std::unique_ptr<tl_func_base> rpc_server_typed_fetch(" << get_php_runtime_type(f) << " *tl_object);" << NL;
      W << "void rpc_server_typed_store(const class_instance<" << G->env().get_tl_classname_prefix() << "RpcFunctionReturnResult> &tl_object_);" << NL;
    } else if (G->server_tl_serialization_used) {
      kphp_warning(fmt_format("TL function {} is marked @kphp, but typed RPC code for it is not found. Automatic serialization in server mode won't work\n"
                              "(e.g. rpc_server_fetch_request() and rpc_server_store_response() functions)", f->name));
    }
  }
  W << END << ";" << NL << NL;
}

void TlFunctionDef::compile(CodeGenerator &W) const {
  const bool needs_typed_fetch_store = TlFunctionDecl::does_tl_function_need_typed_fetch_store(f);
  std::string struct_name = cpp_tl_struct_name("f_", f->name);

  W << "std::unique_ptr<tl_func_base> " << struct_name << "::store(const var& tl_object) " << BEGIN;
  W << "auto tl_func_state = make_unique_on_script_memory<" << struct_name << ">();" << NL;
  W << CombinatorStore(f, CombinatorPart::LEFT, false);
  W << "return std::move(tl_func_state);" << NL;
  W << END << NL << NL;

  W << "var " << struct_name << "::fetch() " << BEGIN;
  W << CombinatorFetch(f, CombinatorPart::RIGHT, false);
  W << END << NL << NL;
  if (needs_typed_fetch_store) {
    W << "std::unique_ptr<tl_func_base> " << struct_name << "::typed_store(const " << get_php_runtime_type(f) << " *tl_object) " << BEGIN;
    W << "auto tl_func_state = make_unique_on_script_memory<" << struct_name << ">();" << NL;
    W << CombinatorStore(f, CombinatorPart::LEFT, true);
    W << "return std::move(tl_func_state);" << NL;
    W << END << NL << NL;

    W << "class_instance<" << G->env().get_tl_classname_prefix() << "RpcFunctionReturnResult> " << struct_name << "::typed_fetch() " << BEGIN;
    W << CombinatorFetch(f, CombinatorPart::RIGHT, true);
    W << END << NL << NL;
  }
  if (f->is_kphp_rpc_server_function() && needs_typed_fetch_store) {
    W << "std::unique_ptr<tl_func_base> " << struct_name << "::rpc_server_typed_fetch(" << get_php_runtime_type(f) << " *tl_object) " << BEGIN;
    W << "CurrentProcessingQuery::get().set_current_tl_function(" << register_tl_const_str(f->name) << ");" << NL;
    W << "auto tl_func_state = make_unique_on_script_memory<" << struct_name << ">();" << NL;
    W << CombinatorFetch(f, CombinatorPart::LEFT, true);
    W << "CurrentProcessingQuery::get().reset();" << NL;
    W << "return std::move(tl_func_state);" << NL;
    W << END << NL << NL;
    W << "void " << struct_name << "::rpc_server_typed_store(const class_instance<" << G->env().get_tl_classname_prefix()
      << "RpcFunctionReturnResult> &tl_object_) " << BEGIN;
    W << "CurrentProcessingQuery::get().set_current_tl_function(" << register_tl_const_str(f->name) << ");" << NL;
    W << "auto tl_object = tl_object_.template cast_to<" << get_php_runtime_type(f, false) << "_result>().get();" << NL;
    W << CombinatorStore(f, CombinatorPart::RIGHT, true);
    W << "CurrentProcessingQuery::get().reset();" << NL;
    W << END << NL << NL;
  }
}
}
