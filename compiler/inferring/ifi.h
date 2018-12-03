#pragma once

#include "compiler/data/vertex-adaptor.h"

enum is_func_id_t {
  ifi_error = -1,
  ifi_unset = 1,
  ifi_isset = 1 << 1,
  ifi_is_bool = 1 << 2,
  ifi_is_numeric = 1 << 3,
  ifi_is_scalar = 1 << 4,
  ifi_is_null = 1 << 5,
  ifi_is_integer = 1 << 6,
  ifi_is_long = 1 << 7,
  ifi_is_float = 1 << 8,
  ifi_is_string = 1 << 9,
  ifi_is_array = 1 << 10,
  ifi_is_object = 1 << 11
};

is_func_id_t get_ifi_id(VertexPtr v);
