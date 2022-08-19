#include "server/database-drivers/pgsql/pgsql.h"
#include "server/database-drivers/pgsql/pgsql-resources.h"

DEFINE_VERBOSITY(pgsql);

namespace database_drivers {

void free_pgsql_lib() {
  free_pgsql_resources();
}

} // namespace database_drivers
