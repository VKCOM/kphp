#pragma once

#include <string>

enum PrimitiveType {
  tp_Unknown,
  tp_False,
  tp_bool,
  tp_int,
  tp_float,
  tp_array,
  tp_string,
  tp_void,
  tp_var,
  tp_UInt,
  tp_Long,
  tp_ULong,
  tp_RPC,
  tp_tuple,
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
PrimitiveType get_ptype_by_name(const std::string &s);
PrimitiveType type_lca(PrimitiveType a, PrimitiveType b);
bool can_store_false(PrimitiveType tp);

