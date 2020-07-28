#include "runtime/url.h"

#include "common/base64.h"

#include "runtime/array_functions.h"
#include "runtime/regexp.h"

string AMPERSAND("&", 1);


//string f$base64_decode (const string &s) {
//  int result_len = (s.size() + 3) / 4 * 3;
//  string res (result_len, false);
//  result_len = base64_decode (s.c_str(), reinterpret_cast <unsigned char *> (res.buffer()), result_len + 1);
//
//  if (result_len < 0) {
//    return string();
//  }
//
//  res.shrink (result_len);
//  return res;
//}

static const short base64_reverse_table[256] = {
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
  -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
  -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
};

/* Function f$base64_decode ported from https://github.com/php/php-src/blob/master/ext/standard/base64.c#L130
 * "This product includes PHP software, freely available from <http://www.php.net/software/>".
*/
Optional<string> f$base64_decode(const string &s, bool strict) {
  /* run through the whole string, converting as we go */
  string::size_type result_len = (s.size() + 3) / 4 * 3;
  string result(result_len, false);
  int i = 0;
  string::size_type j = 0;
  int padding = 0;
  for (string::size_type pos = 0; pos < s.size(); pos++) {
    int ch = s[pos];
    if (ch == '=') {
      padding++;
      continue;
    }

    ch = base64_reverse_table[ch];
    if (!strict) {
      /* skip unknown characters and whitespace */
      if (ch < 0) {
        continue;
      }
    } else {
      /* skip whitespace */
      if (ch == -1) {
        continue;
      }
      /* fail on bad characters or if any data follows padding */
      if (ch == -2 || padding) {
        return false;
      }
    }

    switch (i % 4) {
      case 0:
        result[j] = (unsigned char)(ch << 2);
        break;
      case 1:
        result[j] = static_cast<char>(result[j] | (ch >> 4));
        result[++j] = (unsigned char)((ch & 0x0f) << 4);
        break;
      case 2:
        result[j] = static_cast<char>(result[j] | (ch >> 2));
        result[++j] = (unsigned char)((ch & 0x03) << 6);
        break;
      case 3:
        result[j] = static_cast<char>(result[j] | ch);
        ++j;
        break;
    }
    i++;
  }
  /* fail if the input is truncated (only one char in last group) */
  if (strict && i % 4 == 1) {
    return false;
  }
  /* fail if the padding length is wrong (not VV==, VVV=), but accept zero padding
   * RFC 4648: "In some circumstances, the use of padding [--] is not required" */
  if (strict && padding && (padding > 2 || (i + padding) % 4 != 0)) {
    return false;
  }

  result.shrink(j);

  return result;
}

string f$base64_encode(const string &s) {
  int result_len = (s.size() + 2) / 3 * 4;
  string res(result_len, false);
  result_len = base64_encode(reinterpret_cast <const unsigned char *> (s.c_str()), (int)s.size(), res.buffer(), result_len + 1);

  if (result_len != 0) {
    return string();
  }

  return res;
}

ULong f$base64url_decode_ulong(const string &s) {
  unsigned long long result;
  int r = base64url_decode(s.c_str(), reinterpret_cast <unsigned char *> (&result), 8);
  if (r != 8) {
    php_warning("Can't convert to ULong from base64url string \"%s\"", s.c_str());
    return 0;
  }
  return result;
}

string f$base64url_encode_ulong(ULong val) {
  string res(11, false);
  php_assert (base64url_encode(reinterpret_cast <const unsigned char *> (&val.l), 8, res.buffer(), 12) == 0);

  return res;
}

ULong f$base64url_decode_ulong_NN(const string &s) {
  unsigned char result[8];
  int r = base64url_decode(s.c_str(), result, 8);
  if (r != 8) {
    php_warning("Can't convert to ULong from base64url string \"%s\"", s.c_str());
    return 0;
  }

  return ((unsigned long long)result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3] |
         ((unsigned long long)result[4] << 56) | ((unsigned long long)result[5] << 48) | ((unsigned long long)result[6] << 40) | ((unsigned long long)result[7]
    << 32);
}

