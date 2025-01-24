// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

template<typename ClassInstanceType>
bool f$instance_cache_store(const string & /*key*/, const ClassInstanceType & /*instance*/, int64_t /*ttl*/ = 0) noexcept {
  php_warning("called stub instance_cache_store");
  return false;
}

template<typename ClassInstanceType>
ClassInstanceType f$instance_cache_fetch(const string & /*class_name*/, const string & /*key*/, bool /*even_if_expired*/ = false) noexcept {
  php_warning("called stub instance_cache_fetch");
  return {};
}
