// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/regexp.h"

#include <cstddef>
#include <re2/re2.h>
#if ASAN_ENABLED
#include <sanitizer/lsan_interface.h>
#endif
#include "common/unicode/utf8-utils.h"

#include "runtime/critical_section.h"

int64_t preg_replace_count_dummy;

// TODO: remove when/if we migrate to pcre2
#ifndef PCRE2_ERROR_BADOFFSET
#  define PCRE2_ERROR_BADOFFSET -33
#endif
#ifndef PCRE2_UNSET
#  define PCRE2_UNSET -1
#endif

static re2::StringPiece RE2_submatch[MAX_SUBPATTERNS];
// refactor me please :(
// for i-th match(capturing group)
// submatch[2 * i]     - start position of match
// submatch[2 * i + 1] - end position of match
int32_t regexp::submatch[3 * MAX_SUBPATTERNS];
pcre_extra regexp::extra;


regexp::regexp(const string &regexp_string) {
  init(regexp_string);
}

regexp::regexp(const char *regexp_string, int64_t regexp_len) {
  init(regexp_string, regexp_len);
}

void regexp::pattern_compilation_warning(const char *function, const char *file, char const *message, ...) noexcept {
  va_list args;
  va_start (args, message);
  char buf[1024];
  vsnprintf(buf, sizeof(buf), message, args);
  va_end (args);

  if (function || file) {
    php_warning("%s [in function %s() at %s]", buf, function ? function : "unknown_function", file ? file : "unknown_file");
  } else {
    php_warning("%s", buf);
  }

  // only master process allocated regular expressions are stored on heap;
  // during these regexp usages we'll duplicate this warning
  if (use_heap_memory && !regex_compilation_warning) {
    regex_compilation_warning = strdup(buf);
  }
}

void regexp::check_pattern_compilation_warning() const noexcept {
  if (regex_compilation_warning) {
    php_warning("%s", regex_compilation_warning);
  }
}

int64_t regexp::skip_utf8_subsequent_bytes(int64_t offset, const string &subject) const noexcept {
  if (!is_utf8) {
    return offset;
  }
  // all multibyte utf8 runes consist of subsequent bytes,
  // these subsequent bytes start with 10 bit pattern
  // 0xc0 selects the two most significant bits, then we compare it to 0x80 (0b10000000)
  while (offset < int64_t{subject.size()} && ((static_cast<unsigned char>(subject[offset])) & 0xc0) == 0x80) {
    offset++;
  }
  return offset;
}

