#include "mbstring.h"
#include "runtime/exception.h"

#include "common/unicode/unicode-utils.h"
#include "common/unicode/utf8-utils.h"

#define MIN(a, b)  (((a)<(b))?(a):(b))
#define MBFL_SUBSTR_UNTIL_END ((size_t) -1)

bool mb_UTF8_check(const char *s) {
  do {
#define CHECK(condition) if (!(condition)) {return false;}
    unsigned int a = (unsigned char)(*s++);
    if ((a & 0x80) == 0) {
      if (a == 0) {
        return true;
      }
      continue;
    }

    CHECK ((a & 0x40) != 0);

    unsigned int b = (unsigned char)(*s++);
    CHECK((b & 0xc0) == 0x80);
    if ((a & 0x20) == 0) {
      CHECK((a & 0x1e) > 0);
      continue;
    }

    unsigned int c = (unsigned char)(*s++);
    CHECK((c & 0xc0) == 0x80);
    if ((a & 0x10) == 0) {
      int x = (((a & 0x0f) << 6) | (b & 0x20));
      CHECK(x != 0 && x != 0x360);//surrogates
      continue;
    }

    unsigned int d = (unsigned char)(*s++);
    CHECK((d & 0xc0) == 0x80);
    if ((a & 0x08) == 0) {
      int t = (((a & 0x07) << 6) | (b & 0x30));
      CHECK(0 < t && t < 0x110);//end of unicode
      continue;
    }

    return false;
#undef CHECK
  } while (true);

  php_assert (0);
}

static bool is_detect_incorrect_encoding_names_warning{false};

#ifdef MBFL
extern "C" {
	#include <kphp/libmbfl/mbfl/mbfilter.h>
#include <kphp/libmbfl/mbfl/mbfilter_wchar.h>
}

#define KPHP_UNICODE_CASE_UPPER        0
#define KPHP_UNICODE_CASE_LOWER        1
#define KPHP_UNICODE_CASE_TITLE        2
#define KPHP_UNICODE_CASE_FOLD         3
#define KPHP_UNICODE_CASE_UPPER_SIMPLE 4
#define KPHP_UNICODE_CASE_LOWER_SIMPLE 5
#define KPHP_UNICODE_CASE_TITLE_SIMPLE 6
#define KPHP_UNICODE_CASE_FOLD_SIMPLE  7
#define KPHP_UNICODE_CASE_MODE_MAX     7

static const char * DEFAULT_ENCODING = "UTF-8" ;

static inline int mbfl_is_error(size_t len) {
  return len >= (size_t) -16;
}

mbfl_string *convert_encoding(const char *str, const char *to, const char *from) {

	int len = strlen(str);
	enum mbfl_no_encoding from_encoding, to_encoding;
	mbfl_buffer_converter *convd = NULL;
	mbfl_string _string, result, *ret;

	/* from internal to mbfl */
	from_encoding = mbfl_name2no_encoding(from);
	to_encoding = mbfl_name2no_encoding(to);

	/* init buffer mbfl strings */
	mbfl_string_init(&_string);
	mbfl_string_init(&result);
	_string.no_encoding = from_encoding;
	_string.len = len;
	_string.val = (unsigned char*)str;

	/* converting */
	convd = mbfl_buffer_converter_new(from_encoding, to_encoding, 0);
	ret = mbfl_buffer_converter_feed_result(convd, &_string, &result);
	mbfl_buffer_converter_delete(convd);

	/* fix converting with multibyte encodings */
	if (len % 2 != 0 && ret->len % 2 == 0 && len < ret->len) {
		ret->len++;
		ret->val[ret->len-1] = 63;
	}
	
	return ret;
}

