// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/superglobals.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/json-functions.h"

void init_http_superglobals(const char * buffer, int size) {
  ComponentState & ctx = * get_component_context();
  string query = string(buffer, size);
  mixed http = f$json_decode(query, true);
  ctx.php_script_mutable_globals_singleton.get_superglobals().v$_SERVER.set_value(string("QUERY_TYPE"), string("http"));
  ctx.php_script_mutable_globals_singleton.get_superglobals().v$_POST = http;
}
