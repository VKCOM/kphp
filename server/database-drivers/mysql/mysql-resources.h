// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mysql/mysql.h>

namespace database_drivers {

/**
 * As mysql lib uses heap allocator we must guarantee that all allocated resources will be freed on script termination or earlier to prevent memory leaks.
 * In case of timeouts script execution is interrupted by signal and no destructors are called.
 * So we need to clean up all left resources manually, @see free_mysql_resources().
 */

void register_mysql_response(MYSQL_RES *res);
void remove_mysql_response(MYSQL_RES *res);
void free_mysql_resources();

} // namespace database_drivers