bool check_encoding(const char *value, const char *encoding) {

	/* init buffer mbfl strins */
	mbfl_string _string;
	mbfl_string_init(&_string);
	_string.val = (unsigned char*)value;
	_string.len = strlen((char*)value);

	/* from internal to mbfl */
	const mbfl_encoding *enc = mbfl_name2encoding(encoding);

	/* get all supported encodings */
	const mbfl_encoding **encs = mbfl_get_supported_encodings();
	int len = sizeof(**encs);

	/* identify encoding of input string */
	/* Warning! String can be represented in different encodings, so check needed */
	const mbfl_encoding *i_enc = mbfl_identify_encoding2(&_string, encs, len, 1);

	/* perform convering */
	const char *i_enc_str = (const char*)convert_encoding(value, i_enc->name, enc->name)->val;
	const char *enc_str = (const char*)convert_encoding(i_enc_str, enc->name, i_enc->name)->val;

	/* check equality */
	/* Warning! strcmp not working, because of different encodings */
	bool res = true;
	for (int i = 0; i < strlen(enc_str); i++)
		if (enc_str[i] != value[i]) {
			res = false;
			break;
		}

	free((void*)i_enc_str);
	free((void*)enc_str);
	return res;
}

// TODO: check for array as value
mixed f$mb_convert_encoding(const mixed &str, const string &to_encoding, const mixed &from_encoding) {

	if (str.is_string() && from_encoding.is_string()) {
		const string &s = str.to_string();
		const string &from = from_encoding.to_string();

		const char *c_string = s.c_str();
		const char *c_to_encoding = to_encoding.c_str();
		const char *c_from_encoding = from.c_str();

		/* perform convertion */
		mbfl_string *ret = convert_encoding(c_string, c_to_encoding, c_from_encoding);
		string res = string((const char*)ret->val, ret->len);

		/* check if string represents in from_encoding, magic number 63 - '?' in ASCII */
		if (!check_encoding(c_string, c_from_encoding)) res = string(strlen(c_string), (char)63);

		return res;
	}
	return 0;
}

// TODO: check for optional value
bool f$mb_check_encoding(const mixed &value, const Optional<string> &encoding) {
	const string &val = value.to_string();
	const string &enc = encoding.val();
	const char *c_value = val.c_str();
	const char *c_encoding = enc.c_str();
	return check_encoding(c_value, c_encoding);
}

static const mbfl_encoding *mb_get_encoding(const Optional<string> &enc_name) {
  if (enc_name.has_value()) {
    // no caching unlike PHP version - can be changed if we're going to add mbstring config
    const mbfl_encoding *encoding;
    encoding = mbfl_name2encoding(enc_name.val().c_str());
    if (!encoding) {
      return NULL;
    } else {
      return encoding;
    }
  }
  return mbfl_name2encoding(DEFAULT_ENCODING); // change if we are going to use current encoding
}

int64_t f$mb_strlen(const string &str, const Optional<string> &enc_name){
  const mbfl_encoding *encoding = mb_get_encoding(enc_name);
  if (!encoding) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_strlen", enc_name.val().c_str());
  }
  mbfl_string _string;
  mbfl_string_init(&_string);
  _string.no_encoding = encoding->no_encoding;
  _string.len = str.size();
  _string.val = (unsigned char*)str.c_str();

  size_t n = mbfl_strlen(&_string);

  if (mbfl_is_error(n)) {
    php_critical_error ("error working with \"%s\" string", str.c_str());
  }

  return (int64_t) n;

}

string f$mb_strcut(const string &str, const int64_t start, const Optional<int64_t> &length, const Optional<string> &encoding){
  int64_t _start, _length;
  bool len_is_null = !length.has_value();
  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_strcut", encoding.val().c_str());
  }

  mbfl_string _string, result, *ret;
  mbfl_string_init(&_string);
  _string.no_encoding = enc->no_encoding;
  _string.len = str.size();
  _string.val = (unsigned char*)str.c_str();

  if (len_is_null) {
    _length = _string.len;
  } else {
    _length = length.val();
  }

  _start = start;
  if (start < 0) {
    _start = _string.len + start;
    if (_start < 0) {
      _start = 0;
    }
  }

  if (_length < 0) {
    _length = (_string.len - _start) + _length;
    if (_length < 0) {
      _length = 0;
    }
  }

  if (_start > _length) {
    return string();
  }

  ret = mbfl_strcut(&_string, &result, _start, _length);
  php_assert(ret != NULL);
  return string((const char*) ret->val, ret->len);
}


