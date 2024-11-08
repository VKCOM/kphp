// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-functions.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/component/component-api.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"
#include "runtime-light/utils/json-functions.h"

namespace {

constexpr std::string_view CONFDATA_COMPONENT_NAME = "confdata"; // TODO: it may actually have an alias specified in linking config

mixed extract_confdata_value(tl::confdataValue &&confdata_value) noexcept {
  if (confdata_value.is_php_serialized && confdata_value.is_json_serialized) { // check that we don't have both flags set
    php_warning("confdata value has both php_serialized and json_serialized flags set");
    return {};
  }
  if (confdata_value.is_php_serialized) {
    php_critical_error("unimplemented"); // TODO
  } else if (confdata_value.is_json_serialized) {
    return f$json_decode(confdata_value.value);
  } else {
    return std::move(confdata_value.value);
  }
}

} // namespace

// TODO: the performance of this implementation can be enhanced. rework it when the platform has specific API for that
bool f$is_confdata_loaded() noexcept {
  auto &instance_st{InstanceState::get()};
  if (const auto stream_d{instance_st.open_stream(CONFDATA_COMPONENT_NAME)}; stream_d != INVALID_PLATFORM_DESCRIPTOR) {
    instance_st.release_stream(stream_d);
    return true;
  }
  return false;
}

task_t<mixed> f$confdata_get_value(string key) noexcept {
  tl::TLBuffer tlb{};
  tl::ConfdataGet{.key = std::move(key)}.store(tlb);

  auto query{co_await f$component_client_send_request({CONFDATA_COMPONENT_NAME.data(), static_cast<string::size_type>(CONFDATA_COMPONENT_NAME.size())},
                                                      {tlb.data(), static_cast<string::size_type>(tlb.size())})};
  const auto response{co_await f$component_client_fetch_response(std::move(query))};

  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});
  tl::Maybe<tl::confdataValue> maybe_confdata_value{};
  if (!maybe_confdata_value.fetch(tlb)) {
    php_warning("couldn't fetch response");
    co_return mixed{};
  }

  if (!maybe_confdata_value.opt_value.has_value()) { // no such key
    co_return mixed{};
  }
  co_return extract_confdata_value(*std::move(maybe_confdata_value.opt_value)); // the key exists
}

task_t<array<mixed>> f$confdata_get_values_by_any_wildcard(string wildcard) noexcept {
  tl::TLBuffer tlb{};
  tl::ConfdataGetWildcard{.wildcard = std::move(wildcard)}.store(tlb);

  auto query{co_await f$component_client_send_request({CONFDATA_COMPONENT_NAME.data(), static_cast<string::size_type>(CONFDATA_COMPONENT_NAME.size())},
                                                      {tlb.data(), static_cast<string::size_type>(tlb.size())})};
  const auto response{co_await f$component_client_fetch_response(std::move(query))};

  tlb.clean();
  tlb.store_bytes({response.c_str(), static_cast<size_t>(response.size())});
  tl::Dictionary<tl::confdataValue> dict_confdata_value{};
  if (!dict_confdata_value.fetch(tlb)) {
    php_warning("couldn't fetch response");
    co_return array<mixed>{};
  }

  array<mixed> result{array_size{static_cast<int64_t>(dict_confdata_value.size()), false}};
  std::for_each(dict_confdata_value.begin(), dict_confdata_value.end(),
                [&result](auto &&dict_field) { result.set_value(std::move(dict_field.key), extract_confdata_value(std::move(dict_field.value))); });
  co_return std::move(result);
}
