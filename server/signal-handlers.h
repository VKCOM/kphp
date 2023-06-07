// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-runner.h"


void perform_error_if_running(const char *msg, script_error_t error_type);

void init_handlers();
