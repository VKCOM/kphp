// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/curl/curl.h"

#include "runtime-light/component/component.h"

CurlComponentState &CurlComponentState::get() noexcept {
  return get_component_context()->curl_component_state;
}
