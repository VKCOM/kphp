#pragma once

#include "runtime/kphp_core.h"

string f$uniqid(const string &prefix = string(), bool more_entropy = false);


Optional<string> f$iconv(const string &input_encoding, const string &output_encoding, const string &input_str);


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

var f$getimagesize(const string &name);


int64_t f$posix_getpid();
int64_t f$posix_getuid();
Optional<array<var>> f$posix_getpwuid(int64_t uid);


string f$serialize(const var &v);

var f$unserialize(const string &v);
var unserialize_raw(const char *v, int32_t v_len);

constexpr int64_t JSON_UNESCAPED_UNICODE = 1;
constexpr int64_t JSON_FORCE_OBJECT = 16;
constexpr int64_t JSON_PARTIAL_OUTPUT_ON_ERROR = 512;
constexpr int64_t JSON_AVAILABLE_OPTIONS = JSON_UNESCAPED_UNICODE | JSON_FORCE_OBJECT | JSON_PARTIAL_OUTPUT_ON_ERROR;

Optional<string> f$json_encode(const var &v, int64_t options = 0, bool simple_encode = false);

string f$vk_json_encode_safe(const var &v, bool simple_encode = true);

var f$json_decode(const string &v, bool assoc = false);

string f$print_r(const var &v, bool buffered = false);

template<class T>
string f$print_r(const class_instance<T> &v, bool buffered = false) {
  php_warning("print_r used on object");
  return f$print_r(string(v.get_class(), (string::size_type)strlen(v.get_class())), buffered);
}

void f$var_dump(const var &v);

template<class T>
void f$var_dump(const class_instance<T> &v) {
  php_warning("print_r used on object");
  return f$var_dump(string(v.get_class(), (string::size_type)strlen(v.get_class())));
}


string f$var_export(const var &v, bool buffered = false);

template<class T>
string f$var_export(const class_instance<T> &v, bool buffered = false) {
  php_warning("print_r used on object");
  return f$var_export(string(v.get_class(), (string::size_type)strlen(v.get_class())), buffered);
}

string f$cp1251(const string &utf8_string);


/** For local usage only **/
int64_t f$system(const string &query);

void f$kphp_set_context_on_error(const array<var> &tags, const array<var> &extra_info, const string& env = {});
