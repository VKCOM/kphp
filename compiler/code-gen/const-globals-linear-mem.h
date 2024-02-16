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

class GlobalsLinearMem {
  int count_of_static_fields = 0;
  int count_of_function_statics = 0;
  int count_of_nonconst_defines = 0;
  int count_of_require_once = 0;
  int count_of_php_globals = 0;

  int total_count = 0;
  int total_mem_size = 0;

  std::vector<const TypeData *> debug_sizeof_static_asserts;

  void inc_count_by_origin(VarPtr var);

public:

  void prepare_globals_linear_mem_and_assign_offsets(std::vector<VarPtr> &all_globals);

  int get_total_linear_mem_size() const { return total_mem_size; }

  void codegen_counts_as_comments(CodeGenerator &W) const;
  void codegen_debug_sizeof_static_asserts(CodeGenerator &W) const;
};
