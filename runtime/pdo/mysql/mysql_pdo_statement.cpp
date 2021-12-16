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

namespace pdo::mysql {

class MysqlPdoEmulatedStatement::ExecuteResumable final : public Resumable {
private:
  MysqlPdoEmulatedStatement *ctx{}; // TODO: raii?
  Response *response{};
  int resumable_id{};
public:
  using ReturnT = bool;
  explicit ExecuteResumable(MysqlPdoEmulatedStatement *ctx) noexcept : ctx(ctx)  {}
  bool run() noexcept final {
    RESUMABLE_BEGIN
      resumable_id = vk::singleton<NetDriversAdaptor>::get().send_request(ctx->connector_id, std::make_unique<MysqlRequest>(ctx->connector_id, ctx->statement));
      response = f$wait<Response *, false>(resumable_id);
      TRY_WAIT(MysqlPdoEmulatedStatement_ExecuteResumable_label, response, Response *);
      if (auto *casted = dynamic_cast<MysqlResponse *>(response)) {
        ctx->mysql_res = casted->res;
      } else {
        php_critical_error("Got response of incorrect type from resumable in MySQL PDO::execute");
      }
      RETURN(true);
    RESUMABLE_END
  }
};

MysqlPdoEmulatedStatement::MysqlPdoEmulatedStatement(const string &statement, int connector_id)
  : statement(statement)
  , connector_id(connector_id) {}

bool MysqlPdoEmulatedStatement::execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  (void)v$this, (void)params; // TODO: use params
  return start_resumable<bool>(new ExecuteResumable(this));
}

mixed MysqlPdoEmulatedStatement::fetch(const class_instance<C$PDOStatement> &v$this, int mode, int cursorOrientation, int cursorOffset) noexcept {
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

} // namespace pdo::mysql
