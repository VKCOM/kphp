// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

enum class QueryType { HTTP, COMPONENT };

void init_http_superglobals(const char *buffer, int size);
