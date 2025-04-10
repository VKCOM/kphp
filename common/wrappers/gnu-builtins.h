// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#ifdef __clang__
#if !__has_builtin(__builtin_FILE)
static inline const char* __builtin_FILE() {
  return __FILE__;
}
static inline int __builtin_LINE() {
  return __LINE__;
}
static inline const char* __builtin_FUNCTION() {
  return __FUNCTION__;
}
#endif
#endif

#ifdef __CLION_IDE__
static inline const char* __builtin_FILE() {
  return __FILE__;
}
static inline int __builtin_LINE() {
  return __LINE__;
}
static inline const char* __builtin_FUNCTION() {
  return __FUNCTION__;
}
#endif