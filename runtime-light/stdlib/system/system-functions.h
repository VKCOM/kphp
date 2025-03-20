// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/stdlib/system/system-state.h"

template<typename F>
bool f$register_kphp_on_oom_callback(F && /*callback*/) {
  php_critical_error("call to unsupported function");
}

template<typename F>
void f$kphp_extended_instance_cache_metrics_init(F && /*callback*/) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$system(const string & /*command*/, int64_t & /*result_code*/ = SystemInstanceState::get().result_code_dummy) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$getopt(const string & /*options*/, const array<string> & /*longopts*/ = {},
                                       Optional<int64_t> & /*rest_index*/ = SystemInstanceState::get().rest_index_dummy) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$numa_get_bound_node() noexcept {
  return -1;
}

inline void f$kphp_set_context_on_error([[maybe_unused]] const array<mixed> &tags, [[maybe_unused]] const array<mixed> &extra_info,
                                        [[maybe_unused]] const string &env = {}) noexcept {}

inline int64_t f$posix_getpid() noexcept {
  return static_cast<int64_t>(ImageState::get().pid);
}

inline string f$php_uname(const string &mode = string{1, 'a'}) noexcept {
  const auto &image_st{ImageState::get()};
  const char mode_c{mode.empty() ? 'a' : mode[0]};
  switch (mode_c) {
    case 's':
      return image_st.uname_info_s;
    case 'n':
      return image_st.uname_info_n;
    case 'r':
      return image_st.uname_info_r;
    case 'v':
      return image_st.uname_info_v;
    case 'm':
      return image_st.uname_info_m;
    default:
      return image_st.uname_info_a;
  }
}

string f$php_sapi_name() noexcept;

Optional<string> f$iconv(const string &input_encoding, const string &output_encoding, const string &input_str) noexcept;

inline array<array<string>> f$debug_backtrace() noexcept {
  php_warning("called stub debug_backtrace");
  return {};
}

inline int64_t f$error_reporting([[maybe_unused]] int64_t level) noexcept {
  php_warning("called stub error_reporting");
  return 0;
}

inline int64_t f$error_reporting() noexcept {
  php_warning("called stub error_reporting");
  return 0;
}

inline Optional<string> f$exec([[maybe_unused]] const string &command) noexcept {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$exec([[maybe_unused]] const string &command, [[maybe_unused]] mixed &output,
                               [[maybe_unused]] int64_t &result_code = SystemInstanceState::get().result_code_dummy) noexcept {
  php_critical_error("call to unsupported function");
}

inline string f$get_engine_version() noexcept {
  php_warning("called stub get_engine_version");
  return {};
}

inline string f$get_kphp_cluster_name() noexcept {
  php_warning("called stub get_kphp_cluster_name");
  return string("adm512");
}
