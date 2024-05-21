#pragma once

#include "runtime-core/runtime-core.h"

struct Response {
  int static constexpr ob_max_buffers = 50;
  string_buffer output_buffers[ob_max_buffers];
  int current_buffer = 0;
};

void f$ob_clean();

bool f$ob_end_clean();

Optional<string> f$ob_get_clean();

string f$ob_get_contents();

void f$ob_start(const string &callback = string());

void f$ob_flush();

bool f$ob_end_flush();

Optional<string> f$ob_get_flush();

Optional<int64_t> f$ob_get_length();

int64_t f$ob_get_level();