string f$mb_substr(const string &str, const int64_t start, const Optional<int64_t> &length, const Optional<string> &encoding){
  size_t real_start, real_len;
  bool len_is_null = !length.has_value();

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_substr", encoding.val().c_str());
  }

  mbfl_string _string, result, *ret;
  mbfl_string_init(&_string);
  _string.no_encoding = enc->no_encoding;
  _string.len = str.size();
  _string.val = (unsigned char*)str.c_str();

  size_t mblen = 0;
  if (start < 0 || (!len_is_null && val(length) < 0)) {
    mblen = mbfl_strlen(&_string);
  }

  if (start >= 0) {
    real_start = (size_t) start;
  } else if (-start < mblen) {
    real_start = mblen + start;
  } else {
    real_start = 0;
  }

  /* if "length" position is negative, set it to the length
         * needed to stop that many chars from the end of the string */
  if (len_is_null) {
    real_len = mbfl_strlen(&_string) + 1;
  } else if (val(length) >= 0) {
    real_len = (size_t) val(length);
  } else if (real_start < mblen && - val(length) < mblen - real_start) {
    real_len = (mblen - real_start) + val(length);
  } else {
    real_len = 0;
  }

  ret = mbfl_substr(&_string, &result, real_start, real_len);
  php_assert(ret != NULL);
  return string((const char*) ret->val, ret->len);
}

int64_t f$mb_substr_count(const string &haystack, const string &needle, const Optional<string> &encoding){

  size_t n;
  mbfl_string _haystack, _needle;

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_substr_count", encoding.val().c_str());
  }

  mbfl_string_init(&_haystack);
  _haystack.no_encoding = enc->no_encoding;
  _haystack.len = haystack.size();
  _haystack.val = (unsigned char*) haystack.c_str();

  mbfl_string_init(&_needle);
  _needle.no_encoding = enc->no_encoding;
  _needle.len = needle.size();
  _needle.val = (unsigned char*) needle.c_str();

  if (needle.size() <= 0) {
    php_warning("empty substring");
  }

  n = mbfl_substr_count(&_haystack, &_needle);

  if (mbfl_is_error(n)) {
    php_critical_error ("internal error");
  }

  return (int64_t) n;
}

string mb_convert_case(const string &str, const int64_t mode, const Optional<string> &encoding){

  mixed utf_8 = f$mb_convert_encoding(str, string("UTF_8"), encoding.val());

  if (utf_8.is_string()) {
    string utf_8_str = utf_8.to_string();
    if (strcmp(encoding.val().c_str(), "UTF_8")){
      utf_8_str = str;
    }

    int len = utf_8_str.size();
    string unicode_res(len * 3, false);
    const char *s = utf_8_str.c_str();
    int p = 0, ch = 0, res_len = 0;

    switch(mode) {
      case KPHP_UNICODE_CASE_UPPER:
        while ((p = get_char_utf8(&ch, s)) > 0) {
          s += p;
          res_len += put_char_utf8(unicode_toupper(ch), &unicode_res[res_len]);
        }
        break;

      case KPHP_UNICODE_CASE_LOWER:
        while ((p = get_char_utf8(&ch, s)) > 0) {
          s += p;
          res_len += put_char_utf8(unicode_tolower(ch), &unicode_res[res_len]);
        }
        break;
    }

    if (p < 0) {
      php_warning("Incorrect UTF-8 string \"%s\" in function mb_convert_case", utf_8_str.c_str());
    }
    unicode_res.shrink(res_len);

    mixed res = f$mb_convert_encoding(unicode_res, encoding.val(), string("UTF-8"));

    if (res.is_string()) {
      return res.to_string();
    }
    else {
        php_critical_error ("encoding \"%s\" isn't supported in mb_convert_case", encoding.val().c_str());
    }
  }
  else {
    php_critical_error ("encoding \"%s\" isn't supported in mb_convert_case", encoding.val().c_str());
  }

//  if (mode < 0 || mode > PHP_UNICODE_CASE_MODE_MAX) {
//    php_critical_error ("case mode isn't supported");
//  }

//  if (mode != PHP_UNICODE_CASE_UPPER || mode != PHP_UNICODE_CASE_LOWER) {
//    php_critical_error ("case mode isn't supported");
//  }
//
//  struct convert_case_data data;
//  mbfl_convert_filter *from_wchar, *to_wchar;
//  mbfl_string result, *result_ptr;
//
//  mbfl_memory_device device;
//  mbfl_memory_device_init(&device, str.size() + 1, 0);
//
//  /* encoding -> wchar filter */
//  to_wchar = mbfl_convert_filter_new(enc->no_encoding,
//                                     (&mbfl_encoding_wchar)->no_encoding, convert_case_filter, NULL, &data);
//  if (to_wchar == NULL) {
//    mbfl_memory_device_clear(&device);
//    php_critical_error ("encoding isn't supported");
//  }
//
//  /* wchar -> encoding filter */
//  from_wchar = mbfl_convert_filter_new((&mbfl_encoding_wchar)->no_encoding, enc->no_encoding, mbfl_memory_device_output,
//                                       NULL, &device);
//  if (from_wchar == NULL) {
//    mbfl_convert_filter_delete(to_wchar);
//    mbfl_memory_device_clear(&device);
//    php_critical_error ("encoding isn't supported");
//  }
//
//  data.next_filter = from_wchar;
//  data.no_encoding = enc->no_encoding;
//  data.case_mode = mode;
//  data.title_mode = 0;
//
//  {
//    /* feed data */
//    const unsigned char *p = (const unsigned char *) str.c_str();
//    size_t n = str.size();
//    while (n > 0) {
//      if ((*to_wchar->filter_function)(*p++, to_wchar) < 0) {
//        break;
//      }
//      n--;
//    }
//  }
//
//  mbfl_convert_filter_flush(to_wchar);
//  mbfl_convert_filter_flush(from_wchar);
//  result_ptr = mbfl_memory_device_result(&device, &result);
//  mbfl_convert_filter_delete(to_wchar);
//  mbfl_convert_filter_delete(from_wchar);
//
//  if (!result_ptr) {
//    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, string("mbfl error", 10)));
//  }
//
//  return string((const char*) result_ptr->val, result_ptr->len);
}

