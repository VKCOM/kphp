// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"

extern const string COLON;
extern const string CP1251;
extern const string DOT;
extern const string NEW_LINE;
extern const string SPACE;
extern const string WHAT;

constexpr int32_t PHP_BUF_LEN = (1 << 23);//TODO remove usages of static buffer
extern char php_buf[PHP_BUF_LEN + 1];

extern const char lhex_digits[17];
extern const char uhex_digits[17];

extern int64_t str_replace_count_dummy;

inline uint8_t hex_to_int(char c) noexcept;


string f$addcslashes(const string &str, const string &what);

string f$addslashes(const string &str);

string f$bin2hex(const string &str);

string f$chop(const string &s, const string &what = WHAT);

string f$chr(int64_t v);

string f$convert_cyr_string(const string &str, const string &from_s, const string &to_s);

mixed f$count_chars(const string &str, int64_t mode = 0);

string f$hex2bin(const string &str);

constexpr int64_t ENT_HTML401 = 0;
constexpr int64_t ENT_COMPAT = 0;
constexpr int64_t ENT_QUOTES = 1;
constexpr int64_t ENT_NOQUOTES = 2;

string f$htmlentities(const string &str);

string f$html_entity_decode(const string &str, int64_t flags = ENT_COMPAT | ENT_HTML401, const string &encoding = CP1251);

string f$htmlspecialchars(const string &str, int64_t flags = ENT_COMPAT | ENT_HTML401);

string f$htmlspecialchars_decode(const string &str, int64_t flags = ENT_COMPAT | ENT_HTML401);

string f$lcfirst(const string &str);

int64_t f$levenshtein(const string &str1, const string &str2);

string f$ltrim(const string &s, const string &what = WHAT);

string f$mysql_escape_string(const string &str);

string f$nl2br(const string &str, bool is_xhtml = true);

inline string f$number_format(double number, int64_t decimals = 0);

inline string f$number_format(double number, int64_t decimals, const string &dec_point);

inline string f$number_format(double number, int64_t decimals, const mixed &dec_point);

string f$number_format(double number, int64_t decimals, const string &dec_point, const string &thousands_sep);

inline string f$number_format(double number, int64_t decimals, const string &dec_point, const mixed &thousands_sep);

inline string f$number_format(double number, int64_t decimals, const mixed &dec_point, const string &thousands_sep);

inline string f$number_format(double number, int64_t decimals, const mixed &dec_point, const mixed &thousands_sep);

int64_t f$ord(const string &s);

string f$pack(const string &pattern, const array<mixed> &a);

string f$prepare_search_query(const string &query);

int64_t f$printf(const string &format, const array<mixed> &a);

string f$rtrim(const string &s, const string &what = WHAT);

Optional<string> f$setlocale(int64_t category, const string &locale);

string f$sprintf(const string &format, const array<mixed> &a);

string f$stripcslashes(const string &str);

string f$stripslashes(const string &str);

int64_t f$strcasecmp(const string &lhs, const string &rhs);

int64_t f$strcmp(const string &lhs, const string &rhs);

string f$strip_tags(const string &str, const array<Unknown> &allow);
string f$strip_tags(const string &str, const mixed &allow);
string f$strip_tags(const string &str, const array<string> &allow_list);
string f$strip_tags(const string &str, const string &allow = string());

Optional<int64_t> f$stripos(const string &haystack, const string &needle, int64_t offset = 0);

inline Optional<int64_t> f$stripos(const string &haystack, const mixed &needle, int64_t offset = 0);

Optional<string> f$stristr(const string &haystack, const string &needle, bool before_needle = false);

inline Optional<string> f$stristr(const string &haystack, const mixed &needle, bool before_needle = false);

Optional<string> f$strrchr(const string &haystack, const string &needle);

inline int64_t f$strlen(const string &s);

int64_t f$strncmp(const string &lhs, const string &rhs, int64_t len);

int64_t f$strnatcmp(const string &lhs, const string &rhs);

int64_t f$strspn(const string &hayshack, const string &char_list, int64_t offset = 0) noexcept;

int64_t f$strcspn(const string &hayshack, const string &char_list, int64_t offset = 0) noexcept;

Optional<string> f$strpbrk(const string &haystack, const string &char_list);

Optional<int64_t> f$strpos(const string &haystack, const string &needle, int64_t offset = 0);

