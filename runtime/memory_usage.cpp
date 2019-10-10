#include "runtime/memory_usage.h"

int f$estimate_memory_usage(const string &value) {
  return static_cast<int>(value.estimate_memory_usage());
}

int f$estimate_memory_usage(const var &value) {
  if(value.is_string()) {
    return f$estimate_memory_usage(value.as_string());
  } else if (value.is_array()) {
    return f$estimate_memory_usage(value.as_array());
  }
  return 0;
}