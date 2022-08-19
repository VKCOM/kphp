#pragma once
#include <postgresql/libpq-fe.h>

// Helper structure for storing the error message and error code
struct PGSQL {
  PGSQL() = default;

  void remember_result_status(PGresult *res) {
    char *result_error = PQresultErrorField(res, PG_DIAG_SQLSTATE);
    sqlstate = result_error ? string{result_error} : string{""};
    last_result_status = LIB_PGSQL_CALL(PQresultStatus(res));
  }

  PGconn *conn{nullptr};
  string sqlstate{""};
  int last_result_status{0};
};
