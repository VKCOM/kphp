// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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

constexpr int ifi_any_type = (
  ifi_is_bool | ifi_is_null | ifi_is_integer |
  ifi_is_float | ifi_is_string |
  ifi_is_array | ifi_is_object | ifi_is_false
);

is_func_id_t get_ifi_id(VertexPtr v);