string f$base64url_encode_ulong_NN(ULong val) {
  unsigned long long l = val.l;
  unsigned char str[8];
  str[0] = static_cast<unsigned char>((l >> 24) & 255);
  str[1] = static_cast<unsigned char>((l >> 16) & 255);
  str[2] = static_cast<unsigned char>((l >> 8) & 255);
  str[3] = static_cast<unsigned char>(l & 255);
  str[4] = static_cast<unsigned char>(l >> 56);
  str[5] = static_cast<unsigned char>((l >> 48) & 255);
  str[6] = static_cast<unsigned char>((l >> 40) & 255);
  str[7] = static_cast<unsigned char>((l >> 32) & 255);

  string res(11, false);
  php_assert (base64url_encode(str, 8, res.buffer(), 12) == 0);

  return res;
}


static void parse_str_set_array_value(var &arr, const char *left_br_pos, int key_len, const string &value) {
  php_assert (*left_br_pos == '[');
  const char *right_br_pos = (const char *)memchr(left_br_pos + 1, ']', key_len - 1);
  if (right_br_pos != nullptr) {
    string next_key(left_br_pos + 1, static_cast<string::size_type>(right_br_pos - left_br_pos - 1));
    if (!arr.is_array()) {
      arr = array<var>();
    }

    if (next_key.empty()) {
      next_key = string(arr.to_array().get_next_key());
    }

    if (right_br_pos[1] == '[') {
      parse_str_set_array_value(arr[next_key], right_br_pos + 1, (int)(left_br_pos + key_len - (right_br_pos + 1)), value);
    } else {
      arr.set_value(next_key, value);
    }
  } else {
    arr = value;
  }
}

void parse_str_set_value(var &arr, const string &key, const string &value) {
  const char *key_c = key.c_str();
  const char *left_br_pos = (const char *)memchr(key_c, '[', key.size());
  if (left_br_pos != nullptr) {
    return parse_str_set_array_value(arr[string(key_c, static_cast<string::size_type>(left_br_pos - key_c))], left_br_pos, (int)(key_c + key.size() - left_br_pos), value);
  }

  arr.set_value(key, value);
}

void f$parse_str(const string &str, var &arr) {
  arr = array<var>();

  array<string> par = explode('&', str, INT_MAX);
  int64_t len = par.count();
  for (int64_t i = 0; i < len; i++) {
    const char *cur = par.get_value(i).c_str();
    const char *eq_pos = strchrnul(cur, '=');
    string value;
    if (*eq_pos) {
      value = f$urldecode(string(eq_pos + 1));
    }

    string key(cur, static_cast<string::size_type>(eq_pos - cur));
    key = f$urldecode(key);
    parse_str_set_value(arr, key, value);
  }
}

