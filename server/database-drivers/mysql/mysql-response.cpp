// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/mysql/mysql-response.h"

#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/mysql/mysql-connector.h"
#include "server/database-drivers/mysql/mysql-resources.h"
#include "server/database-drivers/mysql/mysql.h"

namespace database_drivers {

AsyncOperationStatus MysqlResponse::fetch_async() noexcept {
  auto *connector = vk::singleton<database_drivers::Adaptor>::get().get_connector<MysqlConnector>(connector_id);
  if (connector == nullptr) {
    is_error = true;
    return AsyncOperationStatus::ERROR;
  }

  if (!got_columns_info) {
    bool error = LIB_MYSQL_CALL(mysql_read_query_result(connector->ctx));
    if (error) {
      is_error = true;
      return AsyncOperationStatus::ERROR;
    }
    tvkprintf(mysql, 1, "MySQL fetch response: request_id = %d, get columns info\n", bound_request_id);
    got_columns_info = true;
  }

  dl::CriticalSectionGuard guard; // guarantee that MYSQL_RES will be registered to prevent memory leak on script timeouts
  net_async_status status = LIB_MYSQL_CALL(mysql_store_result_nonblocking(connector->ctx, &res)); // buffered queries
  tvkprintf(mysql, 1, "MySQL fetch response: request_id = %d, get result set, status = %d\n", bound_request_id, status);
  switch (status) {
    case NET_ASYNC_COMPLETE: {
      if (res != nullptr) {
        register_mysql_response(res);
        return AsyncOperationStatus::COMPLETED;
      } else {
        if (LIB_MYSQL_CALL(mysql_field_count(connector->ctx)) == 0) {
          affected_rows = LIB_MYSQL_CALL(mysql_affected_rows(connector->ctx));
          return AsyncOperationStatus::COMPLETED;
        } else {
          is_error = true;
          return AsyncOperationStatus::ERROR;
        }
      }
    }
    case NET_ASYNC_ERROR: {
      is_error = true;
      return AsyncOperationStatus::ERROR;
    }
    case NET_ASYNC_NOT_READY: {
      return AsyncOperationStatus::IN_PROGRESS;
    }
    default: {
      is_error = true;
      return AsyncOperationStatus::ERROR;
    }
  }
}

MysqlResponse::~MysqlResponse() {
  if (res) {
    remove_mysql_response(res);
  }
}

} // namespace database_drivers
