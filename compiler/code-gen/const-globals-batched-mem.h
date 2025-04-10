// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class TypeData;
class CodeGenerator;

// Here we put all auto-extracted constants (const strings, const arrays, etc.).
// They are initialized on master process start and therefore accessible from all workers for reading.
// Every const has a special refcount: if PHP code tries to mutate it, it'll be copied to script memory.
// Moreover, every constant has dependency_level (var-data.h). For instance, an array of strings
// should be initialized after all strings have been initialized.
//
// Since their count is huge, they are batched, and initialization is done (codegenerated) per-batch
// (more concrete, all level0 for every batch, then level1, etc.).
//
// When codegen starts, prepare_mem_and_assign_offsets() is called.
// It splits variables into batches and calculates necessary properties used in codegen later.
// Note, that finally each constant is represented as an independent C++ variable
// (whereas mutable globals, see below, are all placed in a single memory piece).
// We tested different ways to use the same approach for constants also. It works, but
// either leads to lots of incremental re-compilation or inconsistent compilation times in vkcom.
//
// See const-vars-init.cpp and collect-const-vars.cpp.
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

  void inc_count_by_type(const TypeData* type);

public:
  static int detect_constants_batch_count(int n_constants);
  static const ConstantsBatchedMem& prepare_mem_and_assign_offsets(const std::vector<VarPtr>& all_constants);

  const std::vector<OneBatchInfo>& get_batches() const {
    return batches;
  }
  const OneBatchInfo& get_batch(uint64_t batch_hash) const {
    return batches.at(batch_hash);
  }
};

// While constants are initialized once in master process, mutable globals exists in each script
// (and are initialized on script start, placed in script memory).
//
// Opposed to constants, mutable globals are NOT C++ variables: instead, they all are placed
// in a single linear memory piece (char *), every var has offset (VarData::offset_in_linear_mem),
// and we use (&reinterpret_cast<T*>) to access a variable at offset (GlobalVarInPhpGlobals below).
// The purpose of this approach is to avoid mutable state at the moment of code generation,
// so that we could potentially compile a script into .so and load it multiple times.
//
// Note, that for correct offset calculation, the compiler must be aware of sizeof() of any possible type.
// If (earlier) a global inferred a type `std::tuple<bool,int>`, g++ determined its size.
// Now, we need to compute sizes and offsets at the moment of code generation, and to do it
// exactly the same as g++ would. See const-globals-batched-mem.cpp for implementation.
//
// Another thing to point is that we also split globals into batches, but leave "spaces" in linear memory:
// [batch1, ...(nothing, rounded up to 1KB), batch2, ...(nothing), ...]
// It's done to achieve less incremental re-compilation: when PHP code changes introducing a new global,
// offsets will be shifted only inside one batch, but not throughout the whole project.
//
// See globals-vars-reset.cpp and (runtime) php-script-globals.h.
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
  static const GlobalsBatchedMem& prepare_mem_and_assign_offsets(const std::vector<VarPtr>& all_globals);

  const std::vector<OneBatchInfo>& get_batches() const {
    return batches;
  }
  const OneBatchInfo& get_batch(int batch_idx) const {
    return batches.at(batch_idx);
  }
};

struct ConstantsExternCollector {
  void add_extern_from_var(VarPtr var);
  void add_extern_from_init_val(VertexPtr init_val);

  void compile(CodeGenerator& W) const;

private:
  std::set<VarPtr> extern_constants;
};

struct ConstantsMemAllocation {
  void compile(CodeGenerator& W) const;
};

struct GlobalsMemAllocation {
  void compile(CodeGenerator& W) const;
};

struct PhpMutableGlobalsAssignCurrent {
  void compile(CodeGenerator& W) const;
};

struct PhpMutableGlobalsDeclareInResumableClass {
  void compile(CodeGenerator& W) const;
};

struct PhpMutableGlobalsAssignInResumableConstructor {
  void compile(CodeGenerator& W) const;
};

struct PhpMutableGlobalsRefArgument {
  void compile(CodeGenerator& W) const;
};

struct PhpMutableGlobalsConstRefArgument {
  void compile(CodeGenerator& W) const;
};

struct GlobalVarInPhpGlobals {
  VarPtr global_var;

  explicit GlobalVarInPhpGlobals(VarPtr global_var)
      : global_var(global_var) {}

  void compile(CodeGenerator& W) const;
};
