// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/const-globals-linear-mem.h"

#include "common/php-functions.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"

namespace {

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

void ConstantsLinearMem::prepare_constants_linear_mem_and_assign_offsets(std::vector<VarPtr> &all_constants) {
  std::sort(all_constants.begin(), all_constants.end(), [](VarPtr c1, VarPtr c2) -> bool {
    return c1->name.compare(c2->name) < 0;
  });

  int offset = 0;
  for (VarPtr var : all_constants) {
    const TypeData *var_type = tinf::get_type(var);
    int cur_sizeof = (calc_sizeof_in_bytes_runtime(var_type) + 7) & -8; // min 8 bytes per variable

    var->offset_in_linear_mem = offset;
    offset += cur_sizeof;
    inc_count_by_type(var_type);
  }

  total_mem_size = offset;
  total_count = all_constants.size();
}

void ConstantsLinearMem::codegen_counts_as_comments(CodeGenerator &W) const {
  W << "// total_mem_size = " << total_mem_size << NL;
  W << "// total_count = " << total_count << NL;
  W << "// count(string) = " << count_of_type_string << NL;
  W << "// count(regexp) = " << count_of_type_regexp << NL;
  W << "// count(array) = " << count_of_type_array << NL;
  W << "// count(mixed) = " << count_of_type_mixed << NL;
  W << "// count(instance) = " << count_of_type_instance << NL;
  W << "// count(other) = " << count_of_type_other << NL;
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
  } else if (!var->is_builtin_global()) {
    count_of_php_globals++;
  } 
}

void GlobalsLinearMem::prepare_globals_linear_mem_and_assign_offsets(std::vector<VarPtr> &all_globals) {
  std::sort(all_globals.begin(), all_globals.end(), [](VarPtr c1, VarPtr c2) -> bool {
    return c1->name.compare(c2->name) < 0;
  });

  int offset = 0;
  for (VarPtr var : all_globals) {
    const TypeData *var_type = tinf::get_type(var);
    int cur_sizeof = (calc_sizeof_in_bytes_runtime(var_type) + 7) & -8; // min 8 bytes per variable

    var->offset_in_linear_mem = offset;
    offset += cur_sizeof;
    inc_count_by_origin(var);

    // todo comment this after testing in vkcom
    if (var_type->use_optional() || !vk::any_of_equal(var_type->get_real_ptype(), tp_mixed, tp_bool, tp_int, tp_float, tp_string, tp_array, tp_Class)) {
      debug_sizeof_static_asserts.push_back(var_type);
    }
  }

  total_mem_size = offset;
  total_count = all_globals.size();
}

void GlobalsLinearMem::codegen_counts_as_comments(CodeGenerator &W) const {
  W << "// total_mem_size = " << total_mem_size << NL;
  W << "// total_count = " << total_count << NL;
  W << "// count(static fields) = " << count_of_static_fields << NL;
  W << "// count(function statics) = " << count_of_function_statics << NL;
  W << "// count(nonconst defines) = " << count_of_nonconst_defines << NL;
  W << "// count(require_once) = " << count_of_require_once << NL;
  W << "// count(php globals) = " << count_of_php_globals << NL;

  if (!debug_sizeof_static_asserts.empty()) {
    codegen_debug_sizeof_static_asserts(W);
  }
}

void GlobalsLinearMem::codegen_debug_sizeof_static_asserts(CodeGenerator &W) const {
  IncludesCollector includes;
  for (const TypeData *type : debug_sizeof_static_asserts) {
    includes.add_all_class_types(*type);
  }
  W << includes;

  for (const TypeData *type : debug_sizeof_static_asserts) {
    int mem = calc_sizeof_in_bytes_runtime(type);
    W << "static_assert(" << mem << " == sizeof(" << type_out(type) << "));" << NL;
  }
}
