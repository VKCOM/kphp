// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/smart_ptrs/singleton.h"

#include "runtime/kphp_core.h"

using curl_easy = int64_t;

curl_easy f$curl_init(const string &url = string{}) noexcept;

void f$curl_reset(curl_easy easy_id) noexcept;

bool f$curl_setopt(curl_easy easy_id, int64_t option, const mixed &value) noexcept;

bool f$curl_setopt_array(curl_easy easy_id, const array<mixed> &options) noexcept;

mixed f$curl_exec(curl_easy easy_id) noexcept;

mixed f$curl_getinfo(curl_easy easy_id, int64_t option = 0) noexcept;

string f$curl_error(curl_easy easy_id) noexcept;

int64_t f$curl_errno(curl_easy easy_id) noexcept;

void f$curl_close(curl_easy easy_id) noexcept;


using curl_multi = int64_t;

curl_multi f$curl_multi_init() noexcept;

Optional<int64_t> f$curl_multi_add_handle(curl_multi multi_id, curl_easy easy_id) noexcept;

Optional<string> f$curl_multi_getcontent(curl_easy easy_id) noexcept;

bool f$curl_multi_setopt(curl_multi multi_id, int64_t option, int64_t value) noexcept;

Optional<int64_t> f$curl_multi_exec(curl_multi multi_id, int64_t &still_running) noexcept;

Optional<int64_t> f$curl_multi_select(curl_multi multi_id, double timeout = 1.0) noexcept;

extern int64_t curl_multi_info_read_msgs_in_queue_stub;
Optional<array<int64_t>> f$curl_multi_info_read(curl_multi multi_id, int64_t &msgs_in_queue = curl_multi_info_read_msgs_in_queue_stub);

Optional<int64_t> f$curl_multi_remove_handle(curl_multi multi_id, curl_easy easy_id) noexcept;

Optional<int64_t> f$curl_multi_errno(curl_multi multi_id) noexcept;

void f$curl_multi_close(curl_multi multi_id) noexcept;

Optional<string> f$curl_multi_strerror(int64_t error_num) noexcept;

void global_init_curl_lib() noexcept;
void free_curl_lib() noexcept;

struct CurlMemoryUsage : vk::not_copyable {
public:
  size_t currently_allocated{0};
  size_t total_allocated{0};

private:
  CurlMemoryUsage() = default;

  friend class vk::singleton<CurlMemoryUsage>;
};

namespace curl_async {

class CurlRequest {
public:
  static CurlRequest build(curl_easy easy_id);

  void send_async() const;
  void finish_request(Optional<string> &&respone = false) const;
  void detach_multi_and_easy_handles() const noexcept;

  const curl_easy easy_id{0};
  const curl_multi multi_id{0};
  const int request_id{0};

private:
  CurlRequest(curl_easy easy_id, curl_multi multi_id) noexcept;
};

class CurlResponse : public ManagedThroughDlAllocator, vk::not_copyable {
public:
  CurlResponse(Optional<string> &&response, int bound_request_id) noexcept
    : response(std::move(response))
    , bound_request_id(bound_request_id) {}

  Optional<string> response;
  const int bound_request_id{0};
};
} // namespace curl_async