string f$mb_strtoupper(const string &str, const Optional<string> &encoding){
  return mb_convert_case(str, KPHP_UNICODE_CASE_UPPER, encoding);
}

string f$mb_strtolower(const string &str, const Optional<string> &encoding){
  return mb_convert_case(str, KPHP_UNICODE_CASE_LOWER, encoding);
}

int64_t f$mb_strwidth(const string &str, const Optional<string> &encoding){
  size_t n;

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_strwidth", encoding.val().c_str());
  }

  mbfl_string _string;
  mbfl_string_init(&_string);
  _string.no_encoding = enc->no_encoding;
  _string.len = str.size();
  _string.val = (unsigned char*)str.c_str();

  n = mbfl_strwidth(&_string);

  return n;
}

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding){
  int reverse = 0;
  size_t real_offset = offset;
  mbfl_string _haystack, _needle;
  size_t n;

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_strpos", encoding.val().c_str());
  }

  mbfl_string_init(&_haystack);
  _haystack.no_encoding = enc->no_encoding;
  _haystack.len = haystack.size();
  _haystack.val = (unsigned char*) haystack.c_str();

  mbfl_string_init(&_needle);
  _needle.no_encoding = enc->no_encoding;
  _needle.len = needle.size();
  _needle.val = (unsigned char*) needle.c_str();

  if (real_offset != 0) {
    size_t slen = mbfl_strlen(&_haystack);
    if (offset < 0) {
      real_offset += slen;
    }
    if (real_offset > slen) {
      php_warning ("offset not contained in string");
      return false;
    }
  }

  if (needle.size() <= 0) {
    php_warning ("empty delimiter");
    return false;
  }

  n = mbfl_strpos(&_haystack, &_needle, real_offset, reverse);
  if (!mbfl_is_error(n)){
    return n;
  } else {
    switch (-n) {
      case 1:
        break;
      case 2:
        php_warning ("Needle has not positive length");
        break;
      case 4:
        php_warning ("Unknown encoding or conversion error");
        break;
      case 8:
        php_warning ("Argument is empty");
        break;
      default:
        php_warning ("Unknown error in mb_strpos");
        break;
    }
    return false;
  }
}

