// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime/allocator.h"

struct C$PDO;
struct C$PDOStatement;

namespace pdo {

class AbstractPdoStatement : public ::ManagedThroughDlAllocator {
public:
  AbstractPdoStatement() = default;
  virtual ~AbstractPdoStatement() = default;

  virtual bool execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept = 0;
  virtual mixed fetch(const class_instance<C$PDOStatement> &v$this) noexcept = 0;
  virtual int64_t affected_rows() noexcept = 0;
};

} // namespace pdo
