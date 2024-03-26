// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"

class TypeData;
class CodeGenerator;

class ConstantsLinearMem {
  friend struct ConstantsLinearMemDeclaration;

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
  static void prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_constants);

  int get_total_linear_mem_size() const { return total_mem_size; }
};

class GlobalsLinearMem {
  friend struct GlobalsLinearMemDeclaration;

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
  static void prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_globals);

  int get_total_linear_mem_size() const { return total_mem_size; }
};

struct ConstantsLinearMemDeclaration {
  explicit ConstantsLinearMemDeclaration(bool is_extern)
    : is_extern(is_extern) {}

  void compile(CodeGenerator &W) const;

private:
  bool is_extern;
};

struct GlobalsLinearMemDeclaration {
  explicit GlobalsLinearMemDeclaration(bool is_extern)
    : is_extern(is_extern) {}

  void compile(CodeGenerator &W) const;

private:
  bool is_extern;
};

struct ConstantsLinearMemAllocation {
  void compile(CodeGenerator &W) const;
};

struct GlobalsLinearMemAllocation {
  void compile(CodeGenerator &W) const;
};

struct ConstantVarInLinearMem {
  VarPtr const_var;

  explicit ConstantVarInLinearMem(VarPtr const_var)
    : const_var(const_var) {}

  void compile(CodeGenerator &W) const;
};

struct GlobalVarInLinearMem {
  VarPtr global_var;

  explicit GlobalVarInLinearMem(VarPtr global_var)
    : global_var(global_var) {}

  void compile(CodeGenerator &W) const;
};
