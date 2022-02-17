// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/mysql/mysql.h"
#include "server/database-drivers/mysql/mysql-resources.h"

DEFINE_VERBOSITY(mysql);

namespace database_drivers {

void free_mysql_lib() {
  free_mysql_resources();
}

} // namespace database_drivers
