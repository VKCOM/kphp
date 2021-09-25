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

class Assumption {
  DEBUG_STRING_METHOD { return as_human_readable(); }

public:
  // the only field of Assumption indicates a possible state:
  // 1) nullptr — undefined (meaning "not instance inside")
  // 2) otherwise, it contains an instance inside
  // important! no int / string[] / tuple(int): only if it has instances! (so that it will help -> resolving)
  const TypeHint *assum_hint{nullptr};

  Assumption() = default;                           // create in undefined state
  explicit Assumption(const TypeHint *type_hint);   // create from type hint (save it if has instances)
  explicit Assumption(ClassPtr klass);

  std::string as_human_readable() const;

  explicit operator bool() const { return assum_hint != nullptr; }

  ClassPtr try_as_class() const { return extract_instance_from_type_hint(assum_hint); }
  Assumption get_inner_if_array() const;
  Assumption get_subkey_by_index(VertexPtr index_key) const;

  static ClassPtr extract_instance_from_type_hint(const TypeHint *type_hint);
};


void assumption_add_for_var(FunctionPtr f, const std::string &var_name, const Assumption &assumption, VertexPtr v_location);

Assumption assume_class_of_expr(FunctionPtr f, VertexPtr root, VertexPtr stop_at);
