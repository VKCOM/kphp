// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class TypeData;
class CodeGenerator;

class ConstantsBatchedMem {
public:
  struct OneBatchInfo {
    int batch_idx;
    int n_constants{0};
    std::vector<VarPtr> constants;
    int max_dep_level{0};
  };

private:
  friend struct ConstantsMemAllocation;

  int count_of_type_string = 0;
  int count_of_type_regexp = 0;
  int count_of_type_array = 0;
  int count_of_type_mixed = 0;
  int count_of_type_instance = 0;
  int count_of_type_other = 0;

  int total_count = 0;
  
  std::vector<OneBatchInfo> batches;

  void inc_count_by_type(const TypeData *type);

public:
  static int detect_constants_batch_count(int n_constants);
  static const ConstantsBatchedMem &prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_constants);

  const std::vector<OneBatchInfo> &get_batches() const { return batches; }
  const OneBatchInfo &get_batch(uint64_t batch_hash) const { return batches.at(batch_hash); }
};

class GlobalsBatchedMem {
public:
  struct OneBatchInfo {
    int batch_idx;
    int n_globals{0};
    std::vector<VarPtr> globals;
  };

private:
  friend struct GlobalsMemAllocation;

  int count_of_static_fields = 0;
  int count_of_function_statics = 0;
  int count_of_nonconst_defines = 0;
  int count_of_require_once = 0;
  int count_of_php_global_scope = 0;

  int total_count = 0;
  int total_mem_size = 0;

  std::vector<OneBatchInfo> batches;

  void inc_count_by_origin(VarPtr var);

public:
  static int detect_globals_batch_count(int n_globals);
  static const GlobalsBatchedMem &prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_globals);

  const std::vector<OneBatchInfo> &get_batches() const { return batches; }
  const OneBatchInfo &get_batch(int batch_idx) const { return batches.at(batch_idx); }
};

struct ConstantsExternCollector {
  void add_extern_from_var(VarPtr var);
  void add_extern_from_init_val(VertexPtr init_val);

  void compile(CodeGenerator &W) const;

private:
  std::set<VarPtr> extern_constants;
};

struct ConstantsMemAllocation {
  void compile(CodeGenerator &W) const;
};

struct GlobalsMemAllocation {
  void compile(CodeGenerator &W) const;
};

struct PhpMutableGlobalsAssignCurrent {
  void compile(CodeGenerator &W) const;
};

struct PhpMutableGlobalsDeclareInResumableClass {
  void compile(CodeGenerator &W) const;
};

struct PhpMutableGlobalsAssignInResumableConstructor {
  void compile(CodeGenerator &W) const;
};

struct PhpMutableGlobalsRefArgument {
  void compile(CodeGenerator &W) const;
};

struct PhpMutableGlobalsConstRefArgument {
  void compile(CodeGenerator &W) const;
};

struct GlobalVarInPhpGlobals {
  VarPtr global_var;

  explicit GlobalVarInPhpGlobals(VarPtr global_var)
    : global_var(global_var) {}

  void compile(CodeGenerator &W) const;
};
