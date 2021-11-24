// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mysql/mysql.h>

#include "runtime/kphp_core.h"
#include "runtime/pdo/abstract_pdo_statement.h"

class MysqlConnector;

namespace pdo::mysql {
class MysqlPdoEmulatedStatement : public pdo::AbstractPdoStatement {
public:
  MysqlPdoEmulatedStatement(MysqlConnector *connector, const string &statement);

  bool execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept final;
  mixed fetch(const class_instance<C$PDOStatement> &v$this, int mode = 42, int cursorOrientation = 42, int cursorOffset = 0) noexcept final;

private:
  MysqlConnector *connector;
  MYSQL_RES *mysql_res{nullptr};
  string statement;
};
} // namespace pdo::mysql