bool regexp::is_valid_RE2_regexp(const char *regexp_string, int64_t regexp_len, bool is_utf8, const char *function, const char *file) noexcept {
  int64_t brackets_depth = 0;
  bool prev_is_group = false;

  for (int64_t i = 0; i < regexp_len; i++) {
    switch (regexp_string[i]) {
      case -128 ... -1:
      case 1 ... 35:
      case 37 ... 39:
      case ',' ... '>':
      case '@':
      case 'A' ... 'Z':
      case ']':
      case '_':
      case '`':
      case 'a' ... 'z':
      case '}' ... 127:
        prev_is_group = true;
        break;
      case '$':
      case '^':
      case '|':
        prev_is_group = false;
        break;
      case 0:
        pattern_compilation_warning(function, file, "Regexp contains symbol with code 0 after %s", regexp_string);
        return false;
      case '(':
        brackets_depth++;

        if (regexp_string[i + 1] == '*') {
          return false;
        }

        if (regexp_string[i + 1] == '?') {
          if (regexp_string[i + 2] == ':') {
            i += 2;
            break;
          }
          if (regexp_string[i + 2] == ':') {
            i += 2;
            break;
          }
          return false;
        }
        prev_is_group = false;
        break;
      case ')':
        brackets_depth--;
        if (brackets_depth < 0) {
          return false;
        }
        prev_is_group = true;
        break;
      case '+':
      case '*':
        if (!prev_is_group) {
          return false;
        }

        prev_is_group = false;
        break;
      case '?':
        if (!prev_is_group && (i == 0 || (regexp_string[i - 1] != '+' && regexp_string[i - 1] != '*' && regexp_string[i - 1] != '?'))) {
          return false;
        }

        prev_is_group = false;
        break;
      case '{': {
        if (!prev_is_group) {
          return false;
        }

        i++;
        int64_t k = 0;
        while ('0' <= regexp_string[i] && regexp_string[i] <= '9') {
          i++;
          k++;
        }

        if (k > 2) {
          return false;
        }

        if (regexp_string[i] == ',') {
          i++;
        }

        k = 0;
        while ('0' <= regexp_string[i] && regexp_string[i] <= '9') {
          k++;
          i++;
        }

        if (k > 2) {
          return false;
        }

        if (regexp_string[i] != '}') {
          pattern_compilation_warning(function, file, "Wrong regexp %s", regexp_string);
          return false;
        }

        prev_is_group = false;
        break;
      }
      case '[':
        if (regexp_string[i + 1] == '^') {
          i++;
        }
        if (regexp_string[i + 1] == ']') {
          i++;
        }
        while (true) {
          while (++i < regexp_len && regexp_string[i] != '\\' && regexp_string[i] != ']' && regexp_string[i] != '[') {
          }

          if (i == regexp_len) {
            return false;
          }

          if (regexp_string[i] == '\\') {
            switch (regexp_string[i + 1]) {
              case 'x':
                if (isxdigit(regexp_string[i + 2]) &&
                    isxdigit(regexp_string[i + 3])) {
                  i += 3;
                  continue;
                }
                return false;
              case '0':
                if ('0' <= regexp_string[i + 2] && regexp_string[i + 2] <= '7' &&
                    '0' <= regexp_string[i + 3] && regexp_string[i + 3] <= '7') {
                  i += 3;
                  continue;
                }
                return false;
              case 'a':
              case 'f':
              case 'n':
              case 'r':
              case 't':
              case -128 ... '/':
              case ':' ... '@':
              case '[' ... '`':
              case '{' ... 127:
                i++;
                continue;
              case 'd':
              case 'D':
              case 's':
              case 'S':
              case 'w':
              case 'W':
                if (!is_utf8) {
                  i++;
                  continue;
                }
                return false;
              default:
                return false;
            }
          } else if (regexp_string[i] == '[') {
            if (regexp_string[i + 1] == ':') {
              return false;
            }
          } else if (regexp_string[i] == ']') {
            break;
          }
        }
        prev_is_group = true;
        break;
      case '\\':
        switch (regexp_string[i + 1]) {
          case 'x':
            if (isxdigit(regexp_string[i + 2]) &&
                isxdigit(regexp_string[i + 3])) {
              i += 3;
              break;
            }
            return false;
          case '0':
            if ('0' <= regexp_string[i + 2] && regexp_string[i + 2] <= '7' &&
                '0' <= regexp_string[i + 3] && regexp_string[i + 3] <= '7') {
              i += 3;
              break;
            }
            return false;
          case 'a':
          case 'f':
          case 'n':
          case 'r':
          case 't':
          case -128 ... '/':
          case ':' ... '@':
          case '[' ... '`':
          case '{' ... 127:
            i++;
            break;
          case 'b':
          case 'B':
          case 'd':
          case 'D':
          case 's':
          case 'S':
          case 'w':
          case 'W':
            if (!is_utf8) {
              i++;
              continue;
            }
            return false;
          default:
            return false;
        }

        prev_is_group = true;
        break;
      default: php_critical_error ("wrong char range assumed");
    }
  }
  if (brackets_depth != 0) {
    pattern_compilation_warning(function, file, "Brackets mismatch in regexp %s", regexp_string);
    return false;
  }

  return true;
}

