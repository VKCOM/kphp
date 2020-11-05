// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/tl/methods/network.h"
#include "common/tl/methods/rwm.h"

template<typename T>
struct tl_out_methods_network_rwm_base : tl_out_methods_raw_msg_base<T, tl_out_methods_network> {
  using Base = tl_out_methods_raw_msg_base<T, tl_out_methods_network>;
  void store_reset() noexcept final {
    rwm_clear(Base::get_rwm());
  }
  int compress(int version) noexcept final {
    compress_rwm(*Base::get_rwm(), version);
    return Base::get_rwm()->total_bytes;
  }
};
