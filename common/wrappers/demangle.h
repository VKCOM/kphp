#pragma once

#include <cxxabi.h>
#include <string>

namespace vk {

inline std::string demangle(const char *raw_name) {
  int status = 0;
  char *real_name = abi::__cxa_demangle(raw_name, nullptr, nullptr, &status);
  std::string res_name = (status == 0) ? real_name : raw_name;
  std::free(real_name);
  return res_name;
}

inline std::string demangle(const std::string &raw_name) {
  return demangle(raw_name.c_str());
}

template<typename T>
std::string demangle() {
  return demangle(typeid(T).name());
}

}  // namespace vk

