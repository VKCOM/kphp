// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/const-globals-batched-mem.h"

#include <common/algorithms/string-algorithms.h>
#include <compiler/data/function-data.h>

#include "common/php-functions.h"

#include "compiler/compiler-core.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"

// see const-globals-batched-mem.h for detailed comments of what's going on

namespace {

ConstantsBatchedMem constants_batched_mem;
GlobalsBatchedMem globals_batched_mem;

int calc_sizeof_tuple_shape(const TypeData *type);

[[gnu::always_inline]] inline int calc_sizeof_in_bytes_runtime(const TypeData *type) {
  switch (type->get_real_ptype()) {
    case tp_int:
      return type->use_optional() ? SIZEOF_OPTIONAL + sizeof(int64_t) : sizeof(int64_t);
    case tp_float:
      return type->use_optional() ? SIZEOF_OPTIONAL + sizeof(double) : sizeof(double);
    case tp_string:
      return type->use_optional() ? SIZEOF_OPTIONAL + SIZEOF_STRING : SIZEOF_STRING;
    case tp_array:
      return type->use_optional() ? SIZEOF_OPTIONAL + SIZEOF_ARRAY_ANY : SIZEOF_ARRAY_ANY;
    case tp_regexp:
      kphp_assert(!type->use_optional());
      return SIZEOF_REGEXP;
    case tp_Class:
      kphp_assert(!type->use_optional());
      return SIZEOF_INSTANCE_ANY;
    case tp_mixed:
      kphp_assert(!type->use_optional());
      return SIZEOF_MIXED;
    case tp_bool:
      return type->use_optional() ? 2 : 1;
    case tp_tuple:
    case tp_shape:
      return calc_sizeof_tuple_shape(type);
    case tp_future:
      return type->use_optional() ? SIZEOF_OPTIONAL + SIZEOF_FUTURE : SIZEOF_FUTURE;
    case tp_future_queue:
      return type->use_optional() ? SIZEOF_OPTIONAL + SIZEOF_FUTURE_QUEUE : SIZEOF_FUTURE_QUEUE;
    case tp_any:
      return SIZEOF_UNKNOWN;
    default:
      kphp_error(0, fmt_format("Unable to detect sizeof() for type = {}", type->as_human_readable()));
      return 0;
  }
}

[[gnu::noinline]] int calc_sizeof_tuple_shape(const TypeData *type) {
  kphp_assert(vk::any_of_equal(type->ptype(), tp_tuple, tp_shape));

  int result = 0;
  bool has_align_8bytes = false;
  for (auto sub = type->lookup_begin(); sub != type->lookup_end(); ++sub) {
    int sub_sizeof = calc_sizeof_in_bytes_runtime(sub->second);
    if (sub_sizeof >= 8) {
      has_align_8bytes = true;
      result = (result + 7) & -8;
    }
    result += sub_sizeof;
  }
  if (has_align_8bytes) {
    result = (result + 7) & -8;
  }
  return type->use_optional()
         ? has_align_8bytes ? SIZEOF_OPTIONAL + result : 1 + result
         : result;
}

} // namespace


void ConstantsBatchedMem::inc_count_by_type(const TypeData *type) {
  if (type->use_optional()) {
    count_of_type_other++;
    return;
  }
  switch (type->get_real_ptype()) {
    case tp_string:
      count_of_type_string++;
      break;
    case tp_regexp:
      count_of_type_regexp++;
      break;
    case tp_array:
      count_of_type_array++;
      return;
    case tp_mixed:
      count_of_type_mixed++;
      break;
    case tp_Class:
      count_of_type_instance++;
      break;
    default:
      count_of_type_other++;
  }
}

int ConstantsBatchedMem::detect_constants_batch_count(int n_constants) {
  // these values are heuristics (don't use integer division, to avoid changing buckets count frequently)
  if (n_constants > 1200000) return 2048;
  if (n_constants > 800000) return 1536;
  if (n_constants > 500000) return 1024;
  if (n_constants > 100000) return 512;
  if (n_constants > 10000) return 256;
  if (n_constants > 5000) return 128;
  if (n_constants > 1000) return 32;
  if (n_constants > 500) return 16;
  if (n_constants > 100) return 4;
  return 1;
}

