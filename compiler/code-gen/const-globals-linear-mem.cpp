// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/const-globals-linear-mem.h"

#include <common/algorithms/string-algorithms.h>
#include <compiler/data/function-data.h>

#include "common/php-functions.h"

#include "compiler/compiler-core.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"

namespace {

ConstantsLinearMem constants_linear_mem;
GlobalsLinearMem globals_linear_mem;

int calc_sizeof_tuple_shape(const TypeData *type);

[[gnu::always_inline]] inline int calc_sizeof_in_bytes_runtime(const TypeData *type) {
  switch (type->get_real_ptype()) {
    case tp_int:
    case tp_float:
      return type->use_optional() ? SIZEOF_OPTIONAL + 8 : 8;
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
    case tp_future_queue:
      return type->use_optional() ? SIZEOF_OPTIONAL + 8 : 8;
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
  return type->use_optional() ? has_align_8bytes ? SIZEOF_OPTIONAL + result : 1 + result : result;
}

} // namespace


void ConstantsLinearMem::inc_count_by_type(const TypeData *type) {
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

void ConstantsLinearMem::prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_constants) {
  ConstantsLinearMem &mem = constants_linear_mem;

  int offset = 0;
  for (VarPtr var : all_constants) {
    const TypeData *var_type = tinf::get_type(var);
    int cur_sizeof = (calc_sizeof_in_bytes_runtime(var_type) + 7) & -8; // min 8 bytes per variable

    var->offset_in_linear_mem = offset;
    offset += cur_sizeof;
    mem.inc_count_by_type(var_type);
  }

  mem.total_mem_size = offset;
  mem.total_count = all_constants.size();
}

void GlobalsLinearMem::inc_count_by_origin(VarPtr var) {
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

void GlobalsLinearMem::prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_globals) {
  GlobalsLinearMem &mem = globals_linear_mem;

  int offset = 0;
  for (VarPtr var : all_globals) {
    const TypeData *var_type = tinf::get_type(var);
    int cur_sizeof = (calc_sizeof_in_bytes_runtime(var_type) + 7) & -8; // min 8 bytes per variable

    var->offset_in_linear_mem = offset;
    offset += cur_sizeof;
    mem.inc_count_by_origin(var);
  }

  mem.total_mem_size = offset;
  mem.total_count = all_globals.size();
}

void ConstantsLinearMemDeclaration::compile(CodeGenerator &W) const {
  if (G->is_output_mode_k2_component()) {
    // in k2_component mode definition of c_linear_mem stored in ImageState
    return;
  }

  if (is_extern) {
    W << "extern char *c_linear_mem;" << NL;
    return;
  }

  const ConstantsLinearMem &mem = constants_linear_mem;
  W << "// total_mem_size = " << mem.total_mem_size << NL;
  W << "// total_count = " << mem.total_count << NL;
  W << "// count(string) = " << mem.count_of_type_string << NL;
  W << "// count(regexp) = " << mem.count_of_type_regexp << NL;
  W << "// count(array) = " << mem.count_of_type_array << NL;
  W << "// count(mixed) = " << mem.count_of_type_mixed << NL;
  W << "// count(instance) = " << mem.count_of_type_instance << NL;
  W << "// count(other) = " << mem.count_of_type_other << NL;

  W << "char *c_linear_mem;" << NL;
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

void ConstantsLinearMemAllocation::compile(CodeGenerator &W) const {
  std::string c_linear_mem_use = G->is_output_mode_k2_component() ? "get_mutable_image_state()->c_linear_mem" : "c_linear_mem";
  if (G->is_output_mode_k2_component()) {
    W << c_linear_mem_use <<
      " = static_cast<char *>(get_platform_context()->allocator.alloc(" << constants_linear_mem.get_total_linear_mem_size() << "));" << NL;
  } else {
    W << c_linear_mem_use << " = new char[" << constants_linear_mem.get_total_linear_mem_size() << "];" << NL;
  }
  W << "memset(" << c_linear_mem_use << ", 0, " << constants_linear_mem.get_total_linear_mem_size() << ");" << NL;
}

void GlobalsLinearMemAllocation::compile(CodeGenerator &W) const {
  const GlobalsLinearMem &mem = globals_linear_mem;
  W << "// total_mem_size = " << mem.total_mem_size << NL;
  W << "// total_count = " << mem.total_count << NL;
  W << "// count(static fields) = " << mem.count_of_static_fields << NL;
  W << "// count(function statics) = " << mem.count_of_function_statics << NL;
  W << "// count(nonconst defines) = " << mem.count_of_nonconst_defines << NL;
  W << "// count(require_once) = " << mem.count_of_require_once << NL;
  W << "// count(php global scope) = " << mem.count_of_php_global_scope << NL;

  if (!G->is_output_mode_lib()) {
    W << "php_globals.once_alloc_linear_mem(" << globals_linear_mem.get_total_linear_mem_size() << ");" << NL;
  } else {
    W << "php_globals.once_alloc_linear_mem(\"" << G->settings().static_lib_name.get() << "\", " << globals_linear_mem.get_total_linear_mem_size() << ");" << NL;
  }
}

void ConstantVarInLinearMem::compile(CodeGenerator &W) const {
  std::string c_linear_mem_use = G->is_output_mode_k2_component() ? "get_image_state()->c_linear_mem" : "c_linear_mem";
  W << "(*reinterpret_cast<" << type_out(tinf::get_type(const_var)) << "*>(" << c_linear_mem_use << "+" << const_var->offset_in_linear_mem << "))";
}

void GlobalVarInPhpGlobals::compile(CodeGenerator &W) const {
  if (global_var->is_builtin_runtime) {
    W << "php_globals.get_superglobals().v$" << global_var->name;
  } else if (!G->is_output_mode_lib()) {
    W << "(*reinterpret_cast<" << type_out(tinf::get_type(global_var)) << "*>(php_globals.get_linear_mem()+" << global_var->offset_in_linear_mem << "))";
  } else {
    W << "(*reinterpret_cast<" << type_out(tinf::get_type(global_var)) << "*>(php_globals.get_linear_mem(\"" << G->settings().static_lib_name.get() << "\")+" << global_var->offset_in_linear_mem << "))";
  }
}