void regexp::init(const string &regexp_string, const char *function, const char *file) {
  static char regexp_cache_storage[sizeof(array<regexp *>)];
  static array<regexp *> *regexp_cache = (array<regexp *> *)regexp_cache_storage;
  static long long regexp_last_query_num = -1;

  use_heap_memory = (dl::get_script_memory_stats().memory_limit == 0);

  if (!use_heap_memory) {
    if (dl::query_num != regexp_last_query_num) {
      new(regexp_cache_storage) array<regexp *>();
      regexp_last_query_num = dl::query_num;
    }

    regexp *re = regexp_cache->get_value(regexp_string);
    if (re != nullptr) {
      php_assert (!re->use_heap_memory);

      subpatterns_count = re->subpatterns_count;
      named_subpatterns_count = re->named_subpatterns_count;
      is_utf8 = re->is_utf8;
      use_heap_memory = re->use_heap_memory;

      subpattern_names = re->subpattern_names;

      pcre_regexp = re->pcre_regexp;
      RE2_regexp = re->RE2_regexp;

      return;
    }
  }

  init(regexp_string.c_str(), regexp_string.size(), function, file);

  if (!use_heap_memory) {
    regexp *re = static_cast <regexp *> (dl::allocate(sizeof(regexp)));
    new(re) regexp();

    re->subpatterns_count = subpatterns_count;
    re->named_subpatterns_count = named_subpatterns_count;
    re->is_utf8 = is_utf8;
    re->use_heap_memory = use_heap_memory;

    re->subpattern_names = subpattern_names;

    re->pcre_regexp = pcre_regexp;
    re->RE2_regexp = RE2_regexp;

    regexp_cache->set_value(regexp_string, re);
  }
}

