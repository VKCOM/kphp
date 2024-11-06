// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/curl/curl-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

CurlComponentContext &CurlComponentContext::get() noexcept {
  return get_component_context()->curl_instance_state;
}