const ConstantsBatchedMem &ConstantsBatchedMem::prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_constants) {
  ConstantsBatchedMem &mem = constants_batched_mem;

  const int N_BATCHES = detect_constants_batch_count(all_constants.size());
  mem.batches.resize(N_BATCHES);

  for (VarPtr var : all_constants) {
    int batch_idx = static_cast<int>(vk::std_hash(var->name) % N_BATCHES);
    var->batch_idx = batch_idx;
    mem.batches[batch_idx].n_constants++;
    if (var->dependency_level > mem.batches[batch_idx].max_dep_level) {
      mem.batches[batch_idx].max_dep_level = var->dependency_level;
    }
  }

  for (int batch_idx = 0; batch_idx < N_BATCHES; ++batch_idx) {
    mem.batches[batch_idx].batch_idx = batch_idx;
    mem.batches[batch_idx].constants.reserve(mem.batches[batch_idx].n_constants);
  }

  for (VarPtr var : all_constants) {
    mem.batches[var->batch_idx].constants.emplace_back(var);
  }

  for (OneBatchInfo &batch : mem.batches) {
    // sort constants by name to make codegen stable
    std::sort(batch.constants.begin(), batch.constants.end(), [](VarPtr c1, VarPtr c2) -> bool {
      return c1->name.compare(c2->name) < 0;
    });

    for (VarPtr var : batch.constants) {
      const TypeData *var_type = tinf::get_type(var);
      mem.inc_count_by_type(var_type);  // count stat to output it
    }

    mem.total_count += batch.constants.size();
  }

  return mem;
}

void GlobalsBatchedMem::inc_count_by_origin(VarPtr var) {
  if (var->is_class_static_var()) {
    count_of_static_fields++;
  } else if (var->is_function_static_var()) {
    count_of_function_statics++;
  } else if (vk::string_view{var->name}.starts_with("d$")) {
    count_of_nonconst_defines++;
  } else if (vk::string_view{var->name}.ends_with("$called")) {
    count_of_require_once++;
  } else if (!var->is_builtin_runtime) {
    count_of_php_global_scope++;
  } 
}

int GlobalsBatchedMem::detect_globals_batch_count(int n_globals) {
  if (n_globals > 10000) return 256;
  if (n_globals > 5000) return 128;
  if (n_globals > 1000) return 32;
  if (n_globals > 500) return 16;
  if (n_globals > 100) return 4;
  if (n_globals > 50) return 2;
  return 1;
}

const GlobalsBatchedMem &GlobalsBatchedMem::prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_globals) {
  GlobalsBatchedMem &mem = globals_batched_mem;

  const int N_BATCHES = detect_globals_batch_count(all_globals.size());
  mem.batches.resize(N_BATCHES);

  for (VarPtr var : all_globals) {
    int batch_idx = static_cast<int>(vk::std_hash(var->name) % N_BATCHES);
    var->batch_idx = batch_idx;
    mem.batches[batch_idx].n_globals++;
  }

  for (int batch_idx = 0; batch_idx < N_BATCHES; ++batch_idx) {
    mem.batches[batch_idx].batch_idx = batch_idx;
    mem.batches[batch_idx].globals.reserve(mem.batches[batch_idx].n_globals);
  }

  for (VarPtr var : all_globals) {
    mem.batches[var->batch_idx].globals.emplace_back(var);
  }

  for (OneBatchInfo &batch : mem.batches) {
    // sort variables by name to make codegen stable
    // note, that all_globals contains also function static vars (explicitly added),
    // and their names can duplicate or be equal to global vars;
    // hence, also sort by holder_func (though global vars don't have holder_func, since there's no point of declaration)
    std::sort(batch.globals.begin(), batch.globals.end(), [](VarPtr c1, VarPtr c2) -> bool {
      int cmp_name = c1->name.compare(c2->name);
      if (cmp_name < 0) {
        return true;
      } else if (cmp_name > 0) {
        return false;
      } else if (c1 == c2) {
        return false;
      } else {
        if (!c1->holder_func) return true;
        if (!c2->holder_func) return false;
        return c1->holder_func->name.compare(c2->holder_func->name) < 0;
      }
    });

    int offset = 0;

    for (VarPtr var : batch.globals) {
      const TypeData *var_type = tinf::get_type(var);
      int cur_sizeof = (calc_sizeof_in_bytes_runtime(var_type) + 7) & -8; // min 8 bytes per variable

      var->offset_in_linear_mem = mem.total_mem_size + offset; // it's continuous
      offset += cur_sizeof;
      mem.inc_count_by_origin(var);
    }

    // leave "spaces" between batches for less incremental re-compilation:
    // when PHP code changes (introducing a new global, for example), offsets will be shifted
    // only inside one batch, but not throughout the whole project
    // (with the exception, when a rounded batch size exceeds next 1KB)
    // note, that we don't do this for constants: while globals memory is a single continuous piece,
    // constants, on the contrary, are physically independent C++ variables
    offset = (offset + 1023) & -1024;

    mem.total_mem_size += offset;
    mem.total_count += batch.globals.size();
  }

  return mem;
}

