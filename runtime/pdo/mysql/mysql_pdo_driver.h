// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime/allocator.h"
#include "runtime/pdo/abstract_pdo_driver.h"

namespace pdo::mysql {

class MysqlPdoDriver : public pdo::AbstractPdoDriver {
public:
  MysqlPdoDriver() = default;

  void connect(const class_instance<C$PDO> &pdo_instance, const string &connection_string,
               const Optional<string> &username, const Optional<string> &password, const Optional<array<mixed>> &options) noexcept final;
  class_instance<C$PDOStatement> prepare(const class_instance<C$PDO> &v$this, const string &query, const array<mixed> &options) noexcept final;
  virtual const char *error_code_sqlstate() noexcept final;
  virtual std::pair<int, const char *> error_info() noexcept final;
};

} // namespace pdo::mysql
