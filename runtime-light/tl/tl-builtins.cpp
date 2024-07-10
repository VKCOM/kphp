//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-builtins.h"

void register_tl_storers_table_and_fetcher(const array<tl_storer_ptr> &gen$ht, tl_fetch_wrapper_ptr gen$t_ReqResult_fetch) {
  auto &rpc_mutable_image_state{RpcImageState::get_mutable()};
  rpc_mutable_image_state.tl_storers_ht = gen$ht;
  rpc_mutable_image_state.tl_fetch_wrapper = gen$t_ReqResult_fetch;
}

int32_t tl_parse_save_pos() {
  return static_cast<int32_t>(RpcComponentContext::get().rpc_buffer.pos());
}

bool tl_parse_restore_pos(int32_t pos) {
  auto &rpc_buf{RpcComponentContext::get().rpc_buffer};
  if (pos < 0 || pos > rpc_buf.pos()) {
    return false;
  }
  rpc_buf.reset(pos);
  return true;
}

mixed tl_arr_get(const mixed &arr, const string &str_key, int64_t num_key, int64_t precomputed_hash) {
  auto &cur_query{CurrentTlQuery::get()};
  if (!arr.is_array()) {
    cur_query.raise_storing_error("Array expected, when trying to access field #%" PRIi64 " : %s", num_key, str_key.c_str());
    return {};
  }

  if (const auto &elem{arr.get_value(num_key)}; !elem.is_null()) {
    return elem;
  }
  if (const auto &elem{precomputed_hash == 0 ? arr.get_value(str_key) : arr.get_value(str_key, precomputed_hash)}; !elem.is_null()) {
    return elem;
  }

  cur_query.raise_storing_error("Field %s (#%" PRIi64 ") not found", str_key.c_str(), num_key);
  return {};
}

void store_magic_if_not_bare(uint32_t inner_magic) {
  if (static_cast<bool>(inner_magic)) {
    f$store_int(inner_magic);
  }
}

void fetch_magic_if_not_bare(uint32_t inner_magic, const char *error_msg) {
  if (static_cast<bool>(inner_magic)) {
    const auto actual_magic = static_cast<uint32_t>(f$fetch_int());
    if (actual_magic != inner_magic) {
      CurrentTlQuery::get().raise_fetching_error("%s\nExpected 0x%08x, but fetched 0x%08x", error_msg, inner_magic, actual_magic);
    }
  }
}

void t_Int::store(const mixed &tl_object) {
  int32_t v32{prepare_int_for_storing(f$intval(tl_object))};
  f$store_int(v32);
}

int32_t t_Int::fetch() {
  //    CHECK_EXCEPTION(return 0);
  return f$fetch_int();
}

void t_Int::typed_store(const t_Int::PhpType &v) {
  int32_t v32{prepare_int_for_storing(v)};
  f$store_int(v32);
}

void t_Int::typed_fetch_to(t_Int::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  out = f$fetch_int();
}

int32_t t_Int::prepare_int_for_storing(int64_t v) {
  auto v32 = static_cast<int32_t>(v);
  if (is_int32_overflow(v)) {
    // TODO
  }
  return v32;
}

void t_Long::store(const mixed &tl_object) {
  int64_t v64{f$intval(tl_object)};
  f$store_long(v64);
}

mixed t_Long::fetch() {
  //    CHECK_EXCEPTION(return mixed());
  return f$fetch_long();
}

void t_Long::typed_store(const t_Long::PhpType &v) {
  f$store_long(v);
}

void t_Long::typed_fetch_to(t_Long::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  out = f$fetch_long();
}

void t_Double::store(const mixed &tl_object) {
  f$store_double(f$floatval(tl_object));
}

double t_Double::fetch() {
  //    CHECK_EXCEPTION(return 0);
  return f$fetch_double();
}

void t_Double::typed_store(const t_Double::PhpType &v) {
  f$store_double(v);
}

void t_Double::typed_fetch_to(t_Double::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  out = f$fetch_double();
}

void t_Float::store(const mixed &tl_object) {
  f$store_float(f$floatval(tl_object));
}

double t_Float::fetch() {
  //    CHECK_EXCEPTION(return 0);
  return f$fetch_float();
}

void t_Float::typed_store(const t_Float::PhpType &v) {
  f$store_float(v);
}

void t_Float::typed_fetch_to(t_Float::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  out = f$fetch_float();
}

void t_String::store(const mixed &tl_object) {
  f$store_string(f$strval(tl_object));
}

string t_String::fetch() {
  //    CHECK_EXCEPTION(return tl_str_);
  return f$fetch_string();
}

void t_String::typed_store(const t_String::PhpType &v) {
  f$store_string(f$strval(v));
}

void t_String::typed_fetch_to(t_String::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  out = f$fetch_string();
}

void t_Bool::store(const mixed &tl_object) {
  f$store_int(tl_object.to_bool() ? TL_BOOL_TRUE : TL_BOOL_FALSE);
}

bool t_Bool::fetch() {
  //    CHECK_EXCEPTION(return false);
  const auto magic = static_cast<uint32_t>(f$fetch_int());
  switch (magic) {
    case TL_BOOL_FALSE:
      return false;
    case TL_BOOL_TRUE:
      return true;
    default: {
      CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
      return false;
    }
  }
}

void t_Bool::typed_store(const t_Bool::PhpType &v) {
  f$store_int(v ? TL_BOOL_TRUE : TL_BOOL_FALSE);
}

void t_Bool::typed_fetch_to(t_Bool::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  const auto magic = static_cast<uint32_t>(f$fetch_int());
  if (magic != TL_BOOL_TRUE && magic != TL_BOOL_FALSE) {
    CurrentTlQuery::get().raise_fetching_error("Incorrect magic of type Bool: 0x%08x", magic);
    return;
  }
  out = magic == TL_BOOL_TRUE;
}

void t_True::store([[maybe_unused]] const mixed &v) {}

array<mixed> t_True::fetch() {
  return {};
}

void t_True::typed_store([[maybe_unused]] const t_True::PhpType &v) {}

void t_True::typed_fetch_to(t_True::PhpType &out) {
  //    CHECK_EXCEPTION(return);
  out = true;
}
