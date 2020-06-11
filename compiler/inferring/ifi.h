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
  ifi_is_float = 1 << 7,
  ifi_is_string = 1 << 8,
  ifi_is_array = 1 << 9,
  ifi_is_object = 1 << 10,
  ifi_is_false = 1 << 11,
};

constexpr is_func_id_t ifi_any_type = static_cast<is_func_id_t>(
  ifi_is_bool | ifi_is_null | ifi_is_integer |
  ifi_is_float | ifi_is_string |
  ifi_is_array | ifi_is_object | ifi_is_false
  );

is_func_id_t get_ifi_id(VertexPtr v);
