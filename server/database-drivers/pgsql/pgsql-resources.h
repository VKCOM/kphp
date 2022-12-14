#pragma once

#include <postgresql/libpq-fe.h>

namespace database_drivers {

void register_pgsql_response(PGresult *res);
void remove_pgsql_response(PGresult *res);
void free_pgsql_resources();

} // namespace database_drivers
