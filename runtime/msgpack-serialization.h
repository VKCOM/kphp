// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/msgpack/adaptors.h"
#include "runtime/msgpack/packer.h"
#include "runtime/msgpack/unpacker.h"
#include "runtime/msgpack/unpack_exception.h"

#include "common/containers/final_action.h"

#include "runtime/runtime-types.h"
#include "runtime/context/runtime-context.h"
#include "runtime/critical_section.h"
#include "runtime/exception.h"
#include "runtime/interface.h"
#include "runtime/string_functions.h"

template<class T>
inline Optional<string> f$msgpack_serialize(const T &value, string *out_err_msg = nullptr) noexcept {
  f$ob_start();
  php_assert(f$ob_get_length().has_value() && f$ob_get_length().val() == 0);

  kphp_runtime_context.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  auto clean_buffer = vk::finally([] {
    f$ob_end_clean();
    kphp_runtime_context.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  });

  vk::msgpack::packer{*coub}.pack(value);

  if (kphp_runtime_context.sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    string err_msg{"msgpacke_serialize buffer overflow"};
    if (out_err_msg) {
      *out_err_msg = std::move(err_msg);
    } else {
      f$warning(err_msg);
    }
    return {};
  }

  return coub->str<ScriptAllocator>();
}

template<class T>
inline string f$msgpack_serialize_safe(const T &value) noexcept {
  string err_msg;
  auto res = f$msgpack_serialize(value, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return res.val();
}
template<class InstanceClass>
inline Optional<string> common_instance_serialize(const class_instance<InstanceClass> &instance, string *out_err_msg) noexcept {
  vk::msgpack::packer_float32_decorator::clear();
  vk::msgpack::CheckInstanceDepth::depth = 0;
  auto result = f$msgpack_serialize(instance, out_err_msg);
  if (vk::msgpack::CheckInstanceDepth::is_exceeded()) {
    *out_err_msg = string("maximum depth of nested instances exceeded");
    return {};
  } else if (!out_err_msg->empty()) {
    return {};
  }
  return result;
}

template<class InstanceClass>
inline Optional<string> f$instance_serialize(const class_instance<InstanceClass> &instance) noexcept {
  string err_msg;
  auto result = common_instance_serialize(instance, &err_msg);
  if (!err_msg.empty()) {
    f$warning(err_msg);
    return {};
  }
  return result;
}

template<class InstanceClass>
inline string f$instance_serialize_safe(const class_instance<InstanceClass> &instance) noexcept {
  string err_msg;
  auto result = common_instance_serialize(instance, &err_msg);
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
inline ResultType f$msgpack_deserialize(const string &buffer, string *out_err_msg = nullptr) noexcept {
  if (buffer.empty()) {
    return {};
  }

  const auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator();
  string err_msg;
  try {
    vk::msgpack::unpacker unpacker{buffer};
    vk::msgpack::object obj = unpacker.unpack();

    if (unpacker.has_error()) {
      err_msg = unpacker.get_error_msg();
    } else {
      return obj.as<ResultType>();
    }
  } catch (vk::msgpack::type_error &e) {
    err_msg = string("Unknown type found during deserialization");
  } catch (vk::msgpack::unpack_error &e) {
    err_msg = string(e.what());
  } catch (...) {
    err_msg = string("something went wrong in deserialization, pass it to KPHP|Team");
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
inline ResultType f$msgpack_deserialize_safe(const string &buffer) noexcept {
  string err_msg;
  auto res = f$msgpack_deserialize(buffer, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return res;
}

template<class ResultClass>
inline ResultClass f$instance_deserialize(const string &buffer, const string&) noexcept {
  return f$msgpack_deserialize<ResultClass>(buffer);
}

template<class ResultClass>
inline ResultClass f$instance_deserialize_safe(const string &buffer, const string&) noexcept {
  string err_msg;
  auto res = f$msgpack_deserialize<ResultClass>(buffer, &err_msg);
  if (!err_msg.empty()) {
    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, err_msg));
    return {};
  }
  return res;
}
