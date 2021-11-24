// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/allocator.h"
#include "runtime/kphp_core.h"

struct C$PDO;
struct C$PDOStatement;

namespace pdo {

class AbstractPdoDriver : public ::ManagedThroughDlAllocator {
public:
  int connection_id;
  bool emulate_prepares = true; // PDO::ATTR_EMULATE_PREPARES is on by default

  AbstractPdoDriver() = default;
  virtual ~AbstractPdoDriver() = default; // TODO: remove connector by connection_id

  virtual void connect(const class_instance<C$PDO> &pdo_instance, const string &connection_string,
                       const Optional<string> &username, const Optional<string> &password, const Optional<array<mixed>> &options) noexcept = 0;
  virtual class_instance<C$PDOStatement> prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept = 0;
};

} // namespace pdo
