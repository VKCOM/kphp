// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sys/utsname.h>

#include "common/mixin/not_copyable.h"
#include "common/php-functions.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/math/math-state.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/string/string-state.h"
#include "runtime-light/stdlib/visitors/shape-visitors.h"
#include "runtime-light/utils/logs.h"

struct ImageState final : private vk::not_copyable {
  RuntimeAllocator allocator;

  char* c_linear_mem{nullptr};
  uint32_t pid{};
  string uname_info_s;
  string uname_info_n;
  string uname_info_r;
  string uname_info_v;
  string uname_info_m;
  string uname_info_a;

  ShapeKeyDemangle shape_key_demangler;

  RpcImageState rpc_image_state;
  StringImageState string_image_state;
  MathImageState math_image_state;

  ImageState() noexcept
      : allocator(INIT_IMAGE_ALLOCATOR_SIZE, 0),
        pid(k2::getpid()) {
    utsname uname_info{};
    if (const auto errc{k2::uname(std::addressof(uname_info))}; errc != k2::errno_ok) [[unlikely]] {
      kphp::log::error("can't get uname, error code: {}", errc);
    }
    uname_info_s = string{uname_info.sysname};
    uname_info_n = string{uname_info.nodename};
    uname_info_r = string{uname_info.release};
    uname_info_v = string{uname_info.version};
    uname_info_m = string{uname_info.machine};
    // +4 for whitespaces
    uname_info_a.reserve_at_least(uname_info_s.size() + uname_info_n.size() + uname_info_r.size() + uname_info_v.size() + uname_info_m.size() + 4);
    uname_info_a.append(uname_info_s);
    uname_info_a.push_back(' ');
    uname_info_a.append(uname_info_n);
    uname_info_a.push_back(' ');
    uname_info_a.append(uname_info_r);
    uname_info_a.push_back(' ');
    uname_info_a.append(uname_info_v);
    uname_info_a.push_back(' ');
    uname_info_a.append(uname_info_m);
    // prevent race condition on reference counter
    uname_info_s.set_reference_counter_to(ExtraRefCnt::for_global_const);
    uname_info_n.set_reference_counter_to(ExtraRefCnt::for_global_const);
    uname_info_r.set_reference_counter_to(ExtraRefCnt::for_global_const);
    uname_info_v.set_reference_counter_to(ExtraRefCnt::for_global_const);
    uname_info_m.set_reference_counter_to(ExtraRefCnt::for_global_const);
    uname_info_a.set_reference_counter_to(ExtraRefCnt::for_global_const);
  }

  static const ImageState& get() noexcept {
    return *k2::image_state();
  }

  static ImageState& get_mutable() noexcept {
    return *const_cast<ImageState*>(k2::image_state());
  }

private:
  static constexpr auto INIT_IMAGE_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB
};
