#include "runtime/kphp_backtrace.h"

#include <cxxabi.h>
#include <execinfo.h>

#include "runtime/critical_section.h"

bool get_demangled_backtrace(void **buffer, int nptrs, int num_shift, backtrace_each_line_callback_t callback, int start_i) {
  dl::CriticalSectionGuard signal_critical_section;

  char **strings = backtrace_symbols(buffer, nptrs);
  if (strings == nullptr) {
    return false;
  }

  for (int i = start_i; i < nptrs; i++) {
    char *mangled_name = nullptr, *offset_begin = nullptr, *offset_end = nullptr;
    for (char *p = strings[i]; *p; ++p) {
      if (*p == '(') {
        mangled_name = p;
      } else if (*p == '+' && mangled_name != nullptr) {
        offset_begin = p;
      } else if (*p == ')' && offset_begin != nullptr) {
        offset_end = p;
        break;
      }
    }
    if (offset_end == nullptr) {
      continue;
    }

    size_t copy_name_len = offset_begin - mangled_name;
    char copy_name[1024], trace_str[2048];
    memcpy(copy_name, mangled_name + 1, copy_name_len - 1);
    copy_name[copy_name_len - 1] = 0;

    int status;
    char *real_name = abi::__cxa_demangle(copy_name, nullptr, nullptr, &status);
    if (status < 0) {
      real_name = copy_name;
    }
    snprintf(trace_str, sizeof(trace_str), "(%d) %.*s : %s+%.*s%s\n", i + num_shift, (int)(mangled_name - strings[i]), strings[i], real_name, (int)(offset_end - offset_begin - 1), offset_begin + 1, offset_end + 1);

    callback(real_name, trace_str);

    if (status == 0) {
      free(real_name);
    }
  }

  free(strings);
  return true;
}