void regexp::init(const char *regexp_string, int64_t regexp_len, const char *function, const char *file) {
  if (regexp_len == 0) {
    pattern_compilation_warning(function, file, "Empty regular expression");
    return;
  }

  char start_delimiter = regexp_string[0], end_delimiter;
  switch (start_delimiter) {
    case '(':
      end_delimiter = ')';
      break;
    case '[':
      end_delimiter = ']';
      break;
    case '{':
      end_delimiter = '}';
      break;
    case '>':
      end_delimiter = '>';
      break;
    case '!' ... '\'':
    case '*' ... '/':
    case ':':
    case ';':
    case '=':
    case '?':
    case '@':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
      end_delimiter = start_delimiter;
      break;
    default:
      pattern_compilation_warning(function, file, "Wrong start delimiter in regular expression \"%s\"", regexp_string);
      return;
  }

  int64_t regexp_end = regexp_len - 1;
  while (regexp_end > 0 && regexp_string[regexp_end] != end_delimiter) {
    regexp_end--;
  }

  if (regexp_end == 0) {
    pattern_compilation_warning(function, file, "No ending matching delimiter '%c' found in regexp: %s", end_delimiter, regexp_string);
    return;
  }

  static_SB.clean().append(regexp_string + 1, static_cast<size_t>(regexp_end - 1));

  use_heap_memory = (dl::get_script_memory_stats().memory_limit == 0);

  auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator(!use_heap_memory);

  is_utf8 = false;
  int32_t pcre_options = 0;
  RE2::Options RE2_options(RE2::Latin1);
  RE2_options.set_log_errors(false);
  bool can_use_RE2 = true;

  for (int64_t i = regexp_end + 1; i < regexp_len; i++) {
    switch (regexp_string[i]) {
      case 'i':
        pcre_options |= PCRE_CASELESS;
        RE2_options.set_case_sensitive(false);
        break;
      case 'm':
        pcre_options |= PCRE_MULTILINE;
        RE2_options.set_one_line(false);
        can_use_RE2 = false;//supported by RE2::Regexp but disabled in an interface while not using posix_syntax
        break;
      case 's':
        pcre_options |= PCRE_DOTALL;
        RE2_options.set_dot_nl(true);
        break;
      case 'x':
        pcre_options |= PCRE_EXTENDED;
        can_use_RE2 = false;
        break;

      case 'A':
        pcre_options |= PCRE_ANCHORED;
        can_use_RE2 = false;
        break;
      case 'D':
        pcre_options |= PCRE_DOLLAR_ENDONLY;
        can_use_RE2 = false;
        break;
      case 'S':
        pattern_compilation_warning(function, file, "Study doesn't supported");
        break;
      case 'U':
        pcre_options |= PCRE_UNGREEDY;
        can_use_RE2 = false;//supported by RE2::Regexp but there is no such an option
        break;
      case 'X':
        pcre_options |= PCRE_EXTRA;
        break;
      case 'u':
        pcre_options |= PCRE_UTF8;
#ifdef PCRE_UCP
        pcre_options |= PCRE_UCP;
#endif
        RE2_options.set_encoding(RE2::Options::EncodingUTF8);
        is_utf8 = true;
        break;

      default:
        pattern_compilation_warning(function, file, "Unknown modifier '%c' found", regexp_string[i]);
        clean();
        return;
    }
  }

  can_use_RE2 = can_use_RE2 && is_valid_RE2_regexp(static_SB.c_str(), static_SB.size(), is_utf8, function, file);

  if (is_utf8 && !mb_UTF8_check(static_SB.c_str())) {
    pattern_compilation_warning(function, file, "Regexp \"%s\" contains not UTF-8 symbols", static_SB.c_str());
    clean();
    return;
  }

  bool need_pcre = false;
  if (can_use_RE2) {
    RE2_regexp = new RE2(re2::StringPiece(static_SB.c_str(), static_SB.size()), RE2_options);
#if ASAN_ENABLED
    __lsan_ignore_object(RE2_regexp);
#endif
    if (!RE2_regexp->ok()) {
      pattern_compilation_warning(function, file, "RE2 compilation of regexp \"%s\" failed. Error %d at %s",
        static_SB.c_str(), RE2_regexp->error_code(), RE2_regexp->error().c_str());

      delete RE2_regexp;
      RE2_regexp = nullptr;
    } else {
      std::string min_str;
      std::string max_str;

      if (!RE2_regexp->PossibleMatchRange(&min_str, &max_str, 1) || min_str.empty()) {//rough estimate for "can match empty string"
        need_pcre = true;
      }
    }

    //We can not mimic PCRE now, but we can't check this. There is no such a function in the interface.
    //So just ignore this distinction
  }

  if (RE2_regexp == nullptr || need_pcre) {
    const char *error;
    int32_t erroffset = 0;
    pcre_regexp = pcre_compile(static_SB.c_str(), pcre_options, &error, &erroffset, nullptr);
#if ASAN_ENABLED
    __lsan_ignore_object(pcre_regexp);
#endif
    if (pcre_regexp == nullptr) {
      pattern_compilation_warning(function, file, "Regexp compilation failed: %s at offset %d", error, erroffset);
      clean();
      return;
    }
  }

  //compile has finished

  named_subpatterns_count = 0;
  if (RE2_regexp) {
    subpatterns_count = RE2_regexp->NumberOfCapturingGroups();
  } else {
    php_assert (pcre_fullinfo(pcre_regexp, nullptr, PCRE_INFO_CAPTURECOUNT, &subpatterns_count) == 0);

    if (subpatterns_count) {
      php_assert (pcre_fullinfo(pcre_regexp, nullptr, PCRE_INFO_NAMECOUNT, &named_subpatterns_count) == 0);

      if (named_subpatterns_count > 0) {
        subpattern_names = new string[subpatterns_count + 1];
#if ASAN_ENABLED
        __lsan_ignore_object(subpattern_names);
#endif

        int32_t name_entry_size = 0;
        php_assert (pcre_fullinfo(pcre_regexp, nullptr, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size) == 0);

        char *name_table;
        php_assert (pcre_fullinfo(pcre_regexp, nullptr, PCRE_INFO_NAMETABLE, &name_table) == 0);

        for (int64_t i = 0; i < named_subpatterns_count; i++) {
          int64_t name_id = (((unsigned char)name_table[0]) << 8) + (unsigned char)name_table[1];
          string name(name_table + 2);

          if (use_heap_memory) {
            name.set_reference_counter_to(ExtraRefCnt::for_global_const);
          }

          if (name.is_int()) {
            pattern_compilation_warning(function, file, "Numeric named subpatterns are not allowed");
          } else {
            subpattern_names[name_id] = name;
          }
          name_table += name_entry_size;
        }
      }
    }
  }
  subpatterns_count++;

  if (subpatterns_count > MAX_SUBPATTERNS) {
    pattern_compilation_warning(function, file, "Maximum number of subpatterns %d exceeded, %d subpatterns found", MAX_SUBPATTERNS, subpatterns_count);
    subpatterns_count = 0;

    delete RE2_regexp;
    RE2_regexp = nullptr;

    delete[] subpattern_names;
    subpattern_names = nullptr;
    clean();
    return;
  }
}

