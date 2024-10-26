// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"

string f$addcslashes(const string &str, const string &what) noexcept;

string f$addslashes(const string &str) noexcept;

string f$bin2hex(const string &str) noexcept;

string f$chop(const string &s, const string &what = StringLibConstants::get().WHAT) noexcept;

string f$chr(int64_t v) noexcept;

string f$convert_cyr_string(const string &str, const string &from_s, const string &to_s) noexcept;

mixed f$count_chars(const string &str, int64_t mode = 0) noexcept;

string f$hex2bin(const string &str) noexcept;

string f$htmlentities(const string &str) noexcept;

string f$html_entity_decode(const string &str, int64_t flags = StringLibConstants::ENT_COMPAT | StringLibConstants::ENT_HTML401,
                            const string &encoding = StringLibConstants::get().CP1251) noexcept;

string f$htmlspecialchars(const string &str, int64_t flags = StringLibConstants::ENT_COMPAT | StringLibConstants::ENT_HTML401) noexcept;

string f$htmlspecialchars_decode(const string &str, int64_t flags = StringLibConstants::ENT_COMPAT | StringLibConstants::ENT_HTML401) noexcept;

string f$lcfirst(const string &str) noexcept;

int64_t f$levenshtein(const string &str1, const string &str2) noexcept;

string f$ltrim(const string &s, const string &what = StringLibConstants::get().WHAT) noexcept;

string f$mysql_escape_string(const string &str) noexcept;

string f$nl2br(const string &str, bool is_xhtml = true) noexcept;

string f$number_format(double number, int64_t decimals, const string &dec_point, const string &thousands_sep) noexcept;

int64_t f$ord(const string &s) noexcept;

string f$pack(const string &pattern, const array<mixed> &a) noexcept;

string f$prepare_search_query(const string &query) noexcept;

string f$rtrim(const string &s, const string &what = StringLibConstants::get().WHAT) noexcept;

Optional<string> f$setlocale(int64_t category, const string &locale) noexcept;

string f$sprintf(const string &format, const array<mixed> &a) noexcept;

string f$stripcslashes(const string &str) noexcept;

string f$stripslashes(const string &str) noexcept;

int64_t f$strcasecmp(const string &lhs, const string &rhs) noexcept;

int64_t f$strcmp(const string &lhs, const string &rhs) noexcept;

string f$strip_tags(const string &str, const array<Unknown> &allow) noexcept;

string f$strip_tags(const string &str, const mixed &allow);

string f$strip_tags(const string &str, const array<string> &allow_list);

string f$strip_tags(const string &str, const string &allow = string{});

Optional<int64_t> f$stripos(const string &haystack, const string &needle, int64_t offset = 0) noexcept;

inline Optional<int64_t> f$stripos(const string &haystack, const mixed &needle, int64_t offset = 0) noexcept;

Optional<string> f$stristr(const string &haystack, const string &needle, bool before_needle = false) noexcept;

Optional<string> f$strrchr(const string &haystack, const string &needle) noexcept;

int64_t f$strncmp(const string &lhs, const string &rhs, int64_t len) noexcept;

int64_t f$strnatcmp(const string &lhs, const string &rhs) noexcept;

int64_t f$strspn(const string &hayshack, const string &char_list, int64_t offset = 0) noexcept;

int64_t f$strcspn(const string &hayshack, const string &char_list, int64_t offset = 0) noexcept;

Optional<string> f$strpbrk(const string &haystack, const string &char_list) noexcept;

Optional<int64_t> f$strpos(const string &haystack, const string &needle, int64_t offset = 0) noexcept;

Optional<int64_t> f$strrpos(const string &haystack, const string &needle, int64_t offset = 0) noexcept;

Optional<int64_t> f$strripos(const string &haystack, const string &needle, int64_t offset = 0) noexcept;

string f$strrev(const string &str) noexcept;

Optional<string> f$strstr(const string &haystack, const string &needle, bool before_needle = false) noexcept;

string f$strtolower(const string &str) noexcept;

string f$strtoupper(const string &str) noexcept;

string f$strtr(const string &subject, const string &from, const string &to) noexcept;

// inline string f$strtr(const string &subject, const mixed &from, const mixed &to);

// inline string f$strtr(const string &subject, const mixed &replace_pairs);

string f$str_pad(const string &input, int64_t len, const string &pad_str = StringLibConstants::get().SPACE,
                 int64_t pad_type = StringLibConstants::STR_PAD_RIGHT) noexcept;

string f$str_repeat(const string &s, int64_t multiplier) noexcept;

