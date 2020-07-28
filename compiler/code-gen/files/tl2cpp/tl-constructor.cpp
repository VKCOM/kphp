#include "compiler/code-gen/files/tl2cpp/tl-constructor.h"

#include "compiler/code-gen/files/tl2cpp/tl-combinator.h"
#include "compiler/code-gen/files/tl2cpp/tl-template-php-type-helpers.h"
#include "compiler/code-gen/naming.h"

namespace tl2cpp {
const vk::tl::combinator *constructor;

std::string TlConstructorDecl::get_optional_args_for_decl(const vk::tl::combinator *c) {
  std::vector<std::string> res;
  for (const auto &arg : c->args) {
    if (arg->is_optional()) {
      if (type_of(arg->type_expr)->is_integer_variable()) {
        res.emplace_back("int64_t " + arg->name);
      } else {
        res.emplace_back("T" + std::to_string(arg->var_num) + " " + arg->name);
      }
    }
  }
  return vk::join(res, ", ");
}

void TlConstructorDecl::compile(CodeGenerator &W) const {
  const bool needs_typed_fetch_store = TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(constructor);

  if (needs_typed_fetch_store && is_type_dependent(constructor, tl)) {
    W << TlTemplatePhpTypeHelpers(constructor);
  }

  std::string template_decl = get_template_declaration(constructor);
  if (!template_decl.empty()) {
    W << template_decl << NL;
  }
  W << "struct " << cpp_tl_struct_name("c_", constructor->name) << " " << BEGIN;
  auto params = get_optional_args_for_decl(constructor);
  if (G->get_untyped_rpc_tl_used()) {
    FunctionSignatureGenerator(W) << "static void store(const var &tl_object" << (!params.empty() ? ", " + params : "") << ")" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "static array<var> fetch(" << params << ")" << SemicolonAndNL();
  }

  if (needs_typed_fetch_store) {
    std::string php_type = get_php_runtime_type(constructor, false);
    FunctionSignatureGenerator(W) << "static void typed_store(const " << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ")" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "static void typed_fetch_to(" << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ")" << SemicolonAndNL();
  }
  W << END << ";\n\n";
}

void TlConstructorDef::compile(CodeGenerator &W) const {
  const bool needs_typed_fetch_store = TlConstructorDecl::does_tl_constructor_need_typed_fetch_store(constructor);

  std::string template_decl = get_template_declaration(constructor);
  std::string template_def = get_template_definition(constructor);
  auto full_struct_name = cpp_tl_struct_name("c_", constructor->name, template_def);
  auto params = TlConstructorDecl::get_optional_args_for_decl(constructor);

  if (G->get_untyped_rpc_tl_used()) {
    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "void " << full_struct_name + "::store(const var& tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
    W << CombinatorStore(constructor, CombinatorPart::LEFT, false);
    W << END << "\n\n";

    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "array<var> " << full_struct_name + "::fetch(" << params << ") " << BEGIN;
    W << CombinatorFetch(constructor, CombinatorPart::LEFT, false);
    W << END << "\n\n";
  }

  if (needs_typed_fetch_store) {
    std::string php_type = get_php_runtime_type(constructor, false);
    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "void " << full_struct_name + "::typed_store(const " << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
    W << CombinatorStore(constructor, CombinatorPart::LEFT, true);
    W << END << "\n\n";

    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "void " << full_struct_name << "::typed_fetch_to(" << php_type << " *tl_object" << (!params.empty() ? ", " + params : "") << ") " << BEGIN;
    W << CombinatorFetch(constructor, CombinatorPart::LEFT, true);
    W << END << "\n\n";
  }
}
}
