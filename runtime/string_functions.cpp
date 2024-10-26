// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/string_functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime/interface.h"
#include "runtime/streams.h"

int64_t f$printf(const string &format, const array<mixed> &a) noexcept {
  string to_print = f$sprintf(format, a);
  print(to_print);
  return to_print.size();
}

int64_t f$vprintf(const string &format, const array<mixed> &args) noexcept {
  return f$printf(format, args);
}

// Based on `getcsv` from `streams`
Optional<array<mixed>> f$str_getcsv(const string &str, const string &delimiter, const string &enclosure, const string &escape) noexcept {
  char delimiter_char = ',';
  char enclosure_char = '"';
  char escape_char = PHP_CSV_NO_ESCAPE;
  /*
   * By PHP Manual: delimiter, enclosure, escape -- one single-byte character only
   * We make it a warning
   * Since PHP 8.3.11 it should return false
   */
  const auto del_size = delimiter.size();
  if (del_size > 1) {
    php_warning("Delimiter must be a single character");
  }
  if (del_size != 0) {
    delimiter_char = delimiter[0];
  }

  const auto enc_size = enclosure.size();
  if (enc_size > 1) {
    php_warning("Enclosure must be a single character");
  }
  if (enc_size != 0) {
    enclosure_char = enclosure[0];
  }

  const auto esc_size = escape.size();
  if (esc_size > 1) {
    php_warning("Escape must be a single character");
  }
  if (esc_size != 0) {
    escape_char = escape[0];
  }

  return getcsv(mixed() /* null */, str, delimiter_char, enclosure_char, escape_char);
}
