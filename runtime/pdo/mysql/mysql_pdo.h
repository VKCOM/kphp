// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mysql/mysql.h>

#include "runtime/allocator.h"
#include "server/external-net-drivers/mysql/mysql.h"

namespace pdo::mysql {
inline void init_lib() {
  LIB_MYSQL_CALL(mysql_library_init(0, nullptr, nullptr));
}

inline void free_lib() {
  LIB_MYSQL_CALL(mysql_library_end());
}
} // namespace pdo::mysql
