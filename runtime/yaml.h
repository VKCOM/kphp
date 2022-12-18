#pragma once

#include "runtime/kphp_core.h"

/*
 * emit mixed into a yaml file
 */
bool f$yaml_emit_file(const string &filename, const mixed &data);

/*
 * emit mixed into a yaml string
 */
string f$yaml_emit(const mixed &data);

/*
 * parse yaml file into mixed
 */
mixed f$yaml_parse_file(const string &filename, int pos = 0);

/*
 * parse yaml string into mixed
 */
mixed f$yaml_parse(const string &data, int pos = 0);