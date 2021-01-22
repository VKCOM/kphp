// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"


enum class AssumptionStatus {
  unknown,
  processing,
  initialized
};

class TypeHint;

class Assumption {
  DEBUG_STRING_METHOD { return as_human_readable(); }

public:
  // the only field of Assumption indicates a possible state:
  // 1) nullptr — undefined
  // 2) primitive (then it's tp_any) — means not instance
  // 3) otherwise, it contains an instance inside
  // important! no int / string[] / tuple(int): only if it has instances! (so that it will help -> resolving)
  const TypeHint *assum_hint{nullptr};

  Assumption() = default;                           // create in undefined state
  static Assumption undefined();                    // create in undefined state
  static Assumption not_instance();                 // create in not_instance state
  explicit Assumption(const TypeHint *type_hint);   // create from type hint (save it if has instances)
  explicit Assumption(ClassPtr klass);

  std::string as_human_readable() const;

  bool is_undefined() const { return assum_hint == nullptr; }
  explicit operator bool() const { return assum_hint != nullptr; }

  bool has_instance() const;

  ClassPtr try_as_class() const { return extract_instance_from_type_hint(assum_hint); }
  Assumption get_inner_if_array() const;
  Assumption get_subkey_by_index(VertexPtr index_key) const;

  static ClassPtr extract_instance_from_type_hint(const TypeHint *type_hint);
};


void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const Assumption &assumption);
Assumption assumption_get_for_var(FunctionPtr f, vk::string_view var_name);
Assumption infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth = 0);
Assumption calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call);
Assumption calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth = 0);
Assumption calc_assumption_for_argument(FunctionPtr f, vk::string_view var_name);
