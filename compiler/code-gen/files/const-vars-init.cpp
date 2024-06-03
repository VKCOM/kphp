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

ConstVarsInit::ConstVarsInit(const ConstantsLinearMem &all_constants_in_mem)
  : all_constants_in_mem(all_constants_in_mem) {}

void ConstVarsInit::compile_const_init_part(CodeGenerator &W, const ConstantsLinearMem::OneBatchInfo &dir_batch) {
  DepLevelContainer const_raw_array_vars;
  DepLevelContainer other_const_vars;
  DepLevelContainer const_raw_string_vars;

  IncludesCollector includes;
  for (VarPtr var : dir_batch.constants) {
    if (!G->is_output_mode_lib()) {
      includes.add_var_signature_depends(var);
      includes.add_vertex_depends(var->init_val);
    }
  }
  W << includes;

  W << OpenNamespace();
  W << "char c_" << dir_batch.batch_hex << "[" << dir_batch.mem_size << "]; // " << dir_batch.batch_path << NL;

  for (VarPtr var : dir_batch.constants) {
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

  const std::string func_name_i = fmt_format("c_init_{}", dir_batch.batch_hex);
  FunctionSignatureGenerator(W) << NL << "void " << func_name_i << "()" << BEGIN;

  size_t str_idx = 0;
  size_t arr_idx = 0;
  for (size_t dep_level = 0; dep_level < max_dep_level; ++dep_level) {
    W << NL << "// level " << dep_level << NL;

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
  }

  W << END;
  W << CloseNamespace();
}

void ConstVarsInit::compile_const_init(CodeGenerator &W, const ConstantsLinearMem &all_constants_in_mem) {
  W << OpenNamespace();

  W << NL;

  FunctionSignatureGenerator(W) << "void const_vars_init() " << BEGIN;
  W << ConstantsLinearMemAllocation() << NL;

  for (const auto &[_, dir_batch] : all_constants_in_mem.get_batches()) {
    const std::string func_name_i = fmt_format("c_init_{}", dir_batch.batch_hex);
    // function declaration
    W << "void " << func_name_i << "();" << NL;
    // function call
    W << func_name_i << "();" << NL;
  }
  W << END;
  W << CloseNamespace();
}

void ConstVarsInit::compile(CodeGenerator &W) const {
  for (const auto &[_, dir_batch] : all_constants_in_mem.get_batches()) {
    W << OpenFile("c." + dir_batch.batch_hex + ".cpp", "o_const_init", false);
    W << ExternInclude(G->settings().runtime_headers.get());
    compile_const_init_part(W, dir_batch);
    W << CloseFile();
  }

  W << OpenFile("const_vars_init.cpp", "", false);
  W << ExternInclude(G->settings().runtime_headers.get());
  compile_const_init(W, all_constants_in_mem);
  W << CloseFile();
}