void regexp::clean() {
  if (!use_heap_memory) {
    // Regexp is stored inside a static cache, see regexp_cache_storage
    return;
  }

  php_assert(!dl::is_malloc_replaced());

  subpatterns_count = 0;
  named_subpatterns_count = 0;
  is_utf8 = false;
  use_heap_memory = false;

  if (pcre_regexp != nullptr) {
    pcre_free(pcre_regexp);
    pcre_regexp = nullptr;
  }

  delete RE2_regexp;
  RE2_regexp = nullptr;

  delete[] subpattern_names;
  subpattern_names = nullptr;
}

regexp::~regexp() {
  clean();
  free(regex_compilation_warning);
}


int64_t regexp::pcre_last_error;

int64_t regexp::exec(const string &subject, int64_t offset, bool second_try) const {
  if (RE2_regexp && !second_try) {
    {
      dl::CriticalSectionGuard critical_section;
      auto malloc_replacement_guard = make_malloc_replacement_with_script_allocator(!use_heap_memory);

      re2::StringPiece text(subject.c_str(), subject.size());
      bool matched = RE2_regexp->Match(text, static_cast<int32_t>(offset), subject.size(), RE2::UNANCHORED, RE2_submatch, subpatterns_count);
      if (!matched) {
        return 0;
      }
    }

    int64_t count = -1;
    for (int64_t i = 0; i < subpatterns_count; i++) {
      if (RE2_submatch[i].data()) {
        submatch[i + i]     = static_cast<int32_t>(RE2_submatch[i].data() - subject.c_str());
        submatch[i + i + 1] = static_cast<int32_t>(submatch[i + i] + RE2_submatch[i].size());
        count = i;
      } else {
        submatch[i + i] = PCRE2_UNSET;
        submatch[i + i + 1] = PCRE2_UNSET;
      }
    }
    php_assert (count >= 0);

    return count + 1;
  }

  php_assert (pcre_regexp);

  int32_t options = second_try ? PCRE_NO_UTF8_CHECK | PCRE_NOTEMPTY_ATSTART : PCRE_NO_UTF8_CHECK;
  dl::enter_critical_section();//OK
  int64_t count = pcre_exec(pcre_regexp, &extra, subject.c_str(), subject.size(),
                            static_cast<int32_t>(offset), options, submatch, 3 * subpatterns_count);
  dl::leave_critical_section();

  php_assert (count != 0);
  if (count == PCRE_ERROR_NOMATCH) {
    return 0;
  }
  if (count < 0) {
    pcre_last_error = count;
    return 0;
  }

  return count;
}


Optional<int64_t> regexp::match(const string &subject, bool all_matches) const {
  pcre_last_error = 0;

  check_pattern_compilation_warning();
  if (pcre_regexp == nullptr && RE2_regexp == nullptr) {
    return false;
  }

  if (is_utf8 && !mb_UTF8_check(subject.c_str())) {
    pcre_last_error = PCRE_ERROR_BADUTF8;
    return false;
  }

  bool second_try = false;//set after matching an empty string
  pcre_last_error = 0;

  int64_t result = 0;
  int64_t offset = 0;
  while (offset <= int64_t{subject.size()}) {
    if (exec(subject, offset, second_try) == 0) {
      if (second_try) {
        second_try = false;
        offset = skip_utf8_subsequent_bytes(offset + 1, subject);
        continue;
      }

      break;
    }

    result++;

    if (!all_matches) {
      break;
    }

    second_try = (submatch[0] == submatch[1]);

    offset = submatch[1];
  }

  if (pcre_last_error != 0) {
    return false;
  }

  return result;
}