Optional<int64_t> f$mb_strrpos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding){
  int reverse = 1;
  mbfl_string _haystack, _needle;
  size_t n;

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    php_critical_error ("encoding \"%s\" isn't supported in mb_strrpos", encoding.val().c_str());
  }

  mbfl_string_init(&_haystack);
  _haystack.no_encoding = enc->no_encoding;
  _haystack.len = haystack.size();
  _haystack.val = (unsigned char*) haystack.c_str();

  mbfl_string_init(&_needle);
  _needle.no_encoding = enc->no_encoding;
  _needle.len = needle.size();
  _needle.val = (unsigned char*) needle.c_str();

  if (offset != 0) {
    size_t haystack_char_len = mbfl_strlen(&_haystack);
    if ((offset > 0 && offset > haystack_char_len) ||
        (offset < 0 && -offset > haystack_char_len)) {
      php_warning ("Offset is greater than the length of haystack string");
      return false;
    }
  }

  n = mbfl_strpos(&_haystack, &_needle, offset, reverse);
  if (!mbfl_is_error(n)) { return n; } else { return false; }

}

Optional<int64_t> f$mb_strripos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding){
  int reverse = 1;
  int64_t real_offset = offset;
  mbfl_string _haystack, _needle;
  size_t n = (size_t) - 1;

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    return n;
  }

  if (needle.size() == 0) {
    php_warning ("Empty delimiter");
    return false;
  }

  mbfl_string_init(&_haystack);
  _haystack.no_encoding = enc->no_encoding;

  mbfl_string_init(&_needle);
  _needle.no_encoding = enc->no_encoding;

  do {
    /* We're using simple case-folding here, because we'd have to deal with remapping of
		 * offsets otherwise. */

    string lower_haystack = f$mb_strtolower(haystack, encoding);
    _haystack.len = lower_haystack.size();
    _haystack.val = (unsigned char*) lower_haystack.c_str();

    if (!_haystack.val || _haystack.len == 0) {
      break;
    }

    string lower_needle = f$mb_strtolower(needle, encoding);
    _needle.len = lower_needle.size();
    _needle.val = (unsigned char*) lower_needle.c_str();

    if (!_needle.val || _needle.len == 0) {
      break;
    }

    if (offset != 0) {
      size_t haystack_char_len = mbfl_strlen(&_haystack);

      if (reverse) {
        if ((offset > 0 && (size_t)offset > haystack_char_len) ||
            (offset < 0 && (size_t)(-offset) > haystack_char_len)) {
          php_warning("Offset is greater than the length of haystack string");
          break;
        }
      } else {
        if (offset < 0) {
          real_offset += (int64_t )haystack_char_len;
        }
        if (real_offset < 0 || (size_t)real_offset > haystack_char_len) {
          php_warning("Offset not contained in string");
          break;
        }
      }
    }

    n = mbfl_strpos(&_haystack, &_needle, real_offset, reverse);
  } while(0);

  if (!mbfl_is_error(n)) { return n; } else { return false; }
}

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding){
  int reverse = 0;
  int64_t real_offset = offset;
  mbfl_string _haystack, _needle;
  size_t n = (size_t) - 1;

  const mbfl_encoding *enc = mb_get_encoding(encoding);

  if (!enc) {
    return n;
  }

  if (needle.size() == 0) {
    php_warning ("Empty delimiter");
    return false;
  }

  mbfl_string_init(&_haystack);
  _haystack.no_encoding = enc->no_encoding;

  mbfl_string_init(&_needle);
  _needle.no_encoding = enc->no_encoding;

  do {
    /* We're using simple case-folding here, because we'd have to deal with remapping of
		 * offsets otherwise. */

    string lower_haystack = f$mb_strtolower(haystack, encoding);
    _haystack.len = lower_haystack.size();
    _haystack.val = (unsigned char*) lower_haystack.c_str();

    if (!_haystack.val || _haystack.len == 0) {
      break;
    }

    string lower_needle = f$mb_strtolower(needle, encoding);
    _needle.len = lower_needle.size();
    _needle.val = (unsigned char*) lower_needle.c_str();

    if (!_needle.val || _needle.len == 0) {
      break;
    }

    if (offset != 0) {
      size_t haystack_char_len = mbfl_strlen(&_haystack);

      if (reverse) {
        if ((offset > 0 && (size_t)offset > haystack_char_len) ||
            (offset < 0 && (size_t)(-offset) > haystack_char_len)) {
          php_warning("Offset is greater than the length of haystack string");
          break;
        }
      } else {
        if (offset < 0) {
          real_offset += (int64_t )haystack_char_len;
        }
        if (real_offset < 0 || (size_t)real_offset > haystack_char_len) {
          php_warning("Offset not contained in string");
          break;
        }
      }
    }

    n = mbfl_strpos(&_haystack, &_needle, real_offset, reverse);
  } while(0);

  if (!mbfl_is_error(n)) { return n; } else { return false; }
}

