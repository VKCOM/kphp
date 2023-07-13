// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

#include "runtime/curl.h"

Optional<string> f$curl_exec_concurrently(curl_easy easy_id, double timeout_s = 1.0);
