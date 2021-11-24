// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/pdo/mysql/mysql_pdo_statement.h"
#include "runtime/kphp_core.h"
#include "runtime/pdo/mysql/mysql_pdo.h"
#include "runtime/resumable.h"
#include "server/external-net-drivers/mysql/mysql-connector.h"
#include "server/external-net-drivers/mysql/mysql-request.h"
#include "server/external-net-drivers/mysql/mysql-response.h"
#include "server/external-net-drivers/net-drivers-adaptor.h"

pdo::mysql::MysqlPdoEmulatedStatement::MysqlPdoEmulatedStatement(MysqlConnector *connector, const string &statement)
  : connector(connector)
  , statement(statement) {}

bool pdo::mysql::MysqlPdoEmulatedStatement::execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  (void)v$this, (void)params; // TODO: use params

  int resumable_id = vk::singleton<NetDriversAdaptor>::get().send_request(connector, new MysqlRequest{connector, statement});

  auto *response = f$wait<Response *, false>(resumable_id);
  if (auto *casted = dynamic_cast<MysqlResponse *>(response)) {
    mysql_res = casted->res;
  } else {
    php_critical_error("Got response of incorrect type from resumable in MySQL PDO::execute");
  }
  array<int> x;
  return true;
}

mixed pdo::mysql::MysqlPdoEmulatedStatement::fetch(const class_instance<C$PDOStatement> &v$this, int mode, int cursorOrientation, int cursorOffset) noexcept {
  (void) v$this, (void) mode, (void) cursorOrientation, (void) cursorOffset;
  MYSQL_ROW row = LIB_MYSQL_CALL(mysql_fetch_row(mysql_res));
  if (row == nullptr) {
    return {};
  }
  array<mixed> res;
  unsigned int fields_num = LIB_MYSQL_CALL(mysql_num_fields(mysql_res));
  for (int i = 0; i < fields_num; ++i) {
    MYSQL_FIELD *cur_field = LIB_MYSQL_CALL(mysql_fetch_field_direct(mysql_res, i));
    const char *cur_col = row[i];
    res.set_value(i, string{cur_col});
    res.set_value(string{cur_field->name}, string{cur_col});
  }
  return res;
}
