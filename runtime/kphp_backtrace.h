#pragma once

#include <functional>

#include "runtime/kphp_core.h"

using backtrace_each_line_callback_t = std::function<void(const char *function_name, const char *trace_str)>;

bool get_demangled_backtrace(void **buffer, int nptrs, int num_shift, backtrace_each_line_callback_t callback, int start_i = 0);