void ConstantsExternCollector::add_extern_from_var(VarPtr var) {
  kphp_assert(var->is_constant());
  extern_constants.insert(var);
}

void ConstantsExternCollector::add_extern_from_init_val(VertexPtr init_val) {
  if (auto var = init_val.try_as<op_var>()) {
    add_extern_from_var(var->var_id);
  }
  for (VertexPtr child : *init_val) {
    add_extern_from_init_val(child);
  }
}

void ConstantsExternCollector::compile(CodeGenerator &W) const {
  for (VarPtr c : extern_constants) {
    W << "extern " << type_out(tinf::get_type(c)) << " " << c->name << ";" << NL;
  }
}

void ConstantsMemAllocation::compile(CodeGenerator &W) const {
  const ConstantsBatchedMem &mem = constants_batched_mem;

  W << "// total_count = " << mem.total_count << NL;
  W << "// count(string) = " << mem.count_of_type_string << NL;
  W << "// count(regexp) = " << mem.count_of_type_regexp << NL;
  W << "// count(array) = " << mem.count_of_type_array << NL;
  W << "// count(mixed) = " << mem.count_of_type_mixed << NL;
  W << "// count(instance) = " << mem.count_of_type_instance << NL;
  W << "// count(other) = " << mem.count_of_type_other << NL;
  W << "// n_batches = " << mem.batches.size() << NL;
}

void GlobalsMemAllocation::compile(CodeGenerator &W) const {
  const GlobalsBatchedMem &mem = globals_batched_mem;

  W << "// total_mem_size = " << mem.total_mem_size << NL;
  W << "// total_count = " << mem.total_count << NL;
  W << "// count(static fields) = " << mem.count_of_static_fields << NL;
  W << "// count(function statics) = " << mem.count_of_function_statics << NL;
  W << "// count(nonconst defines) = " << mem.count_of_nonconst_defines << NL;
  W << "// count(require_once) = " << mem.count_of_require_once << NL;
  W << "// count(php global scope) = " << mem.count_of_php_global_scope << NL;
  W << "// n_batches = " << mem.batches.size() << NL;

  if (!G->is_output_mode_lib() && !G->is_output_mode_k2_lib()) {
    W << "php_globals.once_alloc_linear_mem(" << mem.total_mem_size << ");" << NL;
  } else {
    W << "php_globals.once_alloc_linear_mem(\"" << G->settings().static_lib_name.get() << "\", " << mem.total_mem_size << ");" << NL;
  }
}

void PhpMutableGlobalsAssignCurrent::compile(CodeGenerator &W) const {
  W << "PhpScriptMutableGlobals &php_globals = PhpScriptMutableGlobals::current();" << NL;
}

void PhpMutableGlobalsDeclareInResumableClass::compile(CodeGenerator &W) const {
  W << "PhpScriptMutableGlobals &php_globals;" << NL;
}

void PhpMutableGlobalsAssignInResumableConstructor::compile(CodeGenerator &W) const {
  W << "php_globals(PhpScriptMutableGlobals::current())";
}

void PhpMutableGlobalsRefArgument::compile(CodeGenerator &W) const {
  W << "PhpScriptMutableGlobals &php_globals";
}

void PhpMutableGlobalsConstRefArgument::compile(CodeGenerator &W) const {
  W << "const PhpScriptMutableGlobals &php_globals";
}

void GlobalVarInPhpGlobals::compile(CodeGenerator &W) const {
  if (global_var->is_builtin_runtime) {
    W << "php_globals.get_superglobals().v$" << global_var->name;
  } else if (!G->is_output_mode_lib() && !G->is_output_mode_k2_lib()) {
    W << "(*reinterpret_cast<" << type_out(tinf::get_type(global_var)) << "*>(php_globals.mem()+" << global_var->offset_in_linear_mem << "))";
  } else {
    W << "(*reinterpret_cast<" << type_out(tinf::get_type(global_var)) << "*>(php_globals.mem_for_lib(\"" << G->settings().static_lib_name.get() << "\")+" << global_var->offset_in_linear_mem << "))";
  }
}
