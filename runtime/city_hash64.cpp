// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "city_hash64.h"

::string f$CityHash64(const ::string &s) {
  const auto hash = CityHash_v1_0_2::CityHash64(s.c_str(), s.size());
  char buf[20] = {};
  return ::string{buf, static_cast<::string::size_type>(simd_uint64_to_string(hash, buf) - buf)};
}
