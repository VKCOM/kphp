// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <pcre.h>

#include "common/mixin/not_copyable.h"

#include "runtime/kphp_core.h"
#include "runtime/mbstring/mbstring.h"

namespace re2 {
class RE2;
} // namespace re2

extern int64_t preg_replace_count_dummy;

constexpr int64_t PREG_PATTERN_ORDER = 1;
constexpr int64_t PREG_SET_ORDER = 2;
constexpr int64_t PREG_OFFSET_CAPTURE = 4;

constexpr int64_t PREG_SPLIT_NO_EMPTY = 8;
constexpr int64_t PREG_SPLIT_DELIM_CAPTURE = 16;
constexpr int64_t PREG_SPLIT_OFFSET_CAPTURE = 32;

constexpr int64_t PCRE_RECURSION_LIMIT = 100000;
constexpr int64_t PCRE_BACKTRACK_LIMIT = 1000000;

constexpr int32_t MAX_SUBPATTERNS = 512;

enum {
  PHP_PCRE_NO_ERROR = 0,
  PHP_PCRE_INTERNAL_ERROR,
  PHP_PCRE_BACKTRACK_LIMIT_ERROR,
  PHP_PCRE_RECURSION_LIMIT_ERROR,
  PHP_PCRE_BAD_UTF8_ERROR,
  PREG_BAD_UTF8_OFFSET_ERROR
};

class regexp : vk::not_copyable {
private:
  int32_t subpatterns_count{0};
  int32_t named_subpatterns_count{0};
  bool is_utf8{false};
  bool use_heap_memory{false};

  string *subpattern_names{nullptr};

  pcre *pcre_regexp{nullptr};
  re2::RE2 *RE2_regexp{nullptr};

  char *regex_compilation_warning{nullptr};

  void clean();

  int64_t exec(const string &subject, int64_t offset, bool second_try) const;

  bool is_valid_RE2_regexp(const char *regexp_string, int64_t regexp_len, bool is_utf8, const char *function, const char *file) noexcept;

  static pcre_extra extra;

  static int64_t pcre_last_error;

  static int32_t submatch[3 * MAX_SUBPATTERNS];

  template<class T>
  inline string get_replacement(const T &replace_val, const string &subject, int64_t count) const;

  void pattern_compilation_warning(const char *function, const char *file, char const *message, ...) noexcept __attribute__ ((format (printf, 4, 5)));

  void check_pattern_compilation_warning() const noexcept;

  int64_t skip_utf8_subsequent_bytes(int64_t offset, const string &subject) const noexcept;

public:
  regexp() = default;

  explicit regexp(const string &regexp_string);
  regexp(const char *regexp_string, int64_t regexp_len);

  void init(const string &regexp_string, const char *function = nullptr, const char *file = nullptr);
  void init(const char *regexp_string, int64_t regexp_len, const char *function = nullptr, const char *file = nullptr);


  Optional<int64_t> match(const string &subject, bool all_matches) const;
  Optional<int64_t> match(const string &subject, mixed &matches, bool all_matches, int64_t offset = 0) const;

  Optional<int64_t> match(const string &subject, mixed &matches, int64_t flags, bool all_matches, int64_t offset = 0) const;

  Optional<array<mixed>> split(const string &subject, int64_t limit, int64_t flags) const;

  template<class T>
  Optional<string> replace(const T &replace_val, const string &subject, int64_t limit, int64_t &replace_count) const;

  static int64_t last_error();

  ~regexp();

  static void global_init();
};

void global_init_regexp_lib();

inline void preg_add_match(array<mixed> &v, const mixed &match, const string &name);
inline void preg_add_match(array<string> &v, const string &match, const string &name);


inline Optional<int64_t> f$preg_match(const regexp &regex, const string &subject);

inline Optional<int64_t> f$preg_match_all(const regexp &regex, const string &subject);

inline Optional<int64_t> f$preg_match(const regexp &regex, const string &subject, mixed &matches);

inline Optional<int64_t> f$preg_match_all(const regexp &regex, const string &subject, mixed &matches);

inline Optional<int64_t> f$preg_match(const regexp &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset = 0);

inline Optional<int64_t> f$preg_match_all(const regexp &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset = 0);

inline Optional<int64_t> f$preg_match(const string &regex, const string &subject);

inline Optional<int64_t> f$preg_match_all(const string &regex, const string &subject);

inline Optional<int64_t> f$preg_match(const string &regex, const string &subject, mixed &matches);

inline Optional<int64_t> f$preg_match_all(const string &regex, const string &subject, mixed &matches);