inline Optional<int64_t> f$strpos(const string &haystack, const mixed &needle, int64_t offset = 0);

template<class T>
inline Optional<int64_t> f$strpos(const string &haystack, const Optional<T> &needle, int64_t offset = 0);

Optional<int64_t> f$strrpos(const string &haystack, const string &needle, int64_t offset = 0);

inline Optional<int64_t> f$strrpos(const string &haystack, const mixed &needle, int64_t offset = 0);

Optional<int64_t> f$strripos(const string &haystack, const string &needle, int64_t offset = 0);

inline Optional<int64_t> f$strripos(const string &haystack, const mixed &needle, int64_t offset = 0);

string f$strrev(const string &str);

Optional<string> f$strstr(const string &haystack, const string &needle, bool before_needle = false);

inline Optional<string> f$strstr(const string &haystack, const mixed &needle, bool before_needle = false);

string f$strtolower(const string &str);

string f$strtoupper(const string &str);

string f$strtr(const string &subject, const string &from, const string &to);

template<class T>
string f$strtr(const string &subject, const array<T> &replace_pairs);

inline string f$strtr(const string &subject, const mixed &from, const mixed &to);

inline string f$strtr(const string &subject, const mixed &replace_pairs);

const int64_t STR_PAD_LEFT = 0;
const int64_t STR_PAD_RIGHT = 1;
const int64_t STR_PAD_BOTH = 2;

string f$str_pad(const string &input, int64_t len, const string &pad_str = SPACE, int64_t pad_type = STR_PAD_RIGHT);

string f$str_repeat(const string &s, int64_t multiplier);

string f$str_replace(const string &search, const string &replace, const string &subject, int64_t &replace_count = str_replace_count_dummy);
string f$str_ireplace(const string &search, const string &replace, const string &subject, int64_t &replace_count = str_replace_count_dummy);

void str_replace_inplace(const string &search, const string &replace, string &subject, int64_t &replace_count, bool with_case);
string str_replace(const string &search, const string &replace, const string &subject, int64_t &replace_count, bool with_case);

template<typename T1, typename T2>
string str_replace_string_array(const array<T1> &search, const array<T2> &replace, const string &subject, int64_t &replace_count, bool with_case) {
  string result = subject;

  string replace_value;
  typename array<T2>::const_iterator cur_replace_val;
  cur_replace_val = replace.begin();

  for (typename array<T1>::const_iterator it = search.begin(); it != search.end(); ++it) {
    if (cur_replace_val != replace.end()) {
      replace_value = f$strval(cur_replace_val.get_value());
      ++cur_replace_val;
    } else {
      replace_value = string();
    }

    const string &search_string = f$strval(it.get_value());
    if (search_string.size() >= replace_value.size()) {
      str_replace_inplace(search_string, replace_value, result, replace_count, with_case);
    } else {
      result = str_replace(search_string, replace_value, result, replace_count, with_case);
    }
  }

  return result;
};

template<typename T1, typename T2>
string f$str_replace(const array<T1> &search, const array<T2> &replace, const string &subject, int64_t &replace_count = str_replace_count_dummy) {
  replace_count = 0;
  return str_replace_string_array(search, replace, subject, replace_count, true);
}
template<typename T1, typename T2>
string f$str_ireplace(const array<T1> &search, const array<T2> &replace, const string &subject, int64_t &replace_count = str_replace_count_dummy) {
  replace_count = 0;
  return str_replace_string_array(search, replace, subject, replace_count, false);
}

string f$str_replace(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count = str_replace_count_dummy);
string f$str_ireplace(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count = str_replace_count_dummy);

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_replace(const T1 &search, const T2 &replace, const SubjectT &subject, int64_t &replace_count = str_replace_count_dummy) {
  return f$str_replace(search, replace, subject.val(), replace_count);
}
template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_ireplace(const T1 &search, const T2 &replace, const SubjectT &subject, int64_t &replace_count = str_replace_count_dummy) {
  return f$str_ireplace(search, replace, subject.val(), replace_count);
}

mixed f$str_replace(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count = str_replace_count_dummy);
mixed f$str_ireplace(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count = str_replace_count_dummy);

array<string> f$str_split(const string &str, int64_t split_length = 1);

Optional<string> f$substr(const string &str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max());
Optional<string> f$substr(tmp_string, int64_t start, int64_t length = std::numeric_limits<int64_t>::max());