Optional<int64_t> regexp::match(const string &subject, mixed &matches, bool all_matches, int64_t offset) const {
  pcre_last_error = 0;

  check_pattern_compilation_warning();
  if (pcre_regexp == nullptr && RE2_regexp == nullptr) {
    matches = array<mixed>();
    return false;
  }

  if (all_matches) {
    matches = array<mixed>(array_size(subpatterns_count, named_subpatterns_count, named_subpatterns_count == 0));
    for (int32_t i = 0; i < subpatterns_count; i++) {
      if (named_subpatterns_count && !subpattern_names[i].empty()) {
        matches.set_value(subpattern_names[i], array<mixed>());
      }
      matches.push_back(array<mixed>());
    }
  }

  bool second_try = false;//set after matching an empty string
  pcre_last_error = 0;

  int64_t result = 0;
  offset = subject.get_correct_offset(offset);
  if (offset > subject.size()) {
    matches = array<mixed>();
    pcre_last_error = PCRE2_ERROR_BADOFFSET;
    return false;
  }

  if (is_utf8 && offset && is_invalid_utf8_first_byte(subject[offset])) {
    matches = array<mixed>{};
    pcre_last_error = PCRE_ERROR_BADUTF8_OFFSET;
    return false;
  }
  if (is_utf8 && !mb_UTF8_check(subject.c_str())) {
    matches = array<mixed>{};
    pcre_last_error = PCRE_ERROR_BADUTF8;
    return false;
  }

  while (offset <= int64_t{subject.size()}) {
    const int64_t count = exec(subject, offset, second_try);

    if (count == 0) {
      if (second_try) {
        second_try = false;
        offset = skip_utf8_subsequent_bytes(offset + 1, subject);
        continue;
      }

      if (!all_matches) {
        matches = array<mixed>();
      }

      break;
    }

    result++;

    if (all_matches) {
      for (int32_t i = 0; i < subpatterns_count; i++) {
        const string match_str(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]);
        if (named_subpatterns_count && !subpattern_names[i].empty()) {
          matches[subpattern_names[i]].push_back(match_str);
        }
        matches[i].push_back(match_str);
      }
    } else {
      array<mixed> result_set(array_size(count, named_subpatterns_count, named_subpatterns_count == 0));

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

      matches = result_set;
      break;
    }

    second_try = (submatch[0] == submatch[1]);

    offset = submatch[1];
  }

  if (pcre_last_error != 0) {
    return false;
  }

  return result;
}