inline Optional<int64_t> f$preg_match(const string &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset = 0);

inline Optional<int64_t> f$preg_match_all(const string &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset = 0);

inline Optional<int64_t> f$preg_match(const mixed &regex, const string &subject);

inline Optional<int64_t> f$preg_match_all(const mixed &regex, const string &subject);

inline Optional<int64_t> f$preg_match(const mixed &regex, const string &subject, mixed &matches);

inline Optional<int64_t> f$preg_match_all(const mixed &regex, const string &subject, mixed &matches);

inline Optional<int64_t> f$preg_match(const mixed &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset = 0);

inline Optional<int64_t> f$preg_match_all(const mixed &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset = 0);

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
inline auto f$preg_replace(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline Optional<string> f$preg_replace(const regexp &regex, const string &replace_val, const string &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline Optional<string> f$preg_replace(const regexp &regex, const mixed &replace_val, const string &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline mixed f$preg_replace(const regexp &regex, const string &replace_val, const mixed &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline mixed f$preg_replace(const regexp &regex, const mixed &replace_val, const mixed &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T1, class T2>
inline auto f$preg_replace(const string &regex, const T1 &replace_val, const T2 &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline Optional<string> f$preg_replace(const mixed &regex, const string &replace_val, const string &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline mixed f$preg_replace(const mixed &regex, const string &replace_val, const mixed &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline Optional<string> f$preg_replace(const mixed &regex, const mixed &replace_val, const string &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline mixed f$preg_replace(const mixed &regex, const mixed &replace_val, const mixed &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace_callback(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T>
Optional<string> f$preg_replace_callback(const regexp &regex, const T &replace_val, const string &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T>
mixed f$preg_replace_callback(const regexp &regex, const T &replace_val, const mixed &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T, class T2>
auto f$preg_replace_callback(const string &regex, const T &replace_val, const T2 &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T>
Optional<string> f$preg_replace_callback(const mixed &regex, const T &replace_val, const string &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

template<class T>
mixed f$preg_replace_callback(const mixed &regex, const T &replace_val, const mixed &subject, int64_t limit = -1, int64_t &replace_count = preg_replace_count_dummy);

inline Optional<array<mixed>> f$preg_split(const regexp &regex, const string &subject, int64_t limit = -1, int64_t flags = 0);

inline Optional<array<mixed>> f$preg_split(const string &regex, const string &subject, int64_t limit = -1, int64_t flags = 0);

inline Optional<array<mixed>> f$preg_split(const mixed &regex, const string &subject, int64_t limit = -1, int64_t flags = 0);

string f$preg_quote(const string &str, const string &delimiter = string());

inline int64_t f$preg_last_error();


/*
 *
 *     IMPLEMENTATION
 *
 */


template<>
inline string regexp::get_replacement(const string &replace_val, const string &subject, int64_t count) const {
  const string::size_type len = replace_val.size();

  static_SB.clean();
  for (string::size_type i = 0; i < len; i++) {
    int64_t backref = -1;
    if (replace_val[i] == '\\' && (replace_val[i + 1] == '\\' || replace_val[i + 1] == '$')) {
      i++;
    } else if ((replace_val[i] == '\\' || replace_val[i] == '$') && '0' <= replace_val[i + 1] && replace_val[i + 1] <= '9') {
      if ('0' <= replace_val[i + 2] && replace_val[i + 2] <= '9') {
        backref = (replace_val[i + 1] - '0') * 10 + (replace_val[i + 2] - '0');
        i += 2;
      } else {
        backref = replace_val[i + 1] - '0';
        i++;
      }
    } else if (replace_val[i] == '$' && replace_val[i + 1] == '{' && '0' <= replace_val[i + 2] && replace_val[i + 2] <= '9') {
      if ('0' <= replace_val[i + 3] && replace_val[i + 3] <= '9') {
        if (replace_val[i + 4] == '}') {
          backref = (replace_val[i + 2] - '0') * 10 + (replace_val[i + 3] - '0');
          i += 4;
        }
      } else {
        if (replace_val[i + 3] == '}') {
          backref = replace_val[i + 2] - '0';
          i += 3;
        }
      }
    }

    if (backref == -1) {
      static_SB << replace_val[i];
    } else {
      if (backref < count) {
        int64_t index = backref + backref;
        static_SB.append(subject.c_str() + submatch[index],
                         static_cast<size_t>(submatch[index + 1] - submatch[index]));
      }
    }
  }
  return static_SB.str();//TODO optimize
}

template<class T>
string regexp::get_replacement(const T &replace_val, const string &subject, const int64_t count) const {
  array<string> result_set(array_size(count, named_subpatterns_count, named_subpatterns_count == 0));

  if (named_subpatterns_count) {
    for (int64_t i = 0; i < count; i++) {
      const string match_str(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]);

      preg_add_match(result_set, match_str, subpattern_names[i]);
    }
  } else {
    for (int64_t i = 0; i < count; i++) {
      result_set.push_back(string(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]));
    }
  }

  return f$strval(replace_val(result_set));
}


template<class T>
Optional<string> regexp::replace(const T &replace_val, const string &subject, int64_t limit, int64_t &replace_count) const {
  pcre_last_error = 0;
  int64_t result_count = 0;//calls can be recursive, can't write to replace_count directly

  check_pattern_compilation_warning();
  if (pcre_regexp == nullptr && RE2_regexp == nullptr) {
    return {};
  }

  if (is_utf8 && !mb_UTF8_check(subject.c_str())) {
    pcre_last_error = PCRE_ERROR_BADUTF8;
    return {};
  }

  if (limit == 0 || limit == -1) {
    limit = INT_MAX;
  }

  int64_t offset = 0;
  int64_t last_match = 0;
  bool second_try = false;

  string result;
  while (offset <= int64_t{subject.size()} && limit > 0) {
    int64_t count = exec(subject, offset, second_try);

//    fprintf (stderr, "Found %d submatches at %d:%d from pos %d, pcre_last_error = %d\n", count, submatch[0], submatch[1], offset, pcre_last_error);
    if (count == 0) {
      if (second_try) {
        second_try = false;
        do {
          offset++;
        } while (is_utf8 && offset < int64_t{subject.size()} && (static_cast<unsigned char>(subject[static_cast<string::size_type>(offset)]) & 0xc0) == 0x80);
        continue;
      }

      break;
    }

    result_count++;
    limit--;

    int64_t match_begin = submatch[0];
    offset = submatch[1];

    result.append(subject.c_str() + last_match, static_cast<string::size_type>(match_begin - last_match));
    result.append(get_replacement(replace_val, subject, count));

    second_try = (match_begin == offset);

    last_match = offset;
  }

  replace_count = result_count;
  if (pcre_last_error) {
    return {};
  }

  if (replace_count == 0) {
    return subject;
  }

  result.append(subject.c_str() + last_match, static_cast<string::size_type>(subject.size() - last_match));
  return result;
}


void preg_add_match(array<mixed> &v, const mixed &match, const string &name) {
  if (name.size()) {
    v.set_value(name, match);
  }

  v.push_back(match);
}

void preg_add_match(array<string> &v, const string &match, const string &name) {
  if (name.size()) {
    v.set_value(name, match);
  }

  v.push_back(match);
}

Optional<int64_t> f$preg_match(const regexp &regex, const string &subject) {
  return regex.match(subject, false);
}

Optional<int64_t> f$preg_match_all(const regexp &regex, const string &subject) {
  return regex.match(subject, true);
}

Optional<int64_t> f$preg_match(const regexp &regex, const string &subject, mixed &matches) {
  return regex.match(subject, matches, false);
}

Optional<int64_t> f$preg_match_all(const regexp &regex, const string &subject, mixed &matches) {
  return regex.match(subject, matches, true);
}

Optional<int64_t> f$preg_match(const regexp &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset) {
  return regex.match(subject, matches, flags, false, offset);
}

Optional<int64_t> f$preg_match_all(const regexp &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset) {
  return regex.match(subject, matches, flags, true, offset);
}

Optional<int64_t> f$preg_match(const string &regex, const string &subject) {
  return f$preg_match(regexp(regex), subject);
}

Optional<int64_t> f$preg_match_all(const string &regex, const string &subject) {
  return f$preg_match_all(regexp(regex), subject);
}

Optional<int64_t> f$preg_match(const string &regex, const string &subject, mixed &matches) {
  return f$preg_match(regexp(regex), subject, matches);
}

Optional<int64_t> f$preg_match_all(const string &regex, const string &subject, mixed &matches) {
  return f$preg_match_all(regexp(regex), subject, matches);
}

Optional<int64_t> f$preg_match(const string &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset) {
  return f$preg_match(regexp(regex), subject, matches, flags, offset);
}

Optional<int64_t> f$preg_match_all(const string &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset) {
  return f$preg_match_all(regexp(regex), subject, matches, flags, offset);
}

Optional<int64_t> f$preg_match(const mixed &regex, const string &subject) {
  return f$preg_match(regexp(regex.to_string()), subject);
}

Optional<int64_t> f$preg_match_all(const mixed &regex, const string &subject) {
  return f$preg_match_all(regexp(regex.to_string()), subject);
}

Optional<int64_t> f$preg_match(const mixed &regex, const string &subject, mixed &matches) {
  return f$preg_match(regexp(regex.to_string()), subject, matches);
}

Optional<int64_t> f$preg_match_all(const mixed &regex, const string &subject, mixed &matches) {
  return f$preg_match_all(regexp(regex.to_string()), subject, matches);
}

Optional<int64_t> f$preg_match(const mixed &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset) {
  return f$preg_match(regexp(regex.to_string()), subject, matches, flags, offset);
}

Optional<int64_t> f$preg_match_all(const mixed &regex, const string &subject, mixed &matches, int64_t flags, int64_t offset) {
  return f$preg_match_all(regexp(regex.to_string()), subject, matches, flags, offset);
}


template<class T1, class T2, class T3, class>
inline auto f$preg_replace(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace(regex, replace_val, subject.val(), limit, replace_count);
}

Optional<string> f$preg_replace(const regexp &regex, const string &replace_val, const string &subject, int64_t limit, int64_t &replace_count) {
  return regex.replace(replace_val, subject, limit, replace_count);
}

Optional<string> f$preg_replace(const regexp &regex, const mixed &replace_val, const string &subject, int64_t limit, int64_t &replace_count) {
  if (replace_val.is_array()) {
    php_warning("Parameter mismatch, pattern is a string while replacement is an array");
    return false;
  }

  return f$preg_replace(regex, replace_val.to_string(), subject, limit, replace_count);
}

mixed f$preg_replace(const regexp &regex, const string &replace_val, const mixed &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace(regex, mixed(replace_val), subject, limit, replace_count);
}

mixed f$preg_replace(const regexp &regex, const mixed &replace_val, const mixed &subject, int64_t limit, int64_t &replace_count) {
  if (replace_val.is_array()) {
    php_warning("Parameter mismatch, pattern is a string while replacement is an array");
    return false;
  }

  if (subject.is_array()) {
    replace_count = 0;
    int64_t replace_count_one;
    const array<mixed> &subject_arr = subject.as_array("");
    array<mixed> result(subject_arr.size());
    for (array<mixed>::const_iterator it = subject_arr.begin(); it != subject_arr.end(); ++it) {
      mixed cur_result = f$preg_replace(regex, replace_val.to_string(), it.get_value().to_string(), limit, replace_count_one);
      if (!cur_result.is_null()) {
        result.set_value(it.get_key(), cur_result);
        replace_count += replace_count_one;
      }
    }
    return result;
  } else {
    return f$preg_replace(regex, replace_val.to_string(), subject.to_string(), limit, replace_count);
  }
}

template<class T1, class T2>
auto f$preg_replace(const string &regex, const T1 &replace_val, const T2 &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace(regexp(regex), replace_val, subject, limit, replace_count);
}

Optional<string> f$preg_replace(const mixed &regex, const string &replace_val, const string &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace(regex, mixed(replace_val), subject, limit, replace_count);
}

mixed f$preg_replace(const mixed &regex, const string &replace_val, const mixed &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace(regex, mixed(replace_val), subject, limit, replace_count);
}

Optional<string> f$preg_replace(const mixed &regex, const mixed &replace_val, const string &subject, int64_t limit, int64_t &replace_count) {
  if (regex.is_array()) {
    Optional<string> result = subject;

    replace_count = 0;
    int64_t replace_count_one;

    if (replace_val.is_array()) {
      array<mixed>::const_iterator cur_replace_val = replace_val.begin();

      for (array<mixed>::const_iterator it = regex.begin(); it != regex.end(); ++it) {
        string replace_value;
        if (cur_replace_val != replace_val.end()) {
          replace_value = cur_replace_val.get_value().to_string();
          ++cur_replace_val;
        }

        result = f$preg_replace(it.get_value().to_string(), replace_value, result, limit, replace_count_one);
        replace_count += replace_count_one;
      }
    } else {
      string replace_value = replace_val.to_string();

      for (array<mixed>::const_iterator it = regex.begin(); it != regex.end(); ++it) {
        result = f$preg_replace(it.get_value().to_string(), replace_value, result, limit, replace_count_one);
        replace_count += replace_count_one;
      }
    }

    return result;
  } else {
    if (replace_val.is_array()) {
      php_warning("Parameter mismatch, pattern is a string while replacement is an array");
      return false;
    }

    return f$preg_replace(regex.to_string(), replace_val.to_string(), subject, limit, replace_count);
  }
}

mixed f$preg_replace(const mixed &regex, const mixed &replace_val, const mixed &subject, int64_t limit, int64_t &replace_count) {
  if (subject.is_array()) {
    replace_count = 0;
    int64_t replace_count_one;
    const array<mixed> &subject_arr = subject.as_array("");
    array<mixed> result(subject_arr.size());
    for (array<mixed>::const_iterator it = subject_arr.begin(); it != subject_arr.end(); ++it) {
      mixed cur_result = f$preg_replace(regex, replace_val, it.get_value().to_string(), limit, replace_count_one);
      if (!cur_result.is_null()) {
        result.set_value(it.get_key(), cur_result);
        replace_count += replace_count_one;
      }
    }
    return result;
  } else {
    return f$preg_replace(regex, replace_val, subject.to_string(), limit, replace_count);
  }
}

template<class T1, class T2, class T3, class>
auto f$preg_replace_callback(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace_callback(regex, replace_val, subject.val(), limit, replace_count);
}

template<class T>
Optional<string> f$preg_replace_callback(const regexp &regex, const T &replace_val, const string &subject, int64_t limit, int64_t &replace_count) {
  return regex.replace(replace_val, subject, limit, replace_count);
}

template<class T>
mixed f$preg_replace_callback(const regexp &regex, const T &replace_val, const mixed &subject, int64_t limit, int64_t &replace_count) {
  if (subject.is_array()) {
    replace_count = 0;
    int64_t replace_count_one;
    const array<mixed> &subject_arr = subject.as_array("");
    array<mixed> result(subject_arr.size());
    for (array<mixed>::const_iterator it = subject_arr.begin(); it != subject_arr.end(); ++it) {
      mixed cur_result = f$preg_replace_callback(regex, replace_val, it.get_value().to_string(), limit, replace_count_one);
      if (!cur_result.is_null()) {
        result.set_value(it.get_key(), cur_result);
        replace_count += replace_count_one;
      }
    }
    return result;
  } else {
    return f$preg_replace_callback(regex, replace_val, subject.to_string(), limit, replace_count);
  }
}

template<class T, class T2>
auto f$preg_replace_callback(const string &regex, const T &replace_val, const T2 &subject, int64_t limit, int64_t &replace_count) {
  return f$preg_replace_callback(regexp(regex), replace_val, subject, limit, replace_count);
}

template<class T>
Optional<string> f$preg_replace_callback(const mixed &regex, const T &replace_val, const string &subject, int64_t limit, int64_t &replace_count) {
  if (regex.is_array()) {
    Optional<string> result = subject;

    replace_count = 0;
    int64_t replace_count_one;

    for (array<mixed>::const_iterator it = regex.begin(); it != regex.end(); ++it) {
      result = f$preg_replace_callback(it.get_value().to_string(), replace_val, result, limit, replace_count_one);
      replace_count += replace_count_one;
    }

    return result;
  } else {
    return f$preg_replace_callback(regex.to_string(), replace_val, subject, limit, replace_count);
  }
}

template<class T>
mixed f$preg_replace_callback(const mixed &regex, const T &replace_val, const mixed &subject, int64_t limit, int64_t &replace_count) {
  if (subject.is_array()) {
    replace_count = 0;
    int64_t replace_count_one;
    const array<mixed> &subject_arr = subject.as_array("");
    array<mixed> result(subject_arr.size());
    for (array<mixed>::const_iterator it = subject_arr.begin(); it != subject_arr.end(); ++it) {
      mixed cur_result = f$preg_replace_callback(regex, replace_val, it.get_value().to_string(), limit, replace_count_one);
      if (!cur_result.is_null()) {
        result.set_value(it.get_key(), cur_result);
        replace_count += replace_count_one;
      }
    }
    return result;
  } else {
    return f$preg_replace_callback(regex, replace_val, subject.to_string(), limit, replace_count);
  }
}

Optional<array<mixed>> f$preg_split(const regexp &regex, const string &subject, int64_t limit, int64_t flags) {
  return regex.split(subject, limit, flags);
}

Optional<array<mixed>> f$preg_split(const string &regex, const string &subject, int64_t limit, int64_t flags) {
  return f$preg_split(regexp(regex), subject, limit, flags);
}

Optional<array<mixed>> f$preg_split(const mixed &regex, const string &subject, int64_t limit, int64_t flags) {
  return f$preg_split(regexp(regex.to_string()), subject, limit, flags);
}

int64_t f$preg_last_error() {
  return regexp::last_error();
}