Optional<string> f$mb_strstr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding) {
  Optional<int64_t> start = f$mb_strpos(haystack, needle, 0, encoding);
  if (start.has_value()) {
    if (before_needle) {
      return f$mb_substr(haystack, 0, val(start), encoding);
    } else {
      return f$mb_substr(haystack, val(start), false, encoding);
    }
  }
  return false;
}


Optional<string> f$mb_stristr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding){
  Optional<int64_t> start = f$mb_stripos(haystack, needle, 0, encoding);
  if (start.has_value()) {
    if (before_needle) {
      return f$mb_substr(haystack, 0, val(start), encoding);
    } else {
      return f$mb_substr(haystack, val(start), false, encoding);
    }
  }
  return false;
}

Optional<string> f$mb_strrchr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding){
  Optional<int64_t> start = f$mb_strrpos(haystack, needle, 0, encoding);
  if (start.has_value()) {
    if (before_needle) {
      return f$mb_substr(haystack, 0, val(start), encoding);
    } else {
      return f$mb_substr(haystack, val(start), false, encoding);
    }
  }
  return false;
}

Optional<string> f$mb_strrichr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding){
  Optional<int64_t> start = f$mb_strripos(haystack, needle, 0, encoding);
  if (start.has_value()) {
    if (before_needle) {
      return f$mb_substr(haystack, 0, val(start), encoding);
    } else {
      return f$mb_substr(haystack, val(start), false, encoding);
    }
  }
  return false;
}

#else


static bool is_detect_incorrect_encoding_names_warning{false};

void f$set_detect_incorrect_encoding_names_warning(bool show) {
  is_detect_incorrect_encoding_names_warning = show;
}

void free_detect_incorrect_encoding_names() {
  is_detect_incorrect_encoding_names_warning = false;
}

static int mb_detect_encoding_new(const string &encoding) {
  const auto encoding_name = f$strtolower(encoding).c_str();

  if (!strcmp(encoding_name, "cp1251") || !strcmp(encoding_name, "cp-1251") || !strcmp(encoding_name, "windows-1251")) {
    return 1251;
  }

  if (!strcmp(encoding_name, "utf8") || !strcmp(encoding_name, "utf-8")) {
    return 8;
  }

  return -1;
}

static int mb_detect_encoding(const string &encoding) {
  const int result_new = mb_detect_encoding_new(encoding);

  if (strstr(encoding.c_str(), "1251")) {
    if (is_detect_incorrect_encoding_names_warning && 1251 != result_new) {
      php_warning("mb_detect_encoding returns 1251, but new will return %d, encoding %s", result_new, encoding.c_str());
    }
    return 1251;
  }
  if (strstr(encoding.c_str(), "-8")) {
    if (is_detect_incorrect_encoding_names_warning && 8 != result_new) {
      php_warning("mb_detect_encoding returns 8, but new will return %d, encoding %s", result_new, encoding.c_str());
    }
    return 8;
  }

  if (is_detect_incorrect_encoding_names_warning && -1 != result_new) {
    php_warning("mb_detect_encoding returns -1, but new will return %d, encoding %s", result_new, encoding.c_str());
  }
  return -1;
}

static int64_t mb_UTF8_strlen(const char *s) {
  int64_t res = 0;
  for (int64_t i = 0; s[i]; i++) {
    if ((((unsigned char)s[i]) & 0xc0) != 0x80) {
      res++;
    }
  }
  return res;
}

