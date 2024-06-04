#pragma once

#include "runtime/runtime-types.h"
#include "runtime/pdo/abstract_pdo_statement.h"

namespace database_drivers {
class PgsqlConnector;
class PgsqlResponse;
} // namespace database_drivers

namespace pdo::pgsql {
class PgsqlPdoEmulatedStatement : public pdo::AbstractPdoStatement {
public:
  PgsqlPdoEmulatedStatement(const string &statement, int connector_id);

  bool execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept final;
  mixed fetch(const class_instance<C$PDOStatement> &v$this) noexcept final;
  int64_t affected_rows() noexcept final;

private:
  string statement;
  int processed_row{-1};
  int connector_id{};

  std::unique_ptr<database_drivers::PgsqlResponse> response;

  class ExecuteResumable;
};
} // namespace pdo::pgsql
