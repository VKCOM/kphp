#include "server/database-drivers/pgsql/pgsql-resources.h"

#include <unordered_set>

#include "runtime/critical_section.h"
#include "server/database-drivers/pgsql/pgsql.h"

namespace {

std::unordered_set<PGresult *> pgsql_responses_to_free;

} // namespace

namespace database_drivers {

void register_pgsql_response(PGresult *res) {
  dl::CriticalSectionGuard guard;

  pgsql_responses_to_free.insert(res);
  tvkprintf(pgsql, 2, "pgSQL response %p registered\n", res);
}

void remove_pgsql_response(PGresult *res) {
  dl::CriticalSectionGuard guard;

  if (pgsql_responses_to_free.erase(res)) {
    LIB_PGSQL_CALL(PQclear(res));
    tvkprintf(pgsql, 2, "pgSQL response %p removed\n", res);
  }
}

void free_pgsql_resources() {
  dl::CriticalSectionGuard guard;

  for (auto *res : pgsql_responses_to_free) {
    LIB_PGSQL_CALL(PQclear(res));
    tvkprintf(pgsql, 2, "pgSQL response %p freed on script termination\n", res);
  }
  pgsql_responses_to_free.clear();
}

} // namespace database_drivers