Optional<int64_t> regexp::match(const string &subject, mixed &matches, int64_t flags, bool all_matches, int64_t offset) const {
  pcre_last_error = 0;

  check_pattern_compilation_warning();
  if (pcre_regexp == nullptr && RE2_regexp == nullptr) {
    matches = array<mixed>();
    return false;
  }

  bool offset_capture = false;
  if (flags & PREG_OFFSET_CAPTURE) {
    flags &= ~PREG_OFFSET_CAPTURE;
    offset_capture = true;
  }

  bool pattern_order = all_matches;
  if (all_matches) {
    if (flags & PREG_PATTERN_ORDER) {
      flags &= ~PREG_PATTERN_ORDER;
    } else if (flags & PREG_SET_ORDER) {
      flags &= ~PREG_SET_ORDER;
      pattern_order = false;
    }
  }

  if (flags) {
    php_warning("Invalid parameter flags specified to preg_match%s", all_matches ? "_all" : "");
  }

  if (pattern_order) {
    matches = array<mixed>(array_size(subpatterns_count, named_subpatterns_count, named_subpatterns_count == 0));
    for (int32_t i = 0; i < subpatterns_count; i++) {
      if (named_subpatterns_count && !subpattern_names[i].empty()) {
        matches.set_value(subpattern_names[i], array<mixed>());
      }
      matches.push_back(array<mixed>());
    }
  } else {
    matches = array<mixed>();
  }

  bool second_try = false;//set after matching an empty string

  int64_t result = 0;
  auto empty_match = array<mixed>::create(string(), PCRE2_UNSET);
  offset = subject.get_correct_offset(offset);
  if (offset > subject.size()) {
    matches = array<mixed>();
    pcre_last_error = PCRE2_ERROR_BADOFFSET;
    return false;
  }

  if (is_utf8 && offset && is_invalid_utf8_first_byte(subject[offset])) {
    matches = array<mixed>{};
    pcre_last_error = PCRE_ERROR_BADUTF8_OFFSET;
    return false;
  }
  if (is_utf8 && !mb_UTF8_check(subject.c_str())) {
    matches = array<mixed>{};
    pcre_last_error = PCRE_ERROR_BADUTF8;
    return false;
  }

  while (offset <= int64_t{subject.size()}) {
    const int64_t count = exec(subject, offset, second_try);

    if (count == 0) {
      if (second_try) {
        second_try = false;
        offset = skip_utf8_subsequent_bytes(offset + 1, subject);
        continue;
      }

      break;
    }

    result++;

    if (pattern_order) {
      // push_match() is almost preg_add_match(), but we do a push_back for named matches here;
      // TODO: can we generalize match collection? offset_capture handling is also copied in several places
      const auto push_match = [this](mixed &matches, int i, const mixed &match) {
        if (named_subpatterns_count && !subpattern_names[i].empty()) {
          matches[subpattern_names[i]].push_back(match);
        }
        matches[i].push_back(match);
      };

      for (int32_t i = 0; i < subpatterns_count; i++) {
        const string match_str(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]);
        if (offset_capture && i < count) {
          auto match = array<mixed>::create(match_str, submatch[i + i]);
          push_match(matches, i, match);
        } else {
          if (offset_capture) {
            push_match(matches, i, empty_match);
          } else {
            push_match(matches, i, match_str);
          }
        }
      }
    } else {
      array<mixed> result_set(array_size(count, named_subpatterns_count, named_subpatterns_count == 0));

      if (named_subpatterns_count) {
        for (int64_t i = 0; i < count; i++) {
          const string match_str(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]);

          if (offset_capture) {
            preg_add_match(result_set, array<mixed>::create(match_str, submatch[i + i]), subpattern_names[i]);
          } else {
            preg_add_match(result_set, match_str, subpattern_names[i]);
          }
        }
      } else {
        for (int64_t i = 0; i < count; i++) {
          const string match_str(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]);

          if (offset_capture) {
            result_set.push_back(array<mixed>::create(match_str, submatch[i + i]));
          } else {
            result_set.push_back(match_str);
          }
        }
      }

      if (all_matches) {
        matches.push_back(result_set);
      } else {
        matches = result_set;
        break;
      }
    }

    second_try = (submatch[0] == submatch[1]);

    offset = submatch[1];
  }

  if (pcre_last_error != 0) {
    return false;
  }

  return result;
}

