// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/serialization/msgpack-functions.h"

template<class ResultType = mixed>
ResultType f$msgpack_deserialize(const string& buffer, string* out_err_msg = nullptr) noexcept {
  if (buffer.empty()) {
    return {};
  }

  RuntimeContext::get().msgpack_error = std::nullopt;
  string err_msg;
  vk::msgpack::unpacker unpacker{buffer};
  vk::msgpack::object obj{unpacker.unpack()};

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

template<class ResultClass>
ResultClass f$instance_deserialize(const string& buffer, const string&) noexcept {
  return f$msgpack_deserialize<ResultClass>(buffer);
}

template<class InstanceClass>
string f$instance_serialize_safe(const class_instance<InstanceClass>& instance) noexcept {
  php_critical_error("call to unsupported function");
}

template<class ResultClass>
ResultClass f$instance_deserialize_safe(const string& buffer, const string&) noexcept {
  php_critical_error("call to unsupported function");
}
