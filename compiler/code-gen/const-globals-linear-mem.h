// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"

class TypeData;
class CodeGenerator;

class ConstantsLinearMem {
  int count_of_type_string = 0;
  int count_of_type_regexp = 0;
  int count_of_type_array = 0;
  int count_of_type_mixed = 0;
  int count_of_type_instance = 0;
  int count_of_type_other = 0;

  int total_count = 0;
  int total_mem_size = 0;

  void inc_count_by_type(const TypeData *type);

public:
  
  void prepare_constants_linear_mem_and_assign_offsets(std::vector<VarPtr> &all_constants);
  
  int get_total_linear_mem_size() const { return total_mem_size; }

  void codegen_counts_as_comments(CodeGenerator &W) const;
};

