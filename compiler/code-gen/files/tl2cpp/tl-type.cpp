// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/tl2cpp/tl-type.h"

#include "compiler/code-gen/files/tl2cpp/tl-template-php-type-helpers.h"
#include "compiler/code-gen/naming.h"

namespace tl2cpp {
using vk::tlo_parsing::FLAG_DEFAULT_CONSTRUCTOR;
using vk::tlo_parsing::FLAG_NOCONS;

void TypeStore::compile(CodeGenerator &W) const {
  // todo: CHECK_EXCEPTION(return); ??? Should we interrupt storing if the exception was thrown? (Right now it's inconsistent with fetching)
  auto store_params = get_optional_args_for_call(type->constructors[0]);
  store_params.insert(store_params.begin(),
                      typed_mode ? (type->is_polymorphic() ? "conv_obj" : "tl_object.get()") : "tl_object");
  std::string store_call = (typed_mode ? "typed_store(" : "store(") + vk::join(store_params, ", ") + ");";

  if (typed_mode) {
    W << "if (tl_object.is_null()) " << BEGIN
      << "CurrentProcessingQuery::get().raise_storing_error(\"Instance expected, but false given while storing tl type\");" << NL
      << "return;" << NL
      << END << NL;
  }
  // non-polymorphic types — 1 constructor — forwards a ::store() into the constructor without 'magic'
  if (!type->is_polymorphic()) {
    W << cpp_tl_struct_name("c_", type->constructors[0]->name, template_str) << "::" << store_call << NL;
    return;
  }

  auto *default_constructor = (type->flags & FLAG_DEFAULT_CONSTRUCTOR ? type->constructors.back().get() : nullptr);
  // for polymorphic types:
  // typed TL: ifs with dynamic_cast, similar to the interface methods
  if (typed_mode) {
    bool first = true;
    for (const auto &c : type->constructors) {
      W << (first ? "if " : " else if ");
      first = false;

      std::string cpp_class = get_php_runtime_type(c.get());
      W << "(f$is_a<" << cpp_class << ">(tl_object)) " << BEGIN;
      if (c.get() != default_constructor) {
        W << fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(c->id)) << NL;
      }
      W << "const " << cpp_class << " *conv_obj = tl_object.template cast_to<" << cpp_class << ">().get();" << NL;
      W << cpp_tl_struct_name("c_", c->name, template_str) << "::" << store_call << NL << END;
    }
    W << (first ? "" : " else ") << BEGIN
      << "CurrentProcessingQuery::get().raise_storing_error(\"Invalid constructor %s of type %s\", "
      << "tl_object.get_class(), \"" << type->name << "\");" << NL
      << END << NL;

    // untyped TL: ifs with constructor_name checks
  } else {
    bool first = true;
    W << "const string &c_name = tl_arr_get(tl_object, "
      << register_tl_const_str("_") << ", 0, " << hash_tl_const_str("_") << "L).to_string();" << NL;
    for (const auto &c : type->constructors) {
      W << (first ? "if " : " else if ");
      first = false;

      W << "(c_name == " << register_tl_const_str(c->name) << ") " << BEGIN;
      if (c.get() != default_constructor) {
        W << fmt_format("f$store_int({:#010x});", static_cast<unsigned int>(c->id)) << NL;
      }
      W << cpp_tl_struct_name("c_", c->name, template_str) << "::" << store_call << NL << END;
    }
    W << (first ? "" : " else ") << BEGIN
      << "CurrentProcessingQuery::get().raise_storing_error(\"Invalid constructor %s of type %s\", "
      << "c_name.c_str(), \"" << type->name << "\");" << NL
      << END << NL;
  }
}

