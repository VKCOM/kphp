// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

string f$uniqid(const string& prefix = string(), bool more_entropy = false);

Optional<string> f$iconv(const string& input_encoding, const string& output_encoding, const string& input_str);

void f$sleep(int64_t seconds);

void f$usleep(int64_t micro_seconds);

constexpr int64_t IMAGETYPE_UNKNOWN = 0;
constexpr int64_t IMAGETYPE_GIF = 1;
constexpr int64_t IMAGETYPE_JPEG = 2;
constexpr int64_t IMAGETYPE_PNG = 3;
constexpr int64_t IMAGETYPE_SWF = 4;
constexpr int64_t IMAGETYPE_PSD = 5;
constexpr int64_t IMAGETYPE_BMP = 6;
constexpr int64_t IMAGETYPE_TIFF_II = 7;
constexpr int64_t IMAGETYPE_TIFF_MM = 8;
constexpr int64_t IMAGETYPE_JPC = 9;
constexpr int64_t IMAGETYPE_JPEG2000 = 9;
constexpr int64_t IMAGETYPE_JP2 = 10;

mixed f$getimagesize(const string& name);

int64_t f$posix_getpid();
int64_t f$posix_getuid();
Optional<array<mixed>> f$posix_getpwuid(int64_t uid);

string f$print_r(const mixed& v, bool buffered = false);

template<class T>
string f$print_r(const class_instance<T>& v, bool buffered = false) {
  php_warning("print_r used on object");
  return f$print_r(string(v.get_class(), (string::size_type)strlen(v.get_class())), buffered);
}

void f$var_dump(const mixed& v);

template<class T>
void f$var_dump(const class_instance<T>& v) {
  php_warning("print_r used on object");
  return f$var_dump(string(v.get_class(), (string::size_type)strlen(v.get_class())));
}

string f$var_export(const mixed& v, bool buffered = false);

template<class T>
string f$var_export(const class_instance<T>& v, bool buffered = false) {
  php_warning("print_r used on object");
  return f$var_export(string(v.get_class(), (string::size_type)strlen(v.get_class())), buffered);
}

string f$cp1251(const string& utf8_string);

void f$kphp_set_context_on_error(const array<mixed>& tags, const array<mixed>& extra_info, const string& env = {});
