// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <mysql/mysql.h>

#include "runtime/pdo/mysql/mysql_pdo_emulated_statement.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/resumable.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/mysql/mysql-request.h"
#include "server/database-drivers/mysql/mysql-response.h"
#include "server/database-drivers/mysql/mysql.h"

namespace pdo::mysql {

class MysqlPdoEmulatedStatement::ExecuteResumable final : public Resumable {
private:
  MysqlPdoEmulatedStatement *ctx{};
  int64_t timeout_sec{-1};
  std::unique_ptr<database_drivers::Response> response{};
  int resumable_id{};

public:
  using ReturnT = bool;
  explicit ExecuteResumable(MysqlPdoEmulatedStatement *ctx, int64_t timeout_sec) noexcept
    : ctx(ctx)
    , timeout_sec(timeout_sec) {}
  bool run() noexcept final {
    RESUMABLE_BEGIN
    resumable_id = vk::singleton<database_drivers::Adaptor>::get().launch_request_resumable(
      std::make_unique<database_drivers::MysqlRequest>(ctx->connector_id, ctx->statement));
    response = vk::singleton<database_drivers::Adaptor>::get().wait_request_resumable(resumable_id, timeout_sec);
    TRY_WAIT(MysqlPdoEmulatedStatement_ExecuteResumable_label, response, std::unique_ptr<database_drivers::Response>);
    if (auto *casted = dynamic_cast<database_drivers::MysqlResponse *>(response.get())) {
      ctx->response = std::unique_ptr<database_drivers::MysqlResponse>{casted};
      response.release();
    } else {
      php_critical_error("Unexpected error at MySQL PDO::execute");
    }
    RETURN(!ctx->response->is_error);
    RESUMABLE_END
  }
};

MysqlPdoEmulatedStatement::MysqlPdoEmulatedStatement(const string &statement, int connector_id)
  : statement(statement)
  , connector_id(connector_id) {}

bool MysqlPdoEmulatedStatement::execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  (void)v$this, (void)params; // TODO: use params
  return start_resumable<bool>(new ExecuteResumable(this, v$this.get()->timeout_sec));
}

mixed MysqlPdoEmulatedStatement::fetch(const class_instance<C$PDOStatement> &) noexcept {
  MYSQL_RES *mysql_res = response->res;
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

int64_t MysqlPdoEmulatedStatement::affected_rows() noexcept {
  return response->affected_rows;
}

} // namespace pdo::mysql