// returns associative array like [host=> path=>] or empty array if str is not a valid url
array<var> php_url_parse_ex(const char *str, string::size_type length) {
  array<var> result;
  const char *s = str, *e, *p, *pp, *ue = str + length;

  /* parse scheme */
  if ((e = static_cast<const char *>(memchr(s, ':', length))) && e != s) {
    /* validate scheme */
    p = s;
    while (p < e) {
      /* scheme = 1*[ lowalpha | digit | "+" | "-" | "." ] */
      if (!isalpha(*p) && !isdigit(*p) && *p != '+' && *p != '.' && *p != '-') {
        if (e + 1 < ue && e < s + strcspn(s, "?#")) {
          goto parse_port;
        } else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
          s += 2;
          e = nullptr;
          goto parse_host;
        } else {
          goto just_path;
        }
      }
      p++;
    }

    if (e + 1 == ue) { /* only scheme is available */
      result.set_value(string("scheme", 6), string(s, static_cast<string::size_type>(e - s)));
      return result;
    }

    /*
     * certain schemas like mailto: and zlib: may not have any / after them
     * this check ensures we support those.
     */
    if (*(e + 1) != '/') {
      /* check if the data we get is a port this allows us to
       * correctly parse things like a.com:80
       */
      p = e + 1;
      while (p < ue && isdigit(*p)) {
        p++;
      }

      if ((p == ue || *p == '/') && (p - e) < 7) {
        goto parse_port;
      }

      result.set_value(string("scheme", 6), string(s, static_cast<string::size_type>(e - s)));
      s = e + 1;
      goto just_path;
    } else {
      result.set_value(string("scheme", 6), string(s, static_cast<string::size_type>(e - s)));

      if (e + 2 < ue && *(e + 2) == '/') {
        s = e + 3;
        if (!strcmp(result.get_value(string("scheme", 6)).as_string().c_str(), "file")) {
          if (e + 3 < ue && *(e + 3) == '/') {
            /* support windows drive letters as in: file:///c:/somedir/file.txt */
            if (e + 5 < ue && *(e + 5) == ':') {
              s = e + 4;
            }
            goto just_path;
          }
        }
      } else {
        s = e + 1;
        goto just_path;
      }
    }
  } else if (e) { /* no scheme; starts with colon: look for port */
    parse_port:
    p = e + 1;
    pp = p;

    while (pp < ue && pp - p < 6 && isdigit(*pp)) {
      pp++;
    }

    if (pp - p > 0 && pp - p < 6 && (pp == ue || *pp == '/')) {
      int64_t port = string(p, static_cast<string::size_type>(pp - p)).to_int();
      if (port > 0 && port <= 65535) {
        result.set_value(string("port", 4), port);
        if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
          s += 2;
        }
      } else {
        return {};
      }
    } else if (p == pp && pp == ue) {
      return {};
    } else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
      s += 2;
    } else {
      goto just_path;
    }
  } else if (s + 1 < ue && *s == '/' && *(s + 1) == '/') { /* relative-scheme URL */
    s += 2;
  } else {
    goto just_path;
  }

  parse_host:
  /* Binary-safe strcspn(s, "/?#") */
  e = ue;
  if ((p = static_cast<const char *>(memchr(s, '/', e - s)))) {
    e = p;
  }
  if ((p = static_cast<const char *>(memchr(s, '?', e - s)))) {
    e = p;
  }
  if ((p = static_cast<const char *>(memchr(s, '#', e - s)))) {
    e = p;
  }

  /* check for login and password */
  if ((p = static_cast<const char *>(memrchr(s, '@', (e - s))))) {
    if ((pp = static_cast<const char *>(memchr(s, ':', (p - s))))) {
      result.set_value(string("user", 4), string(s, static_cast<string::size_type>(pp - s)));
      pp++;
      result.set_value(string("pass", 4), string(pp, static_cast<string::size_type>(p - pp)));
    } else {
      result.set_value(string("user", 4), string(s, static_cast<string::size_type>(p - s)));
    }

    s = p + 1;
  }

  /* check for port */
  if (s < ue && *s == '[' && *(e - 1) == ']') {
    /* Short circuit portscan, we're dealing with an IPv6 embedded address */
    p = nullptr;
  } else {
    p = static_cast<const char *>(memrchr(s, ':', (e - s)));
  }

  if (p) {
    if (!result.isset(string("port", 4))) {
      p++;
      if (e - p > 5) { /* port cannot be longer then 5 characters */
        return {};
      } else if (e - p > 0) {
        int64_t port = string(p, static_cast<string::size_type>(e - p)).to_int();
        if (port > 0 && port <= 65535) {
          result.set_value(string("port", 4), port);
        } else {
          return {};
        }
      }
      p--;
    }
  } else {
    p = e;
  }

  /* check if we have a valid host, if we don't reject the string as url */
  if ((p - s) < 1) {
    return {};
  }
  result.set_value(string("host", 4), string(s, static_cast<string::size_type>(p - s)));

  if (e == ue) {
    return result;
  }

  s = e;

  just_path:

  e = ue;
  p = static_cast<const char *>(memchr(s, '#', (e - s)));
  if (p) {
    p++;
    if (p < e) {
      result.set_value(string("fragment", 8), string(p, static_cast<string::size_type>(e - p)));
    }
    e = p - 1;
  }

  p = static_cast<const char *>(memchr(s, '?', (e - s)));
  if (p) {
    p++;
    if (p < e) {
      result.set_value(string("query", 5), string(p, static_cast<string::size_type>(e - p)));
    }
    e = p - 1;
  }

  if (s < e || s == ue) {
    result.set_value(string("path", 4), string(s, static_cast<string::size_type>(e - s)));
  }

  return result;
}