static int64_t mb_UTF8_advance(const char *s, int64_t cnt) {
  php_assert (cnt >= 0);
  int64_t i;
  for (i = 0; s[i] && cnt >= 0; i++) {
    if ((((unsigned char)s[i]) & 0xc0) != 0x80) {
      cnt--;
    }
  }
  if (cnt < 0) {
    i--;
  }
  return i;
}

static int64_t mb_UTF8_get_offset(const char *s, int64_t pos) {
  int64_t res = 0;
  for (int64_t i = 0; i < pos && s[i]; i++) {
    if ((((unsigned char)s[i]) & 0xc0) != 0x80) {
      res++;
    }
  }
  return res;
}

bool f$mb_check_encoding(const string &str, const string &encoding) {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error ("encoding \"%s\" doesn't supported in mb_check_encoding", encoding.c_str());
    return !str.empty();
  }

  if (encoding_num == 1251) {
    return true;
  }

  return mb_UTF8_check(str.c_str());
}


int64_t f$mb_strlen(const string &str, const string &encoding) {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error ("encoding \"%s\" doesn't supported in mb_strlen", encoding.c_str());
    return str.size();
  }

  if (encoding_num == 1251) {
    return str.size();
  }

  return mb_UTF8_strlen(str.c_str());
}


string f$mb_strtolower(const string &str, const string &encoding) {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error ("encoding \"%s\" doesn't supported in mb_strtolower", encoding.c_str());
    return str;
  }

  int len = str.size();
  if (encoding_num == 1251) {
    string res(len, false);
    for (int i = 0; i < len; i++) {
      switch ((unsigned char)str[i]) {
        case 'A' ... 'Z':
          res[i] = (char)(str[i] + 'a' - 'A');
          break;
        case 0xC0 ... 0xDF:
          res[i] = (char)(str[i] + 32);
          break;
        case 0x81:
          res[i] = (char)0x83;
          break;
        case 0xA3:
          res[i] = (char)0xBC;
          break;
        case 0xA5:
          res[i] = (char)0xB4;
          break;
        case 0xA1:
        case 0xB2:
        case 0xBD:
          res[i] = (char)(str[i] + 1);
          break;
        case 0x80:
        case 0x8A:
        case 0x8C ... 0x8F:
        case 0xA8:
        case 0xAA:
        case 0xAF:
          res[i] = (char)(str[i] + 16);
          break;
        default:
          res[i] = str[i];
      }
    }

    return res;
  } else {
    string res(len * 3, false);
    const char *s = str.c_str();
    int res_len = 0;
    int p;
    int ch;
    while ((p = get_char_utf8(&ch, s)) > 0) {
      s += p;
      res_len += put_char_utf8(unicode_tolower(ch), &res[res_len]);
    }
    if (p < 0) {
      php_warning("Incorrect UTF-8 string \"%s\" in function mb_strtolower", str.c_str());
    }
    res.shrink(res_len);

    return res;
  }
}

string f$mb_strtoupper(const string &str, const string &encoding) {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error ("encoding \"%s\" doesn't supported in mb_strtoupper", encoding.c_str());
    return str;
  }

  int len = str.size();
  if (encoding_num == 1251) {
    string res(len, false);
    for (int i = 0; i < len; i++) {
      switch ((unsigned char)str[i]) {
        case 'a' ... 'z':
          res[i] = (char)(str[i] + 'A' - 'a');
          break;
        case 0xE0 ... 0xFF:
          res[i] = (char)(str[i] - 32);
          break;
        case 0x83:
          res[i] = (char)(0x81);
          break;
        case 0xBC:
          res[i] = (char)(0xA3);
          break;
        case 0xB4:
          res[i] = (char)(0xA5);
          break;
        case 0xA2:
        case 0xB3:
        case 0xBE:
          res[i] = (char)(str[i] - 1);
          break;
        case 0x98:
        case 0xA0:
        case 0xAD:
          res[i] = ' ';
          break;
        case 0x90:
        case 0x9A:
        case 0x9C ... 0x9F:
        case 0xB8:
        case 0xBA:
        case 0xBF:
          res[i] = (char)(str[i] - 16);
          break;
        default:
          res[i] = str[i];
      }
    }

    return res;
  } else {
    string res(len * 3, false);
    const char *s = str.c_str();
    int res_len = 0;
    int p;
    int ch;
    while ((p = get_char_utf8(&ch, s)) > 0) {
      s += p;
      res_len += put_char_utf8(unicode_toupper(ch), &res[res_len]);
    }
    if (p < 0) {
      php_warning("Incorrect UTF-8 string \"%s\" in function mb_strtoupper", str.c_str());
    }
    res.shrink(res_len);

    return res;
  }
}

