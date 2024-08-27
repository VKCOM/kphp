//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

task_t<Optional<string>> f$openssl_random_pseudo_bytes(int64_t length);
task_t<Optional<array<mixed>>> f$openssl_x509_parse(const string &data, bool shortnames = true);
