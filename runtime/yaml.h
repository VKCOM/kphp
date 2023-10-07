#pragma once

#include "runtime/kphp_core.h"

bool f$yaml_emit_file(const string &filename, const mixed &data);

string f$yaml_emit(const mixed &data);

mixed f$yaml_parse_file(const string &filename, int pos = 0);

mixed f$yaml_parse(const string &data, int pos = 0);
