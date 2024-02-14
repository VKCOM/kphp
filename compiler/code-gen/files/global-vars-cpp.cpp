// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/global-vars-cpp.h"

#include "common/algorithms/hashes.h"

// todo filter
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/stage.h"

struct InitVar {
  VarPtr var;
  explicit InitVar(VarPtr var) : var(var) {}

  void compile(CodeGenerator &W) const {
    Location save_location = stage::get_location();

    VertexPtr init_val = var->init_val;
    if (init_val->type() == op_conv_regexp) {
      const auto &location = init_val->get_location();
      kphp_assert(location.function && location.file);
      W << VarName(var) << ".init (" << var->init_val << ", "
        << RawString(location.function->name) << ", "
        << RawString(location.file->relative_file_name + ':' + std::to_string(location.line))
        << ");" << NL;
    } else {
      W << VarName(var) << " = " << var->init_val << ";" << NL;
    }

    stage::set_location(save_location);
  }
};


//static void add_dependent_declarations(VertexPtr vertex, std::set<VarPtr> &dependent_vars) {
//  if (!vertex) {
//    return;
//  }
//  for (auto child: *vertex) {
//    add_dependent_declarations(child, dependent_vars);
//  }
//  if (auto var = vertex.try_as<op_var>()) {
//    dependent_vars.emplace(var->var_id);
//  }
//}

//static void compile_raw_array(CodeGenerator &W, const VarPtr &var, int shift) {
//  if (shift == -1) {
//    W << InitVar(var);
//    W << VarName(var) << ".set_reference_counter_to(ExtraRefCnt::for_global_const);" << NL << NL;
//    return;
//  }
//
//  W << VarName(var) << ".assign_raw((char *) &raw_arrays[" << shift << "]);" << NL << NL;
//}

static void compile_globals_part(CodeGenerator &W, const std::vector<VarPtr> &vars, size_t part_id) {
  std::string file_name = "globals" + std::to_string(part_id) + ".cpp";
  W << OpenFile(file_name, "o_globals", false);

  W << ExternInclude(G->settings().runtime_headers.get());

  DepLevelContainer const_raw_array_vars;
  DepLevelContainer other_const_vars;
  DepLevelContainer const_raw_string_vars;
  std::set<VarPtr> dependent_vars;

  IncludesCollector includes;
  for (auto var : vars) {
    if (!G->settings().is_static_lib_mode() || !var->is_builtin_global()) {
      includes.add_var_signature_depends(var);
      includes.add_vertex_depends(var->init_val);
    }
  }
  W << includes;

  W << OpenNamespace();
  for (auto var : vars) {
    if (G->settings().is_static_lib_mode() && var->is_builtin_global()) {
      continue;
    }

    W << VarDeclaration(var);
//    if (var->is_constant()) {
//      switch (var->init_val->type()) {
//        case op_string:
//          const_raw_string_vars.add(var);
//          break;
//        case op_array:
//          add_dependent_declarations(var->init_val, dependent_vars);
//          const_raw_array_vars.add(var);
//          break;
//        case op_var:
//          add_dependent_declarations(var->init_val, dependent_vars);
//          other_const_vars.add(var);
//          break;
//        default:
//          other_const_vars.add(var);
//          break;
//      }
//    }
  }

  std::vector<VarPtr> extern_depends;
  std::set_difference(dependent_vars.begin(), dependent_vars.end(),
                      vars.begin(), vars.end(), std::back_inserter(extern_depends));
  for (auto var : extern_depends) {
    W << VarExternDeclaration(var);
  }

  std::vector<std::string> values(const_raw_string_vars.size());
  std::transform(const_raw_string_vars.begin(), const_raw_string_vars.end(),
                 values.begin(),
                 [](const VarPtr &var){ return var->init_val.as<op_string>()->get_string(); });
  auto const_string_shifts = compile_raw_data(W, values);

  const std::vector<int> const_array_shifts = compile_arrays_raw_representation(const_raw_array_vars, W);
  kphp_assert(const_array_shifts.size() == const_raw_array_vars.size());


//  const size_t max_dep_level = std::max({const_raw_string_vars.max_dep_level(), const_raw_array_vars.max_dep_level(), other_const_vars.max_dep_level()});

//  size_t str_idx = 0;
//  size_t arr_idx = 0;
//  for (size_t dep_level = 0; dep_level < max_dep_level; ++dep_level) {
//    FunctionSignatureGenerator(W) << NL << "void const_vars_init_priority_" << dep_level << "_file_" << part_id << "()" << BEGIN;
//
//    for (const auto &var : const_raw_string_vars.vars_by_dep_level(dep_level)) {
//      W << VarName(var) << ".assign_raw (&raw[" << const_string_shifts[str_idx++] << "]);" << NL;
//    }
//
//    for (const auto &var : const_raw_array_vars.vars_by_dep_level(dep_level)) {
//      compile_raw_array(W, var, const_array_shifts[arr_idx++]);
//    }
//
//    for (const auto &var: other_const_vars.vars_by_dep_level(dep_level)) {
//      W << InitVar(var);
//      const auto *type_data = var->tinf_node.get_type();
//      PrimitiveType ptype = type_data->ptype();
//      if (vk::any_of_equal(ptype, tp_array, tp_mixed, tp_string)) {
//        W << VarName(var);
//        if (type_data->use_optional()) {
//          W << ".val()";
//        }
//        W << ".set_reference_counter_to(ExtraRefCnt::for_global_const);" << NL;
//      }
//    }
//
//    W << END << NL;
//  }

  W << CloseNamespace();
  W << CloseFile();
}

GlobalVarsCppPart::GlobalVarsCppPart(std::vector<VarPtr> &&vars_of_part, size_t part_id)
  : vars_of_part_(std::move(vars_of_part))
  , part_id(part_id) {}

void GlobalVarsCppPart::compile(CodeGenerator &W) const {
  std::sort(vars_of_part_.begin(), vars_of_part_.end());
  compile_globals_part(W, vars_of_part_, part_id);
}
