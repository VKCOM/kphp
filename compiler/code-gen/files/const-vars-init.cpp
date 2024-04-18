// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/const-vars-init.h"

#include "common/algorithms/hashes.h"

#include "compiler/code-gen/const-globals-linear-mem.h"
#include "compiler/code-gen/declarations.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/public.h"

struct InitConstVar {
  VarPtr var;
  explicit InitConstVar(VarPtr var) : var(var) {}

  void compile(CodeGenerator &W) const {
    Location save_location = stage::get_location();

    VertexPtr init_val = var->init_val;
    if (init_val->type() == op_conv_regexp) {
      const auto &location = init_val->get_location();
      kphp_assert(location.function && location.file);
      W << ConstantVarInLinearMem(var) << ".init (" << var->init_val << ", " << RawString(location.function->name) << ", "
        << RawString(location.file->relative_file_name + ':' + std::to_string(location.line))
        << ");" << NL;
    } else {
      W << ConstantVarInLinearMem(var) << " = " << var->init_val << ";" << NL;
    }

    stage::set_location(save_location);
  }
};


static void compile_raw_array(CodeGenerator &W, const VarPtr &var, int shift) {
  if (shift == -1) {
    W << InitConstVar(var);
    W << ConstantVarInLinearMem(var) << ".set_reference_counter_to(ExtraRefCnt::for_global_const);" << NL << NL;
    return;
  }

  W << ConstantVarInLinearMem(var) << ".assign_raw((char *) &raw_arrays[" << shift << "]);" << NL << NL;
}

ConstVarsInit::ConstVarsInit(std::vector<std::vector<VarPtr>> &&all_constants_batched)
  : all_constants_batched(std::move(all_constants_batched)) {}

void ConstVarsInit::compile_const_init_part(CodeGenerator &W, int batch_num, const std::vector<VarPtr> &cur_batch) {
  DepLevelContainer const_raw_array_vars;
  DepLevelContainer other_const_vars;
  DepLevelContainer const_raw_string_vars;

  IncludesCollector includes;
  ConstantsLinearMemExternCollector c_mem_extern;
  for (VarPtr var : cur_batch) {
    if (!G->is_output_mode_lib()) {
      includes.add_var_signature_depends(var);
      includes.add_vertex_depends(var->init_val);
    }
    c_mem_extern.add_batch_num_from_var(var);
    c_mem_extern.add_batch_num_from_init_val(var->init_val);
  }
  W << includes;

  W << OpenNamespace();
  W << c_mem_extern << NL;

  for (VarPtr var : cur_batch) {
    switch (var->init_val->type()) {
      case op_string:
        const_raw_string_vars.add(var);
        break;
      case op_array:
        const_raw_array_vars.add(var);
        break;
      default:
        other_const_vars.add(var);
        break;
    }
  }

  std::vector<std::string> str_values(const_raw_string_vars.size());
  std::transform(const_raw_string_vars.begin(), const_raw_string_vars.end(),
                 str_values.begin(),
                 [](VarPtr var) { return var->init_val.as<op_string>()->str_val; });

  const std::vector<int> const_string_shifts = compile_raw_data(W, str_values);
  const std::vector<int> const_array_shifts = compile_arrays_raw_representation(const_raw_array_vars, W);
  const size_t max_dep_level = std::max({const_raw_string_vars.max_dep_level(), const_raw_array_vars.max_dep_level(), other_const_vars.max_dep_level(), 1ul});

  size_t str_idx = 0;
  size_t arr_idx = 0;
  for (size_t dep_level = 0; dep_level < max_dep_level; ++dep_level) {
    const std::string func_name_i = fmt_format("const_vars_init_deplevel{}_file{}", dep_level, batch_num);
    FunctionSignatureGenerator(W) << NL << "void " << func_name_i << "()" << BEGIN;

    for (VarPtr var : const_raw_string_vars.vars_by_dep_level(dep_level)) {
      // W << "// " << var->as_human_readable() << NL;
      W << ConstantVarInLinearMem(var) << ".assign_raw (&raw[" << const_string_shifts[str_idx++] << "]);" << NL;
    }

    for (VarPtr var : const_raw_array_vars.vars_by_dep_level(dep_level)) {
      // W << "// " << var->as_human_readable() << NL;
      compile_raw_array(W, var, const_array_shifts[arr_idx++]);
    }

    for (VarPtr var: other_const_vars.vars_by_dep_level(dep_level)) {
      // W << "// " << var->as_human_readable() << NL;
      W << InitConstVar(var);
      const TypeData *type_data = var->tinf_node.get_type();
      if (vk::any_of_equal(type_data->ptype(), tp_array, tp_mixed, tp_string)) {
        W << ConstantVarInLinearMem(var);
        if (type_data->use_optional()) {
          W << ".val()";
        }
        W << ".set_reference_counter_to(ExtraRefCnt::for_global_const);" << NL;
      }
    }

    W << END << NL;
  }

  W << CloseNamespace();
}

void ConstVarsInit::compile_const_init(CodeGenerator &W, int n_batches, const std::vector<int> &max_dep_levels) {
  W << OpenNamespace();

  W << NL;
  W << ConstantsLinearMemDeclaration() << NL;

  FunctionSignatureGenerator(W) << "void const_vars_init() " << BEGIN;
  W << ConstantsLinearMemAllocation() << NL;

  int very_max_dep_level = 0;
  for (int max_dep_level : max_dep_levels) {
    very_max_dep_level = std::max(very_max_dep_level, max_dep_level);
  }

  for (int dep_level = 0; dep_level <= very_max_dep_level; ++dep_level) {
    for (size_t batch_num = 0; batch_num < n_batches; ++batch_num) {
      if (dep_level <= max_dep_levels[batch_num]) {
        const std::string func_name_i = fmt_format("const_vars_init_deplevel{}_file{}", dep_level, batch_num);
        // function declaration
        W << "void " << func_name_i << "();" << NL;
        // function call
        W << func_name_i << "();" << NL;
      }
    }
  }
  W << END;
  W << CloseNamespace();
}

void ConstVarsInit::compile(CodeGenerator &W) const {
  int n_batches = static_cast<int>(all_constants_batched.size());

  std::vector<int> max_dep_levels(n_batches);
  for (int batch_num = 0; batch_num < n_batches; ++batch_num) {
    for (VarPtr var : all_constants_batched[batch_num]) {
      if (var->dependency_level > max_dep_levels[batch_num]) {
        max_dep_levels[batch_num] = var->dependency_level;
      }
    }
  }

  for (int batch_num = 0; batch_num < n_batches; ++batch_num) {
    W << OpenFile("const_init." + std::to_string(batch_num) + ".cpp", "o_const_init", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_const_init_part(W, batch_num, all_constants_batched[batch_num]);
    W << CloseFile();
  }

  W << OpenFile("const_vars_init.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_const_init(W, n_batches, max_dep_levels);
  W << CloseFile();
}
