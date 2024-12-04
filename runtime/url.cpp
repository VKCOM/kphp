// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/url.h"

string AMPERSAND("&", 1);

// string f$base64_decode (const string &s) {
//   int result_len = (s.size() + 3) / 4 * 3;
//   string res (result_len, false);
//   result_len = base64_decode (s.c_str(), reinterpret_cast <unsigned char *> (res.buffer()), result_len + 1);
//
//   if (result_len < 0) {
//     return string();
//   }
//
//   res.shrink (result_len);
//   return res;
// }
