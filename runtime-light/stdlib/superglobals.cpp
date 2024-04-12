#include "runtime-light/stdlib/superglobals.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/json-functions.h"

void init_http_superglobals(const char * buffer, int size) {
  ComponentState & ctx = * get_component_context();
  string query = string(buffer, size);
  mixed globals = f$json_decode(query, true);
  ctx.php_script_mutable_globals_singleton.get_superglobals().v$_SERVER.set_value(string("QUERY"), string("http"));
  ctx.php_script_mutable_globals_singleton.get_superglobals().v$_POST = globals;
}

void init_component_superglobals() {
  ComponentState & ctx = * get_component_context();
  ctx.php_script_mutable_globals_singleton.get_superglobals().v$_SERVER.set_value(string("QUERY"), string("component"));
}