void TypeFetch::compile(CodeGenerator &W) const {
  auto fetch_params = get_optional_args_for_call(type->constructors[0]);
  std::string fetch_call;
  if (!typed_mode) {
    fetch_call = "fetch(" + vk::join(fetch_params, ", ") + ");";
  } else {
    fetch_params.insert(fetch_params.begin(), "result.get()");
    fetch_call = "typed_fetch_to(" + vk::join(fetch_params, ", ") + ");";
  }

  // non-polymorphic types are fetched in a trivial way - their fetch is delegated to the constructor
  if (!type->is_polymorphic()) {
    const auto &constructor = type->constructors.front();
    if (typed_mode) {
      W << "CHECK_EXCEPTION(return);" << NL;
      W << get_php_runtime_type(constructor.get(), true) << " result;" << NL;
      W << "result.alloc();" << NL;
      // all c_*.typed_fetch_to() get already allocated object;
      // it's allocated at t_*.typed_fetch_to()
      W << cpp_tl_struct_name("c_", constructor->name, template_str) << "::" << fetch_call << NL;
      W << "tl_object = result;" << NL;
    } else {
      W << "CHECK_EXCEPTION(return array<mixed>());" << NL;
      W << "return " << cpp_tl_struct_name("c_", constructor->name, template_str) << "::" << fetch_call << NL;
    }
    return;
  }

  // polymorphic types are fetched differently: first 'magic', then switch (magic) for every constructor
  if (!typed_mode) {
    W << "array<mixed> result;" << NL;
  }
  W << "CHECK_EXCEPTION(return" << (typed_mode ? "" : " result") << ");" << NL;
  auto *default_constructor = (type->flags & FLAG_DEFAULT_CONSTRUCTOR ? type->constructors.back().get() : nullptr);
  bool has_name = type->constructors_num > 1 && !(type->flags & FLAG_NOCONS);
  if (default_constructor != nullptr) {
    W << "int pos = tl_parse_save_pos();" << NL;
  }
  W << "auto magic = static_cast<unsigned int>(f$fetch_int());" << NL;
  W << "switch(magic) " << BEGIN;
  for (const auto &c : type->constructors) {
    if (c.get() == default_constructor) {
      continue;
    }
    W << fmt_format("case {:#010x}: ", static_cast<unsigned int>(c->id)) << BEGIN;
    if (!typed_mode) {
      W << "result = " << cpp_tl_struct_name("c_", c->name, template_str) << "::" << fetch_call << NL;
      if (has_name) {
        W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(c->name) << ", " << hash_tl_const_str("_") << "L);"
          << NL;
      }
    } else {
      W << get_php_runtime_type(c.get(), true) << " result;" << NL;
      W << "result.alloc();" << NL;
      W << cpp_tl_struct_name("c_", c->name, template_str) << "::" << fetch_call << NL;
      W << "tl_object = result;" << NL;
    }
    W << "break;" << NL;
    W << END << NL;
  }
  W << "default: " << BEGIN;
  if (default_constructor != nullptr) {
    W << "tl_parse_restore_pos(pos);" << NL;
    if (!typed_mode) {
      W << "result = " << cpp_tl_struct_name("c_", default_constructor->name, template_str) << "::" << fetch_call << NL;
      if (has_name) {
        W << "result.set_value(" << register_tl_const_str("_") << ", " << register_tl_const_str(default_constructor->name) << ", "
          << hash_tl_const_str("_") << "L);" << NL;
      }
    } else {
      W << get_php_runtime_type(default_constructor, true) << " result;" << NL;
      W << "result.alloc();" << NL;
      W << cpp_tl_struct_name("c_", default_constructor->name, template_str) << "::" << fetch_call << NL;
      W << "tl_object = result;" << NL;
    }
  } else {
    W << "CurrentProcessingQuery::get().raise_fetching_error(\"Incorrect magic of type " << type->name << ": 0x%08x\", magic);" << NL;
  }
  W << END << NL;
  W << END << NL;
  if (!typed_mode) {
    W << "return result;" << NL;
  }
}

