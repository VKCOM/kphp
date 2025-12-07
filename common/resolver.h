// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <netdb.h>

extern int kdb_hosts_loaded;
int kdb_load_hosts();

struct hostent* kdb_gethostbyname(const char* name);
const char* kdb_gethostname();