namespace {

int check_strpos_agrs(const char *func_name, const string &needle, int64_t offset, const string &encoding) noexcept {
  if (unlikely(offset < 0)) {
    php_warning("Wrong offset = %" PRIi64 " in function %s()", offset, func_name);
    return 0;
  }
  if (unlikely(needle.empty())) {
    php_warning("Parameter needle is empty in function %s()", func_name);
    return 0;
  }

  const int encoding_num = mb_detect_encoding(encoding);
  if (unlikely(encoding_num < 0)) {
    php_critical_error ("encoding \"%s\" doesn't supported in %s()", encoding.c_str(), func_name);
    return 0;
  }
  return encoding_num;
}

Optional<int64_t> mp_strpos_impl(const string &haystack, const string &needle, int64_t offset, int encoding_num) noexcept {
  if (encoding_num == 1251) {
    return f$strpos(haystack, needle, offset);
  }

  int64_t UTF8_offset = mb_UTF8_advance(haystack.c_str(), offset);
  const char *s = static_cast<const char *>(memmem(haystack.c_str() + UTF8_offset, haystack.size() - UTF8_offset, needle.c_str(), needle.size()));
  if (unlikely(s == nullptr)) {
    return false;
  }
  return mb_UTF8_get_offset(haystack.c_str() + UTF8_offset, s - (haystack.c_str() + UTF8_offset)) + offset;
}

} // namespace

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, int64_t offset, const string &encoding) noexcept {
  if (const int encoding_num = check_strpos_agrs("mb_strpos", needle, offset, encoding)) {
    return mp_strpos_impl(haystack, needle, offset, encoding_num);
  }
  return false;
}

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, int64_t offset, const string &encoding) noexcept {
  if (const int encoding_num = check_strpos_agrs("mb_stripos", needle, offset, encoding)) {
    return mp_strpos_impl(f$mb_strtolower(haystack, encoding), f$mb_strtolower(needle, encoding), offset, encoding_num);
  }
  return false;
}

string f$mb_substr(const string &str, int64_t start, const mixed &length_var, const string &encoding) {
  int encoding_num = mb_detect_encoding(encoding);
  if (encoding_num < 0) {
    php_critical_error ("encoding \"%s\" doesn't supported in mb_substr", encoding.c_str());
    return str;
  }

  int64_t length;
  if (length_var.is_null()) {
    length = std::numeric_limits<int64_t>::max();
  } else {
    length = length_var.to_int();
  }

  if (encoding_num == 1251) {
    Optional<string> res = f$substr(str, start, length);
    if (!res.has_value()) {
      return {};
    }
    return res.val();
  }

  int64_t len = mb_UTF8_strlen(str.c_str());
  if (start < 0) {
    start += len;
  }
  if (start > len) {
    start = len;
  }
  if (length < 0) {
    length = len - start + length;
  }
  if (length <= 0 || start < 0) {
    return {};
  }
  if (len - start < length) {
    length = len - start;
  }

  int64_t UTF8_start = mb_UTF8_advance(str.c_str(), start);
  int64_t UTF8_length = mb_UTF8_advance(str.c_str() + UTF8_start, length);

  return {str.c_str() + UTF8_start, static_cast<string::size_type>(UTF8_length)};
}

#endif

void f$set_detect_incorrect_encoding_names_warning(bool show) {
  is_detect_incorrect_encoding_names_warning = show;
}

void free_detect_incorrect_encoding_names() {
  is_detect_incorrect_encoding_names_warning = false;
}