tmp_string f$_tmp_substr(const string &str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max());
tmp_string f$_tmp_substr(tmp_string str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max());

int64_t f$substr_count(const string &haystack, const string &needle, int64_t offset = 0, int64_t length = std::numeric_limits<int64_t>::max());

string f$substr_replace(const string &str, const string &replacement, int64_t start, int64_t length = std::numeric_limits<int64_t>::max());

Optional<int64_t> f$substr_compare(const string &main_str, const string &str, int64_t offset, int64_t length = std::numeric_limits<int64_t>::max(), bool case_insensitivity = false);

bool f$str_starts_with(const string &haystack, const string &needle);

bool f$str_ends_with(const string &haystack, const string &needle);

tmp_string f$_tmp_trim(tmp_string s, const string &what = WHAT);
tmp_string f$_tmp_trim(const string &s, const string &what = WHAT);
string f$trim(tmp_string s, const string &what = WHAT);
string f$trim(const string &s, const string &what = WHAT);

string f$ucfirst(const string &str);

string f$ucwords(const string &str);

Optional<array<mixed>> f$unpack(const string &pattern, const string &data);

int64_t f$vprintf(const string &format, const array<mixed> &args);

string f$vsprintf(const string &format, const array<mixed> &args);

string f$wordwrap(const string &str, int64_t width = 75, const string &brk = NEW_LINE, bool cut = false);

/*
 *
 *     IMPLEMENTATION
 *
 */

namespace impl_ {

struct Hex2CharMapMaker {
private:
  static constexpr uint8_t hex2int_char(size_t c) noexcept {
    return ('0' <= c && c <= '9') ? static_cast<uint8_t>(c - '0') :
           ('a' <= c && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10) :
           ('A' <= c && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10) : 16;
  }

public:
  template<size_t... Ints>
  static constexpr auto make(std::index_sequence<Ints...>) noexcept {
    return std::array<uint8_t, sizeof...(Ints)>{
      {
        hex2int_char(Ints)...,
      }};
  }
};

} // namepsace impl_

uint8_t hex_to_int(char c) noexcept {
  static constexpr auto hex_int_map = impl_::Hex2CharMapMaker::make(std::make_index_sequence<256>());
  return hex_int_map[static_cast<uint8_t>(c)];
}

string f$number_format(double number, int64_t decimals) {
  return f$number_format(number, decimals, DOT, COLON);
}

string f$number_format(double number, int64_t decimals, const string &dec_point) {
  return f$number_format(number, decimals, dec_point, COLON);
}

string f$number_format(double number, int64_t decimals, const mixed &dec_point) {
  return f$number_format(number, decimals, dec_point.is_null() ? DOT : dec_point.to_string(), COLON);
}

string f$number_format(double number, int64_t decimals, const string &dec_point, const mixed &thousands_sep) {
  return f$number_format(number, decimals, dec_point, thousands_sep.is_null() ? COLON : thousands_sep.to_string());
}

string f$number_format(double number, int64_t decimals, const mixed &dec_point, const string &thousands_sep) {
  return f$number_format(number, decimals, dec_point.is_null() ? DOT : dec_point.to_string(), thousands_sep);
}

string f$number_format(double number, int64_t decimals, const mixed &dec_point, const mixed &thousands_sep) {
  return f$number_format(number, decimals, dec_point.is_null() ? DOT : dec_point.to_string(), thousands_sep.is_null() ? COLON : thousands_sep.to_string());
}

int64_t f$strlen(const string &s) {
  return s.size();
}

Optional<int64_t> f$stripos(const string &haystack, const mixed &needle, int64_t offset) {
  if (needle.is_string()) {
    return f$stripos(haystack, needle.to_string(), offset);
  } else {
    return f$stripos(haystack, string(1, (char)needle.to_int()), offset);
  }
}

Optional<string> f$stristr(const string &haystack, const mixed &needle, bool before_needle) {
  if (needle.is_string()) {
    return f$stristr(haystack, needle.to_string(), before_needle);
  } else {
    return f$stristr(haystack, string(1, (char)needle.to_int()), before_needle);
  }
}

template<class T>
inline Optional<int64_t> f$strpos(const string &haystack, const Optional<T> &needle, int64_t offset) {
  return f$strpos(haystack, needle.val(), offset);
}

