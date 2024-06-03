// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <set>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class TypeData;
class CodeGenerator;

class ConstantsLinearMem {
public:
  struct OneBatchInfo {
    uint64_t batch_hash;
    std::string batch_hex;
    std::string batch_path;
    std::vector<VarPtr> constants;
    int mem_size;
    int max_dep_level;
  };

private:
  friend struct ConstantsLinearMemAllocation;

  int count_of_type_string = 0;
  int count_of_type_regexp = 0;
  int count_of_type_array = 0;
  int count_of_type_mixed = 0;
  int count_of_type_instance = 0;
  int count_of_type_other = 0;

  std::map<uint64_t, OneBatchInfo> batches;

  int total_count = 0;
  int total_mem_size = 0;

  void inc_count_by_type(const TypeData *type);

public:
  static const ConstantsLinearMem &prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_constants);

  int get_total_linear_mem_size() const { return total_mem_size; }

  const std::map<uint64_t, OneBatchInfo> &get_batches() const { return batches; }
  const OneBatchInfo &get_batch(uint64_t batch_hash) const { return batches.at(batch_hash); }
};

class GlobalsLinearMem {
public:
  struct OneBatchInfo {
    int batch_idx;
    std::vector<VarPtr> globals;
    int mem_size;
  };

private:
  friend struct GlobalsLinearMemAllocation;

  int count_of_static_fields = 0;
  int count_of_function_statics = 0;
  int count_of_nonconst_defines = 0;
  int count_of_require_once = 0;
  int count_of_php_global_scope = 0;

  std::vector<OneBatchInfo> batches;

  int total_count = 0;
  int total_mem_size = 0;

  void inc_count_by_origin(VarPtr var);

public:
  static int detect_globals_batch_count(int n_globals);
  static const GlobalsLinearMem &prepare_mem_and_assign_offsets(const std::vector<VarPtr> &all_globals);

  int get_total_linear_mem_size() const { return total_mem_size; }

  const std::vector<OneBatchInfo> &get_batches() const { return batches; }
  const OneBatchInfo &get_batch(int batch_idx) const { return batches.at(batch_idx); }
};

struct ConstantsLinearMemExternCollector {
  void add_batch_num_from_var(VarPtr var);
  void add_batch_num_from_init_val(VertexPtr init_val);

  void compile(CodeGenerator &W) const;

private:
  std::set<uint64_t> required_batch_hashes;
};

struct PhpMutableGlobalsAssignCurrent {
  void compile(CodeGenerator &W) const;
};

struct PhpMutableGlobalsDeclareInResumableClass {
  void compile(CodeGenerator &W) const;
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
