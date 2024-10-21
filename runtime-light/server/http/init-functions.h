// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/tl/tl-functions.h"

void init_http_server(tl::K2InvokeHttp &&invoke_http) noexcept;
