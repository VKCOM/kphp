// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime/pdo/abstract_pdo_statement.h"

namespace database_drivers {
class MysqlConnector;
class MysqlResponse;
} // namespace database_drivers

namespace pdo::mysql {
class MysqlPdoEmulatedStatement : public pdo::AbstractPdoStatement {
public:
  MysqlPdoEmulatedStatement(const string& statement, int connector_id);

  bool execute(const class_instance<C$PDOStatement>& v$this, const Optional<array<mixed>>& params) noexcept final;
  mixed fetch(const class_instance<C$PDOStatement>& v$this) noexcept final;
  int64_t affected_rows() noexcept final;

private:
  string statement;
  int connector_id{};

  std::unique_ptr<database_drivers::MysqlResponse> response;

  class ExecuteResumable;
};
} // namespace pdo::mysql
