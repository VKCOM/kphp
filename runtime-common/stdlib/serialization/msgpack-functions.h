// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/containers/final_action.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/msgpack/packer.h"
#include "runtime-common/stdlib/msgpack/unpack_exception.h"
#include "runtime-common/stdlib/msgpack/unpacker.h"
#include "runtime-common/stdlib/msgpack/adaptors.h"
#include "runtime-common/stdlib/msgpack/check_instance_depth.h"

template<class T>
Optional<string> f$msgpack_serialize(const T &value, string *out_err_msg = nullptr) noexcept {
  auto &str_buffer{RuntimeContext::get().static_SB.clean()};
  php_assert(str_buffer.size() == 0);

  RuntimeContext::get().sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  auto clean_buffer = vk::finally([&str_buffer] {
    str_buffer.clean();
    RuntimeContext::get().sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  });

  vk::msgpack::packer{str_buffer}.pack(value);

  if (RuntimeContext::get().sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    string err_msg{"msgpacke_serialize buffer overflow"};
    if (out_err_msg) {
      *out_err_msg = std::move(err_msg);
    } else {
      f$warning(err_msg);
    }
    return {};
  }

  return str_buffer.str();
}

namespace msgpack_functions_impl {
template<class InstanceClass>
inline Optional<string> common_instance_serialize(const class_instance<InstanceClass> &instance, string *out_err_msg) noexcept {
  vk::msgpack::packer_float32_decorator::clear();
  SerializationLibContext::get().check_instance_depth = 0;
  auto result = f$msgpack_serialize(instance, out_err_msg);
  if (vk::msgpack::CheckInstanceDepth::is_exceeded()) {
    *out_err_msg = string("maximum depth of nested instances exceeded");
    return {};
  } else if (!out_err_msg->empty()) {
    return {};
  }
  return result;
}
} // namespace msgpack_functions_impl

template<class InstanceClass>
Optional<string> f$instance_serialize(const class_instance<InstanceClass> &instance) noexcept {
  string err_msg;
  auto result = msgpack_functions_impl::common_instance_serialize(instance, &err_msg);
  if (!err_msg.empty()) {
    f$warning(err_msg);
    return {};
  }
  return result;
}