Optional<int64_t> f$strpos(const string &haystack, const mixed &needle, int64_t offset) {
  if (needle.is_string()) {
    return f$strpos(haystack, needle.to_string(), offset);
  } else {
    return f$strpos(haystack, string(1, (char)needle.to_int()), offset);
  }
}

Optional<int64_t> f$strrpos(const string &haystack, const mixed &needle, int64_t offset) {
  if (needle.is_string()) {
    return f$strrpos(haystack, needle.to_string(), offset);
  } else {
    return f$strrpos(haystack, string(1, (char)needle.to_int()), offset);
  }
}

Optional<int64_t> f$strripos(const string &haystack, const mixed &needle, int64_t offset) {
  if (needle.is_string()) {
    return f$strripos(haystack, needle.to_string(), offset);
  } else {
    return f$strripos(haystack, string(1, (char)needle.to_int()), offset);
  }
}

Optional<string> f$strstr(const string &haystack, const mixed &needle, bool before_needle) {
  if (needle.is_string()) {
    return f$strstr(haystack, needle.to_string(), before_needle);
  } else {
    return f$strstr(haystack, string(1, (char)needle.to_int()), before_needle);
  }
}

template<class T>
string f$strtr(const string &subject, const array<T> &replace_pairs) {
  const char *piece = subject.c_str(), *piece_end = subject.c_str() + subject.size();
  string result;
  while (1) {
    const char *best_pos = nullptr;
    int64_t best_len = -1;
    string replace;
    for (typename array<T>::const_iterator p = replace_pairs.begin(); p != replace_pairs.end(); ++p) {
      const string search = f$strval(p.get_key());
      int64_t search_len = search.size();
      if (search_len == 0) {
        return subject;
      }
      const char *pos = static_cast <const char *> (memmem(static_cast <const void *> (piece), (size_t)(piece_end - piece), static_cast <const void *> (search.c_str()), (size_t)search_len));
      if (pos != nullptr && (best_pos == nullptr || best_pos > pos || (best_pos == pos && search_len > best_len))) {
        best_pos = pos;
        best_len = search_len;
        replace = f$strval(p.get_value());
      }
    }
    if (best_pos == nullptr) {
      result.append(piece, static_cast<string::size_type>(piece_end - piece));
      break;
    }

    result.append(piece, static_cast<string::size_type>(best_pos - piece));
    result.append(replace);

    piece = best_pos + best_len;
  }

  return result;
}

inline string f$strtr(const string &subject, const mixed &from, const mixed &to) {
  return f$strtr(subject, from.to_string(), to.to_string());
}

inline string f$strtr(const string &subject, const mixed &replace_pairs) {
  return f$strtr(subject, replace_pairs.as_array("strtr"));
}

string f$xor_strings(const string &s, const string &t);

namespace impl_ {
extern double default_similar_text_percent_stub;
} // namespace impl_
int64_t f$similar_text(const string &first, const string &second, double &percent = impl_::default_similar_text_percent_stub);

// similar_text ( string $first , string $second [, float &$percent ] ) : int

// str_concat_arg generalizes both tmp_string and string arguments;
// it can be constructed from both of them, so concat functions can operate
// on both tmp_string and string types
// there is a special (string, string) overloading for concat2 to
// allow the empty string result optimization to kick in
struct str_concat_arg {
  const char *data;
  string::size_type size;

  str_concat_arg(const string &s) : data{s.c_str()}, size{s.size()} {}
  str_concat_arg(tmp_string s) : data{s.data}, size{s.size} {}

  tmp_string as_tmp_string() const noexcept {
    return {data, size};
  }
};

// str_concat functions implement efficient string-typed `.` (concatenation) operator implementation;
// apart from being machine-code size efficient (a function call is more compact), they're also
// usually faster as runtime is compiled with -O3 which is almost never the case for translated C++ code
// (it's either -O2 or -Os most of the time)
//
// we choose to have 4 functions (up to 5 arguments) because of the frequency distribution:
//   37619: 2 args
//   20616: 3 args
//    4534: 5 args
//    3791: 4 args
//     935: 7 args
//     565: 6 args
//     350: 9 args
// Both 6 and 7 argument combination already look infrequent enough to not bother
string str_concat(const string &s1, const string &s2);
string str_concat(str_concat_arg s1, str_concat_arg s2);
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3);
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4);
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4, str_concat_arg s5);
