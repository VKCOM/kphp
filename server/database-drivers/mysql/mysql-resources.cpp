// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/database-drivers/mysql/mysql-resources.h"

#include <unordered_set>

#include "runtime/critical_section.h"
#include "server/database-drivers/mysql/mysql.h"

namespace {

std::unordered_set<MYSQL_RES *> mysql_responses_to_free; // all operations are in the critical section

} // namespace

namespace database_drivers {

void register_mysql_response(MYSQL_RES *res) {
  dl::CriticalSectionGuard guard;

  mysql_responses_to_free.insert(res);
  tvkprintf(mysql, 2, "MySQL response %p registered\n", res);
}

void remove_mysql_response(MYSQL_RES *res) {
  dl::CriticalSectionGuard guard;

  if (mysql_responses_to_free.erase(res)) {
    LIB_MYSQL_CALL(mysql_free_result(res));
    tvkprintf(mysql, 2, "MySQL response %p removed\n", res);
  }
}

void free_mysql_resources() {
  dl::CriticalSectionGuard guard;

  for (auto *res : mysql_responses_to_free) {
    LIB_MYSQL_CALL(mysql_free_result(res));
    tvkprintf(mysql, 2, "MySQL response %p freed on script termination\n", res);
  }
  mysql_responses_to_free.clear();
}

} // namespace database_drivers
