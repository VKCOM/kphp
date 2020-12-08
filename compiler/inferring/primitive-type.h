// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

enum PrimitiveType {
  tp_Unknown,
  tp_Null,
  tp_False,
  tp_bool,
  tp_int,
  tp_float,
  tp_string,
  tp_array,
  tp_mixed,
  tp_void,
  tp_tuple,
  tp_shape,
  tp_future,
  tp_future_queue,
  tp_regexp,
  tp_Class,
  tp_Error,
  tp_Any,
  tp_CreateAny,
  ptype_size
};

const char *ptype_name(PrimitiveType id);
PrimitiveType type_lca(PrimitiveType a, PrimitiveType b);
bool can_store_false(PrimitiveType tp);
bool can_store_null(PrimitiveType tp);