void TlTypeDeclaration::compile(CodeGenerator &W) const {
  W << "/* ~~~~~~~~~ " << t->name << " ~~~~~~~~~ */\n" << NL;
  const bool needs_typed_fetch_store = TlTypeDeclaration::does_tl_type_need_typed_fetch_store(t);
  if (needs_typed_fetch_store && is_type_dependent(t, tl) && t->is_polymorphic()) {
    // For non-polymorphic types, use type helpers for the constructor; they're guaranteed to be available
    W << TlTemplatePhpTypeHelpers(t);
  }
  std::string struct_name = cpp_tl_struct_name("t_", t->name);
  auto *constructor = t->constructors[0].get();
  std::string template_decl = get_template_declaration(constructor);
  if (!template_decl.empty()) {
    W << template_decl << NL;
  }
  W << "struct " << struct_name << " " << BEGIN;
  W << "using PhpType = "
    << (needs_typed_fetch_store ? get_php_runtime_type(t) : "tl_undefined_php_type") << ";" << NL;
  std::vector<std::string> constructor_params;
  std::vector<std::string> constructor_inits;
  for (const auto &arg : constructor->args) {
    if (arg->var_num != -1) {
      if (type_of(arg->type_expr)->is_integer_variable()) {
        if (arg->is_optional()) {
          W << "int64_t " << arg->name << "{0};" << NL;
          constructor_params.emplace_back("int64_t " + arg->name);
          constructor_inits.emplace_back(fmt_format("{}({})", arg->name, arg->name));
        }
      } else if (type_of(arg->type_expr)->name == T_TYPE) {
        W << fmt_format("T{} {};", arg->var_num, arg->name) << NL;
        kphp_assert(arg->is_optional());
        constructor_params.emplace_back(fmt_format("T{} {}", arg->var_num, arg->name));
        constructor_inits.emplace_back(fmt_format("{}(std::move({}))", arg->name, arg->name));
      }
    }
  }
  if (!constructor_params.empty()) {
    W << "explicit " << struct_name << "(" << vk::join(constructor_params, ", ") << ") : " << vk::join(constructor_inits, ", ") << " {}\n" << NL;
  }
  if (G->get_untyped_rpc_tl_used()) {
    FunctionSignatureGenerator(W) << "void store(const mixed& tl_object)" << SemicolonAndNL();
    FunctionSignatureGenerator(W) << "array<mixed> fetch()" << SemicolonAndNL();
  }
  if (needs_typed_fetch_store) {
    FunctionSignatureGenerator(W)  << "void typed_store(const PhpType &tl_object)" << SemicolonAndNL();
    FunctionSignatureGenerator(W)  << "void typed_fetch_to(PhpType &tl_object)" << SemicolonAndNL();
  }
  W << END << ";\n\n";
}

void TlTypeDefinition::compile(CodeGenerator &W) const {
  const bool needs_typed_fetch_store = TlTypeDeclaration::does_tl_type_need_typed_fetch_store(t);

  auto *constructor = t->constructors[0].get();
  std::string struct_name = cpp_tl_struct_name("t_", t->name);
  std::string template_decl = get_template_declaration(constructor);
  std::string template_def = get_template_definition(constructor);
  auto full_struct_name = struct_name + template_def;

  if (G->get_untyped_rpc_tl_used()) {
    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "void " << full_struct_name << "::store(const mixed &tl_object) " << BEGIN;
    W << TypeStore(t, template_def);
    W << END << "\n\n";

    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "array<mixed> " << full_struct_name + "::fetch() " << BEGIN;
    W << TypeFetch(t, template_def);
    W << END << "\n\n";
  }

  if (needs_typed_fetch_store) {
    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "void " << full_struct_name << "::typed_store(const PhpType &tl_object) " << BEGIN;
    W << TypeStore(t, template_def, true);
    W << END << "\n\n";

    W << template_decl << NL;
    FunctionSignatureGenerator(W) << "void " << full_struct_name + "::typed_fetch_to(PhpType &tl_object) " << BEGIN;
    W << TypeFetch(t, template_def, true);
    W << END << "\n\n";
  }
}
}
