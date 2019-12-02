#pragma once

#include "runtime/kphp_core.h"

using curl_easy = int;

curl_easy f$curl_init(const string &url = string{}) noexcept;

bool f$curl_setopt(curl_easy easy_id, int option, const var &value) noexcept;

bool f$curl_setopt_array(curl_easy easy_id, const array<var> &options) noexcept;

var f$curl_exec(curl_easy easy_id) noexcept;

var f$curl_getinfo(curl_easy easy_id, int option = 0) noexcept;

string f$curl_error(curl_easy easy_id) noexcept;

int f$curl_errno(curl_easy easy_id) noexcept;

void f$curl_close(curl_easy easy_id) noexcept;


using curl_multi = int;

curl_multi f$curl_multi_init() noexcept;

Optional<int> f$curl_multi_add_handle(curl_multi multi_id, curl_easy easy_id) noexcept;

Optional<string> f$curl_multi_getcontent(curl_easy easy_id) noexcept;

bool f$curl_multi_setopt(curl_multi multi_id, int option, int value) noexcept;

Optional<int> f$curl_multi_exec(curl_multi multi_id, int &still_running) noexcept;

Optional<int> f$curl_multi_select(curl_multi multi_id, double timeout = 1.0) noexcept;

extern int curl_multi_info_read_msgs_in_queue_stub;
Optional<array<int>> f$curl_multi_info_read(curl_multi multi_id, int &msgs_in_queue = curl_multi_info_read_msgs_in_queue_stub);

Optional<int> f$curl_multi_remove_handle(curl_multi multi_id, curl_easy easy_id) noexcept;

Optional<int> f$curl_multi_errno(curl_multi multi_id) noexcept;

void f$curl_multi_close(curl_multi multi_id) noexcept;

Optional<string> f$curl_multi_strerror(int error_num) noexcept;

void free_curl_lib() noexcept;


