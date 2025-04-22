// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/serialization/msgpack-functions.h"

#include "common/containers/final_action.h"

#include "runtime/context/runtime-context.h"
#include "runtime/critical_section.h"
#include "runtime/exception.h"
#include "runtime/interface.h"
#include "runtime/string_functions.h"

template<class T>
inline string f$msgpack_serialize_safe(const T& value) noexcept {
  string err_msg;
  auto res = f$msgpack_serialize(value, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return res.val();
}

template<class InstanceClass>
inline string f$instance_serialize_safe(const class_instance<InstanceClass>& instance) noexcept {
  string err_msg;
  auto result = msgpack_functions_impl_::common_instance_serialize(instance, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return result.val();
}

/**
 * this function works nice with POSIX signal despite of exceptions
 * due to malloc_replacement_guard exceptions will be allocated in script memory
 * and we don't need critical_section here.
 *
 * For better understanding exceptions please look through this article
 * https://monoinfinito.wordpress.com/series/exception-handling-in-c/
 */
template<class ResultType = mixed>
inline ResultType f$msgpack_deserialize(const string& buffer, string* out_err_msg = nullptr) noexcept {
  if (buffer.empty()) {
    return {};
  }

  SerializationLibContext::get().clear_msgpack_error();
  const auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator();
  string err_msg;
  vk::msgpack::unpacker unpacker{buffer};
  vk::msgpack::object obj = unpacker.unpack();

  if (unpacker.has_error()) {
    // parsing or allocation errors
    err_msg = unpacker.get_error_msg();
  } else {
    auto res = obj.as<ResultType>();
    if (unpacker.has_error()) {
      // scheme compliance errors
      err_msg = unpacker.get_error_msg();
    } else {
      return res;
    }
  }

  if (!err_msg.empty()) {
    if (out_err_msg) {
      *out_err_msg = std::move(err_msg);
    } else {
      f$warning(err_msg);
    }
    return {};
  };

  return {};
}

template<class ResultType = mixed>
inline ResultType f$msgpack_deserialize_safe(const string& buffer) noexcept {
  string err_msg;
  auto res = f$msgpack_deserialize(buffer, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return res;
}

template<class ResultClass>
inline ResultClass f$instance_deserialize(const string& buffer, const string&) noexcept {
  return f$msgpack_deserialize<ResultClass>(buffer);
}

template<class ResultClass>
inline ResultClass f$instance_deserialize_safe(const string& buffer, const string&) noexcept {
  string err_msg;
  auto res = f$msgpack_deserialize<ResultClass>(buffer, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return res;
}
