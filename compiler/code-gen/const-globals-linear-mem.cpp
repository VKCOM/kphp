// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/const-globals-linear-mem.h"

#include "common/php-functions.h"

#include "compiler/code-gen/common.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"

[[gnu::always_inline]] static inline int calc_sizeof_in_bytes_runtime(const TypeData *type) {
  switch (type->get_real_ptype()) {
    case tp_int:
    case tp_float:
      return 8;
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
    default:
      kphp_error(0, fmt_format("Unable to detect sizeof() for type = {}", type->as_human_readable()));
      return 0;
  }
}

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
    int cur_sizeof = calc_sizeof_in_bytes_runtime(var_type);

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