Optional<array<mixed>> regexp::split(const string &subject, int64_t limit, int64_t flags) const {
  pcre_last_error = 0;

  check_pattern_compilation_warning();
  if (pcre_regexp == nullptr && RE2_regexp == nullptr) {
    return false;
  }

  if (is_utf8 && !mb_UTF8_check(subject.c_str())) {
    pcre_last_error = PCRE_ERROR_BADUTF8;
    return false;
  }

  bool no_empty       = flags & PREG_SPLIT_NO_EMPTY;
  bool delim_capture  = flags & PREG_SPLIT_DELIM_CAPTURE;
  bool offset_capture = flags & PREG_SPLIT_OFFSET_CAPTURE;

  if (flags & ~(PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_OFFSET_CAPTURE)) {
    php_warning("Invalid parameter flags specified to preg_split");
  }

  if (limit == 0 || limit == -1) {
    limit = INT_MAX;
  }

  int64_t offset = 0;
  int64_t last_match = 0;
  bool second_try = false;

  array<mixed> result;
  while (offset <= int64_t{subject.size()} && limit > 1) {
    int64_t count = exec(subject, offset, second_try);

    if (count == 0) {
      if (second_try) {
        second_try = false;
        offset = skip_utf8_subsequent_bytes(offset + 1, subject);
        continue;
      }

      break;
    }

    if (submatch[0] != last_match || !no_empty) {
      string match_str(subject.c_str() + last_match, static_cast<string::size_type>(submatch[0] - last_match));

      if (offset_capture) {
        result.push_back(array<mixed>::create(match_str, last_match));
      } else {
        result.push_back(match_str);
      }

      limit--;
    }

    if (submatch[1] >= 0) {
      last_match = submatch[1];
    }

    if (delim_capture) {
      for (int64_t i = 1; i < count; i++) {
        if (submatch[i + i + 1] != submatch[i + i] || !no_empty) {
          string match_str(subject.c_str() + submatch[i + i], submatch[i + i + 1] - submatch[i + i]);

          if (offset_capture) {
            result.push_back(array<mixed>::create(match_str, submatch[i + i]));
          } else {
            result.push_back(match_str);
          }
        }
      }
    }

    second_try = (submatch[0] == submatch[1]);

    offset = submatch[1];
  }

  if (last_match < int64_t{subject.size()} || !no_empty) {
    string match_str(subject.c_str() + last_match, static_cast<string::size_type>(subject.size() - last_match));

    if (offset_capture) {
      result.push_back(array<mixed>::create(match_str, last_match));
    } else {
      result.push_back(match_str);
    }
  }

  if (pcre_last_error != 0) {
    return false;
  }

  return result;
}

int64_t regexp::last_error() {
  switch (pcre_last_error) {
    case PHP_PCRE_NO_ERROR:
      return PHP_PCRE_NO_ERROR;
    case PCRE_ERROR_MATCHLIMIT:
      return PHP_PCRE_BACKTRACK_LIMIT_ERROR;
    case PCRE_ERROR_RECURSIONLIMIT:
      return PHP_PCRE_RECURSION_LIMIT_ERROR;
    case PCRE_ERROR_BADUTF8:
      return PHP_PCRE_BAD_UTF8_ERROR;
    case PCRE_ERROR_BADUTF8_OFFSET:
      return PREG_BAD_UTF8_OFFSET_ERROR;
    case PCRE2_ERROR_BADOFFSET:
      return PHP_PCRE_INTERNAL_ERROR;
    default:
      php_assert (0);
      exit(1);
  }
}


string f$preg_quote(const string &str, const string &delimiter) {
  const string::size_type len = str.size();

  static_SB.clean().reserve(4 * len);

  for (string::size_type i = 0; i < len; i++) {
    switch (str[i]) {
      case '.':
      case '\\':
      case '+':
      case '*':
      case '?':
      case '[':
      case '^':
      case ']':
      case '$':
      case '(':
      case ')':
      case '{':
      case '}':
      case '=':
      case '!':
      case '>':
      case '<':
      case '|':
      case ':':
      case '-':
      case '#':
        static_SB.append_char('\\');
        static_SB.append_char(str[i]);
        break;
      case '\0':
        static_SB.append_char('\\');
        static_SB.append_char('0');
        static_SB.append_char('0');
        static_SB.append_char('0');
        break;
      default:
        if (!delimiter.empty() && str[i] == delimiter[0]) {
          static_SB.append_char('\\');
        }
        static_SB.append_char(str[i]);
        break;
    }
  }

  return static_SB.str();
}

void regexp::global_init() {
  extra.flags = PCRE_EXTRA_MATCH_LIMIT | PCRE_EXTRA_MATCH_LIMIT_RECURSION;
  extra.match_limit = PCRE_BACKTRACK_LIMIT;
  extra.match_limit_recursion = PCRE_RECURSION_LIMIT;
}

void global_init_regexp_lib() {
  regexp::global_init();
}