// returns var, as array|false|null (null if component doesn't exist)
var f$parse_url(const string &s, int64_t component) {
  array<var> url_as_array = php_url_parse_ex(s.c_str(), s.size());

  if (url_as_array.empty()) {
    return false;
  }

  if (component != -1) {
    switch (component) {
      case 0:             // PHP_URL_SCHEME
        return url_as_array.get_value(string("scheme", 6));
      case 1:             // PHP_URL_HOST
        return url_as_array.get_value(string("host", 4));
      case 2:             // PHP_URL_PORT
        return url_as_array.get_value(string("port", 4));
      case 3:             // PHP_URL_USER
        return url_as_array.get_value(string("user", 4));
      case 4:             // PHP_URL_PASS
        return url_as_array.get_value(string("pass", 4));
      case 5:             // PHP_URL_PATH
        return url_as_array.get_value(string("path", 4));
      case 6:             // PHP_URL_QUERY
        return url_as_array.get_value(string("query", 5));
      case 7:             // PHP_URL_FRAGMENT
        return url_as_array.get_value(string("fragment", 8));
      default:
        php_warning("Wrong parameter component = %ld in function parse_url", component);
        return false;
    }
  }

  return url_as_array;
}

string f$rawurldecode(const string &s) {
  static_SB.clean().reserve(s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (s[i] == '%') {
      int num_high = hex_to_int(s[i + 1]);
      if (num_high < 16) {
        int num_low = hex_to_int(s[i + 2]);
        if (num_low < 16) {
          static_SB.append_char(static_cast<char>((num_high << 4) + num_low));
          i += 2;
          continue;
        }
      }
    }
    static_SB.append_char(s[i]);
  }
  return static_SB.str();
}


static const char *good_url_symbols =
  "00000000000000000000000000000000"
  "00000000000001101111111111000000"
  "01111111111111111111111111100001"
  "01111111111111111111111111100000"
  "00000000000000000000000000000000"
  "00000000000000000000000000000000"
  "00000000000000000000000000000000"
  "00000000000000000000000000000000";//[0-9a-zA-Z-_.]

string f$rawurlencode(const string &s) {
  static_SB.clean().reserve(3 * s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (good_url_symbols[(unsigned char)s[i]] == '1') {
      static_SB.append_char(s[i]);
    } else {
      static_SB.append_char('%');
      static_SB.append_char(uhex_digits[(s[i] >> 4) & 15]);
      static_SB.append_char(uhex_digits[s[i] & 15]);
    }
  }
  return static_SB.str();
}

string f$urldecode(const string &s) {
  static_SB.clean().reserve(s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (s[i] == '%') {
      int num_high = hex_to_int(s[i + 1]);
      if (num_high < 16) {
        int num_low = hex_to_int(s[i + 2]);
        if (num_low < 16) {
          static_SB.append_char(static_cast<char>((num_high << 4) + num_low));
          i += 2;
          continue;
        }
      }
    } else if (s[i] == '+') {
      static_SB.append_char(' ');
      continue;
    }
    static_SB.append_char(s[i]);
  }
  return static_SB.str();
}

string f$urlencode(const string &s) {
  static_SB.clean().reserve(3 * s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (good_url_symbols[(unsigned char)s[i]] == '1') {
      static_SB.append_char(s[i]);
    } else if (s[i] == ' ') {
      static_SB.append_char('+');
    } else {
      static_SB.append_char('%');
      static_SB.append_char(uhex_digits[(s[i] >> 4) & 15]);
      static_SB.append_char(uhex_digits[s[i] & 15]);
    }
  }
  return static_SB.str();
}
