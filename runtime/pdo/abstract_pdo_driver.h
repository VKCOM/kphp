// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "kphp-core/kphp_core.h"
#include "runtime/allocator.h"

struct C$PDO;
struct C$PDOStatement;

namespace pdo {

class AbstractPdoDriver : public ::ManagedThroughDlAllocator {
public:
  int connector_id;

  AbstractPdoDriver() = default;
  virtual ~AbstractPdoDriver() noexcept;

  virtual void connect(const class_instance<C$PDO> &pdo_instance, const string &connection_string,
                       const Optional<string> &username, const Optional<string> &password, const Optional<array<mixed>> &options) noexcept = 0;
  virtual class_instance<C$PDOStatement> prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept = 0;
  virtual const char *error_code_sqlstate() noexcept = 0;
  virtual std::pair<int, const char *> error_info() noexcept = 0;
};

} // namespace pdo