string f$str_replace(const string &search, const string &replace, const string &subject,
                     int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept;

string f$str_ireplace(const string &search, const string &replace, const string &subject,
                      int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept;

void str_replace_inplace(const string &search, const string &replace, string &subject, int64_t &replace_count, bool with_case) noexcept;

string str_replace(const string &search, const string &replace, const string &subject, int64_t &replace_count, bool with_case) noexcept;

template<typename T1, typename T2>
string str_replace_string_array(const array<T1> &search, const array<T2> &replace, const string &subject, int64_t &replace_count, bool with_case) noexcept {
  string result = subject;
  string replace_value;
  typename array<T2>::const_iterator cur_replace_val = replace.begin();

  for (typename array<T1>::const_iterator it = search.begin(); it != search.end(); ++it) {
    if (cur_replace_val != replace.end()) {
      replace_value = f$strval(cur_replace_val.get_value());
      ++cur_replace_val;
    } else {
      replace_value = string{};
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
string f$str_replace(const array<T1> &search, const array<T2> &replace, const string &subject,
                     int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept {
  replace_count = 0;
  return str_replace_string_array(search, replace, subject, replace_count, true);
}

template<typename T1, typename T2>
string f$str_ireplace(const array<T1> &search, const array<T2> &replace, const string &subject,
                      int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept {
  replace_count = 0;
  return str_replace_string_array(search, replace, subject, replace_count, false);
}

string f$str_replace(const mixed &search, const mixed &replace, const string &subject,
                     int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept;

string f$str_ireplace(const mixed &search, const mixed &replace, const string &subject,
                      int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept;

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_replace(const T1 &search, const T2 &replace, const SubjectT &subject,
                       int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept {
  return f$str_replace(search, replace, subject.val(), replace_count);
}

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_ireplace(const T1 &search, const T2 &replace, const SubjectT &subject,
                        int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept {
  return f$str_ireplace(search, replace, subject.val(), replace_count);
}

mixed f$str_replace(const mixed &search, const mixed &replace, const mixed &subject,
                    int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept;

mixed f$str_ireplace(const mixed &search, const mixed &replace, const mixed &subject,
                     int64_t &replace_count = StringLibContext::get().str_replace_count_dummy) noexcept;

array<string> f$str_split(const string &str, int64_t split_length = 1) noexcept;

Optional<string> f$substr(const string &str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) noexcept;

Optional<string> f$substr(tmp_string, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) noexcept;

tmp_string f$_tmp_substr(const string &str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) noexcept;

tmp_string f$_tmp_substr(tmp_string str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) noexcept;

int64_t f$substr_count(const string &haystack, const string &needle, int64_t offset = 0, int64_t length = std::numeric_limits<int64_t>::max()) noexcept;

string f$substr_replace(const string &str, const string &replacement, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) noexcept;

Optional<int64_t> f$substr_compare(const string &main_str, const string &str, int64_t offset, int64_t length = std::numeric_limits<int64_t>::max(),
                                   bool case_insensitivity = false) noexcept;

bool f$str_starts_with(const string &haystack, const string &needle) noexcept;

bool f$str_ends_with(const string &haystack, const string &needle) noexcept;

tmp_string f$_tmp_trim(tmp_string s, const string &what = StringLibConstants::get().WHAT) noexcept;

tmp_string f$_tmp_trim(const string &s, const string &what = StringLibConstants::get().WHAT) noexcept;

string f$trim(tmp_string s, const string &what = StringLibConstants::get().WHAT) noexcept;

string f$trim(const string &s, const string &what = StringLibConstants::get().WHAT) noexcept;

string f$ucfirst(const string &str) noexcept;

string f$ucwords(const string &str) noexcept;

Optional<array<mixed>> f$unpack(const string &pattern, const string &data) noexcept;

string f$vsprintf(const string &format, const array<mixed> &args) noexcept;

string f$wordwrap(const string &str, int64_t width = 75, const string &brk = StringLibConstants::get().NEW_LINE, bool cut = false) noexcept;

/*
 *
 *     IMPLEMENTATION
 *
 */

namespace impl_ {

struct Hex2CharMapMaker {
private:
  static constexpr uint8_t hex2int_char(size_t c) noexcept {
    return ('0' <= c && c <= '9')   ? static_cast<uint8_t>(c - '0')
           : ('a' <= c && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10)
           : ('A' <= c && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10)
                                    : 16;
  }

public:
  template<size_t... Ints>
  static constexpr auto make(std::index_sequence<Ints...> /*unused*/) noexcept {
    return std::array<uint8_t, sizeof...(Ints)>{{
      hex2int_char(Ints)...,
    }};
  }
};

} // namespace impl_

inline uint8_t hex_to_int(char c) noexcept {
  static constexpr auto hex_int_map = impl_::Hex2CharMapMaker::make(std::make_index_sequence<256>());
  return hex_int_map[static_cast<uint8_t>(c)];
}

inline string f$number_format(double number, int64_t decimals) noexcept {
  return f$number_format(number, decimals, StringLibConstants::get().DOT, StringLibConstants::get().COLON);
}

inline string f$number_format(double number, int64_t decimals, const string &dec_point) noexcept {
  return f$number_format(number, decimals, dec_point, StringLibConstants::get().COLON);
}

inline string f$number_format(double number, int64_t decimals, const mixed &dec_point) noexcept {
  return f$number_format(number, decimals, dec_point.is_null() ? StringLibConstants::get().DOT : dec_point.to_string(), StringLibConstants::get().COLON);
}

inline string f$number_format(double number, int64_t decimals, const string &dec_point, const mixed &thousands_sep) noexcept {
  return f$number_format(number, decimals, dec_point, thousands_sep.is_null() ? StringLibConstants::get().COLON : thousands_sep.to_string());
}

inline string f$number_format(double number, int64_t decimals, const mixed &dec_point, const string &thousands_sep) noexcept {
  return f$number_format(number, decimals, dec_point.is_null() ? StringLibConstants::get().DOT : dec_point.to_string(), thousands_sep);
}

inline string f$number_format(double number, int64_t decimals, const mixed &dec_point, const mixed &thousands_sep) noexcept {
  return f$number_format(number, decimals, dec_point.is_null() ? StringLibConstants::get().DOT : dec_point.to_string(),
                         thousands_sep.is_null() ? StringLibConstants::get().COLON : thousands_sep.to_string());
}

inline int64_t f$strlen(const string &s) noexcept {
  return s.size();
}

inline Optional<int64_t> f$stripos(const string &haystack, const mixed &needle, int64_t offset) noexcept {
  if (needle.is_string()) {
    return f$stripos(haystack, needle.to_string(), offset);
  } else {
    return f$stripos(haystack, string(1, static_cast<char>(needle.to_int())), offset);
  }
}

inline Optional<string> f$stristr(const string &haystack, const mixed &needle, bool before_needle) noexcept {
  if (needle.is_string()) {
    return f$stristr(haystack, needle.to_string(), before_needle);
  } else {
    return f$stristr(haystack, string(1, static_cast<char>(needle.to_int())), before_needle);
  }
}

template<class T>
Optional<int64_t> f$strpos(const string &haystack, const Optional<T> &needle, int64_t offset) noexcept {
  return f$strpos(haystack, needle.val(), offset);
}

inline Optional<int64_t> f$strpos(const string &haystack, const mixed &needle, int64_t offset) noexcept {
  if (needle.is_string()) {
    return f$strpos(haystack, needle.to_string(), offset);
  } else {
    return f$strpos(haystack, string(1, static_cast<char>(needle.to_int())), offset);
  }
}

inline Optional<int64_t> f$strrpos(const string &haystack, const mixed &needle, int64_t offset) noexcept {
  if (needle.is_string()) {
    return f$strrpos(haystack, needle.to_string(), offset);
  } else {
    return f$strrpos(haystack, string(1, static_cast<char>(needle.to_int())), offset);
  }
}

inline Optional<int64_t> f$strripos(const string &haystack, const mixed &needle, int64_t offset) noexcept {
  if (needle.is_string()) {
    return f$strripos(haystack, needle.to_string(), offset);
  } else {
    return f$strripos(haystack, string(1, static_cast<char>(needle.to_int())), offset);
  }
}

inline Optional<string> f$strstr(const string &haystack, const mixed &needle, bool before_needle) noexcept {
  if (needle.is_string()) {
    return f$strstr(haystack, needle.to_string(), before_needle);
  } else {
    return f$strstr(haystack, string(1, static_cast<char>(needle.to_int())), before_needle);
  }
}

template<class T>
string f$strtr(const string &subject, const array<T> &replace_pairs) noexcept {
  const char *piece = subject.c_str();
  const char *piece_end = subject.c_str() + subject.size();
  string result;
  while (true) {
    const char *best_pos = nullptr;
    int64_t best_len = -1;
    string replace;
    for (typename array<T>::const_iterator p = replace_pairs.begin(); p != replace_pairs.end(); ++p) {
      const string search = f$strval(p.get_key());
      int64_t search_len = search.size();
      if (search_len == 0) {
        return subject;
      }
      const char *pos = static_cast<const char *>(memmem(static_cast<const void *>(piece), static_cast<size_t>(piece_end - piece),
                                                         static_cast<const void *>(search.c_str()), static_cast<size_t>(search_len)));
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

inline string f$strtr(const string &subject, const mixed &from, const mixed &to) noexcept {
  return f$strtr(subject, from.to_string(), to.to_string());
}

inline string f$strtr(const string &subject, const mixed &replace_pairs) noexcept {
  return f$strtr(subject, replace_pairs.as_array("strtr"));
}

string f$xor_strings(const string &s, const string &t) noexcept;

int64_t f$similar_text(const string &first, const string &second, double &percent = StringLibContext::get().default_similar_text_percent_stub) noexcept;

// similar_text ( string $first , string $second [, float &$percent ] ) : int

// str_concat_arg generalizes both tmp_string and string arguments;
// it can be constructed from both of them, so concat functions can operate
// on both tmp_string and string types
// there is a special (string, string) overloading for concat2 to
// allow the empty string result optimization to kick in
struct str_concat_arg {
  const char *data;
  string::size_type size;

  str_concat_arg(const string &s) noexcept
    : data{s.c_str()}
    , size{s.size()} {}
  str_concat_arg(tmp_string s) noexcept
    : data{s.data}
    , size{s.size} {}

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
string str_concat(const string &s1, const string &s2) noexcept;
string str_concat(str_concat_arg s1, str_concat_arg s2) noexcept;
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3) noexcept;
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4) noexcept;
string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4, str_concat_arg s5) noexcept;
