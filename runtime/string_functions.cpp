// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/string_functions.h"

#include <clocale>
#include <sys/types.h>
#include <cctype>

#include "common/macos-ports.h"
#include "common/unicode/unicode-utils.h"

#include "runtime/interface.h"

const string COLON(",", 1);
const string CP1251("cp1251");
const string DOT(".", 1);
const string NEW_LINE("\n", 1);
const string SPACE(" ", 1);
const string WHAT(" \n\r\t\v\0", 6);

static const string ONE("1", 1);
static const string PERCENT("%", 1);

char php_buf[PHP_BUF_LEN + 1];

const char lhex_digits[17] = "0123456789abcdef";
const char uhex_digits[17] = "0123456789ABCDEF";

int64_t str_replace_count_dummy;

static inline const char *get_mask(const string &what) {
  static char mask[256];
  memset(mask, 0, 256);

  int len = what.size();
  for (int i = 0; i < len; i++) {
    unsigned char c = what[i];
    if (what[i + 1] == '.' && what[i + 2] == '.' && (unsigned char)what[i + 3] >= c) {
      memset(mask + c, 1, (unsigned char)what[i + 3] - c + 1);
      i += 3;
    } else if (c == '.' && what[i + 1] == '.') {
      php_warning("Invalid '..'-range in string \"%s\" at position %d.", what.c_str(), i);
    } else {
      mask[c] = 1;
    }
  }

  return mask;
}

string f$addcslashes(const string &str, const string &what) {
  const char *mask = get_mask(what);

  int len = str.size();
  static_SB.clean().reserve(4 * len);

  for (int i = 0; i < len; i++) {
    unsigned char c = str[i];
    if (mask[c]) {
      static_SB.append_char('\\');
      if (c < 32 || c > 126) {
        switch (c) {
          case '\n':
            static_SB.append_char('n');
            break;
          case '\t':
            static_SB.append_char('t');
            break;
          case '\r':
            static_SB.append_char('r');
            break;
          case '\a':
            static_SB.append_char('a');
            break;
          case '\v':
            static_SB.append_char('v');
            break;
          case '\b':
            static_SB.append_char('b');
            break;
          case '\f':
            static_SB.append_char('f');
            break;
          default:
            static_SB.append_char(static_cast<char>((c >> 6) + '0'));
            static_SB.append_char(static_cast<char>(((c >> 3) & 7) + '0'));
            static_SB.append_char(static_cast<char>((c & 7) + '0'));
        }
      } else {
        static_SB.append_char(c);
      }
    } else {
      static_SB.append_char(c);
    }
  }
  return static_SB.str();
}

string f$addslashes(const string &str) {
  int len = str.size();

  static_SB.clean().reserve(2 * len);
  for (int i = 0; i < len; i++) {
    switch (str[i]) {
      case '\0':
        static_SB.append_char('\\');
        static_SB.append_char('0');
        break;
      case '\'':
      case '\"':
      case '\\':
        static_SB.append_char('\\');
        /* fallthrough */
      default:
        static_SB.append_char(str[i]);
    }
  }
  return static_SB.str();
}

string f$bin2hex(const string &str) {
  int len = str.size();
  string result(2 * len, false);

  for (int i = 0; i < len; i++) {
    result[2 * i] = lhex_digits[(str[i] >> 4) & 15];
    result[2 * i + 1] = lhex_digits[str[i] & 15];
  }

  return result;
}

string f$chop(const string &s, const string &what) {
  return f$rtrim(s, what);
}

string f$chr(int64_t v) {
  return {1, static_cast<char>(v)};
}

static const unsigned char win_to_koi[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
  46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
  46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
  154, 174, 190, 46, 159, 189, 46, 46, 179, 191, 180, 157, 46, 46, 156, 183,
  46, 46, 182, 166, 173, 46, 46, 158, 163, 152, 164, 155, 46, 46, 46, 167,
  225, 226, 247, 231, 228, 229, 246, 250, 233, 234, 235, 236, 237, 238, 239, 240,
  242, 243, 244, 245, 230, 232, 227, 254, 251, 253, 255, 249, 248, 252, 224, 241,
  193, 194, 215, 199, 196, 197, 214, 218, 201, 202, 203, 204, 205, 206, 207, 208,
  210, 211, 212, 213, 198, 200, 195, 222, 219, 221, 223, 217, 216, 220, 192, 209};

static const unsigned char koi_to_win[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
  32, 32, 32, 184, 186, 32, 179, 191, 32, 32, 32, 32, 32, 180, 162, 32,
  32, 32, 32, 168, 170, 32, 178, 175, 32, 32, 32, 32, 32, 165, 161, 169,
  254, 224, 225, 246, 228, 229, 244, 227, 245, 232, 233, 234, 235, 236, 237, 238,
  239, 255, 240, 241, 242, 243, 230, 226, 252, 251, 231, 248, 253, 249, 247, 250,
  222, 192, 193, 214, 196, 197, 212, 195, 213, 200, 201, 202, 203, 204, 205, 206,
  207, 223, 208, 209, 210, 211, 198, 194, 220, 219, 199, 216, 221, 217, 215, 218};

string f$convert_cyr_string(const string &str, const string &from_s, const string &to_s) {
  char from = (char)toupper(from_s[0]);
  char to = (char)toupper(to_s[0]);

  const unsigned char *table = nullptr;
  if (from == 'W' && to == 'K') {
    table = win_to_koi;
  }
  if (from == 'K' && to == 'W') {
    table = koi_to_win;
  }
  if (table == nullptr) {
    php_critical_error ("unsupported conversion from '%c' to '%c' in function convert_cyr_string", from, to);
    return str;
  }

  int len = str.size();
  string result(len, false);
  for (int i = 0; i < len; i++) {
    result[i] = table[(unsigned char)str[i]];
  }
  return result;
}

mixed f$count_chars(const string &str, int64_t mode) {
  int64_t chars[256] = {0};

  if (static_cast<uint32_t>(mode) > 4u) {
    php_warning("Unknown mode %" PRIi64, mode);
    return false;
  }

  const string::size_type len = str.size();
  for (string::size_type i = 0; i < len; i++) {
    chars[static_cast<unsigned char>(str[i])]++;
  }

  if (mode <= 2) {
    array<mixed> result;
    for (int64_t i = 0; i < 256; i++) {
      if ((mode != 2 && chars[i] != 0) ||
          (mode != 1 && chars[i] == 0)) {
        result.set_value(i, chars[i]);
      }
    }
    return result;
  }

  string result;
  for (int i = 0; i < 256; i++) {
    if ((mode == 3) == (chars[i] != 0)) {
      result.push_back(char(i));
    }
  }
  return result;
}

string f$hex2bin(const string &str) {
  int len = str.size();
  if (len & 1) {
    php_warning("Wrong argument \"%s\" supplied for function hex2bin", str.c_str());
    return {};
  }

  string result(len / 2, false);
  for (int i = 0; i < len; i += 2) {
    int num_high = hex_to_int(str[i]);
    int num_low = hex_to_int(str[i + 1]);
    if (num_high == 16 || num_low == 16) {
      php_warning("Wrong argument \"%s\" supplied for function hex2bin", str.c_str());
      return {};
    }
    result[i / 2] = (char)((num_high << 4) + num_low);
  }

  return result;
}

static const int entities_size = 251;

static const char *ent_to_num_s[entities_size] = {
  "AElig", "Aacute", "Acirc", "Agrave", "Alpha", "Aring", "Atilde", "Auml", "Beta", "Ccedil",
  "Chi", "Dagger", "Delta", "ETH", "Eacute", "Ecirc", "Egrave", "Epsilon", "Eta", "Euml",
  "Gamma", "Iacute", "Icirc", "Igrave", "Iota", "Iuml", "Kappa", "Lambda", "Mu", "Ntilde",
  "Nu", "OElig", "Oacute", "Ocirc", "Ograve", "Omega", "Omicron", "Oslash", "Otilde", "Ouml",
  "Phi", "Pi", "Prime", "Psi", "Rho", "Scaron", "Sigma", "THORN", "Tau", "Theta",
  "Uacute", "Ucirc", "Ugrave", "Upsilon", "Uuml", "Xi", "Yacute", "Yuml", "Zeta", "aacute",
  "acirc", "acute", "aelig", "agrave", "alefsym", "alpha", "amp", "and", "ang", "aring",
  "asymp", "atilde", "auml", "bdquo", "beta", "brvbar", "bull", "cap", "ccedil", "cedil",
  "cent", "chi", "circ", "clubs", "cong", "copy", "crarr", "cup", "curren", "dArr",
  "dagger", "darr", "deg", "delta", "diams", "divide", "eacute", "ecirc", "egrave", "empty",
  "emsp", "ensp", "epsilon", "equiv", "eta", "eth", "euml", "euro", "exist", "fnof",
  "forall", "frac12", "frac14", "frac34", "frasl", "gamma", "ge", "gt", "hArr", "harr",
  "hearts", "hellip", "iacute", "icirc", "iexcl", "igrave", "image", "infin", "int", "iota",
  "iquest", "isin", "iuml", "kappa", "lArr", "lambda", "lang", "laquo", "larr", "lceil",
  "ldquo", "le", "lfloor", "lowast", "loz", "lrm", "lsaquo", "lsquo", "lt", "macr",
  "mdash", "micro", "middot", "minus", "mu", "nabla", "nbsp", "ndash", "ne", "ni",
  "not", "notin", "nsub", "ntilde", "nu", "oacute", "ocirc", "oelig", "ograve", "oline",
  "omega", "omicron", "oplus", "or", "ordf", "ordm", "oslash", "otilde", "otimes", "ouml",
  "para", "part", "permil", "perp", "phi", "pi", "piv", "plusmn", "pound", "prime",
  "prod", "prop", "psi", "rArr", "radic", "rang", "raquo", "rarr", "rceil",
  "rdquo", "real", "reg", "rfloor", "rho", "rlm", "rsaquo", "rsquo", "sbquo", "scaron",
  "sdot", "sect", "shy", "sigma", "sigmaf", "sim", "spades", "sub", "sube", "sum",
  "sup", "sup1", "sup2", "sup3", "supe", "szlig", "tau", "there4", "theta", "thetasym",
  "thinsp", "thorn", "tilde", "times", "trade", "uArr", "uacute", "uarr", "ucirc", "ugrave",
  "uml", "upsih", "upsilon", "uuml", "weierp", "xi", "yacute", "yen", "yuml", "zeta",
  "zwj", "zwnj"};

static int ent_to_num_i[entities_size] = {
  198, 193, 194, 192, 913, 197, 195, 196, 914, 199, 935, 8225, 916, 208, 201, 202, 200, 917, 919, 203,
  915, 205, 206, 204, 921, 207, 922, 923, 924, 209, 925, 338, 211, 212, 210, 937, 927, 216, 213, 214,
  934, 928, 8243, 936, 929, 352, 931, 222, 932, 920, 218, 219, 217, 933, 220, 926, 221, 376, 918, 225,
  226, 180, 230, 224, 8501, 945, 38, 8743, 8736, 229, 8776, 227, 228, 8222, 946, 166, 8226, 8745, 231, 184,
  162, 967, 710, 9827, 8773, 169, 8629, 8746, 164, 8659, 8224, 8595, 176, 948, 9830, 247, 233, 234, 232, 8709,
  8195, 8194, 949, 8801, 951, 240, 235, 8364, 8707, 402, 8704, 189, 188, 190, 8260, 947, 8805, 62, 8660, 8596,
  9829, 8230, 237, 238, 161, 236, 8465, 8734, 8747, 953, 191, 8712, 239, 954, 8656, 955, 9001, 171, 8592, 8968,
  8220, 8804, 8970, 8727, 9674, 8206, 8249, 8216, 60, 175, 8212, 181, 183, 8722, 956, 8711, 160, 8211, 8800, 8715,
  172, 8713, 8836, 241, 957, 243, 244, 339, 242, 8254, 969, 959, 8853, 8744, 170, 186, 248, 245, 8855, 246,
  182, 8706, 8240, 8869, 966, 960, 982, 177, 163, 8242, 8719, 8733, 968, 8658, 8730, 9002, 187, 8594, 8969,
  8221, 8476, 174, 8971, 961, 8207, 8250, 8217, 8218, 353, 8901, 167, 173, 963, 962, 8764, 9824, 8834, 8838, 8721,
  8835, 185, 178, 179, 8839, 223, 964, 8756, 952, 977, 8201, 254, 732, 215, 8482, 8657, 250, 8593, 251, 249,
  168, 978, 965, 252, 8472, 958, 253, 165, 255, 950, 8205, 8204};
/*
static int cp1251_to_utf8[128] = {
  0x402, 0x403,  0x201A, 0x453,  0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x409,  0x2039, 0x40A, 0x40C, 0x40B, 0x40F,
  0x452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x0,    0x2122, 0x459,  0x203A, 0x45A, 0x45C, 0x45B, 0x45F,
  0xA0,  0x40E,  0x45E,  0x408,  0xA4,   0x490,  0xA6,   0xA7,   0x401,  0xA9,   0x404,  0xAB,   0xAC,  0xAD,  0xAE,  0x407,
  0xB0,  0xB1,   0x406,  0x456,  0x491,  0xB5,   0xB6,   0xB7,   0x451,  0x2116, 0x454,  0xBB,   0x458, 0x405, 0x455, 0x457,
  0x410, 0x411,  0x412,  0x413,  0x414,  0x415,  0x416,  0x417,  0x418,  0x419,  0x41A,  0x41B,  0x41C, 0x41D, 0x41E, 0x41F,
  0x420, 0x421,  0x422,  0x423,  0x424,  0x425,  0x426,  0x427,  0x428,  0x429,  0x42A,  0x42B,  0x42C, 0x42D, 0x42E, 0x42F,
  0x430, 0x431,  0x432,  0x433,  0x434,  0x435,  0x436,  0x437,  0x438,  0x439,  0x43A,  0x43B,  0x43C, 0x43D, 0x43E, 0x43F,
  0x440, 0x441,  0x442,  0x443,  0x444,  0x445,  0x446,  0x447,  0x448,  0x449,  0x44A,  0x44B,  0x44C, 0x44D, 0x44E, 0x44F};
*/
static const char *cp1251_to_utf8_str[128] = {
  "&#1026;", "&#1027;", "&#8218;", "&#1107;", "&#8222;", "&hellip;", "&dagger;", "&Dagger;", "&euro;", "&permil;", "&#1033;", "&#8249;", "&#1034;", "&#1036;",
  "&#1035;", "&#1039;",
  "&#1106;", "&#8216;", "&#8217;", "&#8219;", "&#8220;", "&bull;", "&ndash;", "&mdash;", "", "&trade;", "&#1113;", "&#8250;", "&#1114;", "&#1116;", "&#1115;",
  "&#1119;",
  "&nbsp;", "&#1038;", "&#1118;", "&#1032;", "&curren;", "&#1168;", "&brvbar;", "&sect;", "&#1025;", "&copy;", "&#1028;", "&laquo;", "&not;", "&shy;", "&reg;",
  "&#1031;",
  "&deg;", "&plusmn;", "&#1030;", "&#1110;", "&#1169;", "&micro;", "&para;", "&middot;", "&#1105;", "&#8470;", "&#1108;", "&raquo;", "&#1112;", "&#1029;",
  "&#1109;", "&#1111;",
  "&#1040;", "&#1041;", "&#1042;", "&#1043;", "&#1044;", "&#1045;", "&#1046;", "&#1047;", "&#1048;", "&#1049;", "&#1050;", "&#1051;", "&#1052;", "&#1053;",
  "&#1054;", "&#1055;",
  "&#1056;", "&#1057;", "&#1058;", "&#1059;", "&#1060;", "&#1061;", "&#1062;", "&#1063;", "&#1064;", "&#1065;", "&#1066;", "&#1067;", "&#1068;", "&#1069;",
  "&#1070;", "&#1071;",
  "&#1072;", "&#1073;", "&#1074;", "&#1075;", "&#1076;", "&#1077;", "&#1078;", "&#1079;", "&#1080;", "&#1081;", "&#1082;", "&#1083;", "&#1084;", "&#1085;",
  "&#1086;", "&#1087;",
  "&#1088;", "&#1089;", "&#1090;", "&#1091;", "&#1092;", "&#1093;", "&#1094;", "&#1095;", "&#1096;", "&#1097;", "&#1098;", "&#1099;", "&#1100;", "&#1101;",
  "&#1102;", "&#1103;"};

string f$htmlentities(const string &str) {
  int len = (int)str.size();
  static_SB.clean().reserve(8 * len);

  for (int i = 0; i < len; i++) {
    switch (str[i]) {
      case '&':
        static_SB.append_char('&');
        static_SB.append_char('a');
        static_SB.append_char('m');
        static_SB.append_char('p');
        static_SB.append_char(';');
        break;
      case '"':
        static_SB.append_char('&');
        static_SB.append_char('q');
        static_SB.append_char('u');
        static_SB.append_char('o');
        static_SB.append_char('t');
        static_SB.append_char(';');
        break;
      case '<':
        static_SB.append_char('&');
        static_SB.append_char('l');
        static_SB.append_char('t');
        static_SB.append_char(';');
        break;
      case '>':
        static_SB.append_char('&');
        static_SB.append_char('g');
        static_SB.append_char('t');
        static_SB.append_char(';');
        break;
      default:
        if (str[i] < 0) {
          const char *utf8_str = cp1251_to_utf8_str[128 + str[i]];
          static_SB.append_unsafe(utf8_str, static_cast<int>(strlen(utf8_str)));
        } else {
          static_SB.append_char(str[i]);
        }
    }
  }

  return static_SB.str();
}

string f$html_entity_decode(const string &str, int64_t flags, const string &encoding) {
  if (flags >= 3) {
    php_critical_error ("unsupported parameter flags = %" PRIi64 " in function html_entity_decode", flags);
  }

  bool utf8 = memchr(encoding.c_str(), '8', encoding.size()) != nullptr;
  if (!utf8 && strstr(encoding.c_str(), "1251") == nullptr) {
    php_critical_error ("unsupported encoding \"%s\" in function html_entity_decode", encoding.c_str());
    return str;
  }

  int len = str.size();
  string res(len * 7 / 4 + 4, false);
  char *p = &res[0];
  for (int i = 0; i < len; i++) {
    if (str[i] == '&') {
      int j = i + 1;
      while (j < len && str[j] != ';') {
        j++;
      }
      if (j < len) {
        if ((flags & ENT_QUOTES) && j == i + 5) {
          if (str[i + 1] == '#' && str[i + 2] == '0' && str[i + 3] == '3' && str[i + 4] == '9') {
            i += 5;
            *p++ = '\'';
            continue;
          }
        }
        if (!(flags & ENT_NOQUOTES) && j == i + 5) {
          if (str[i + 1] == 'q' && str[i + 2] == 'u' && str[i + 3] == 'o' && str[i + 4] == 't') {
            i += 5;
            *p++ = '\"';
            continue;
          }
        }

        int l = 0, r = entities_size;
        while (l + 1 < r) {
          int m = (l + r) >> 1;
          if (strncmp(str.c_str() + i + 1, ent_to_num_s[m], j - i - 1) < 0) {
            r = m;
          } else {
            l = m;
          }
        }
        if (strncmp(str.c_str() + i + 1, ent_to_num_s[l], j - i - 1) == 0) {
          int num = ent_to_num_i[l];
          i = j;
          if (utf8) {
            if (num < 128) {
              *p++ = (char)num;
            } else if (num < 0x800) {
              *p++ = (char)(0xc0 + (num >> 6));
              *p++ = (char)(0x80 + (num & 63));
            } else {
              *p++ = (char)(0xe0 + (num >> 12));
              *p++ = (char)(0x80 + ((num >> 6) & 63));
              *p++ = (char)(0x80 + (num & 63));
            }
          } else {
            if (num < 128) {
              *p++ = (char)num;
            } else {
              *p++ = '&';
              *p++ = '#';
              if (num >= 1000) {
                *p++ = (char)(num / 1000 % 10 + '0');
              }
              *p++ = (char)(num / 100 % 10 + '0');
              *p++ = (char)(num / 10 % 10 + '0');
              *p++ = (char)(num % 10 + '0');
              *p++ = ';';
            }
          }
          continue;
        }
      }
    }

    *p++ = str[i];
  }
  res.shrink(static_cast<string::size_type>(p - res.c_str()));

  return res;
}

string f$htmlspecialchars(const string &str, int64_t flags) {
  if (flags >= 3) {
    php_critical_error ("unsupported parameter flags = %" PRIi64 " in function htmlspecialchars", flags);
  }

  const string::size_type len = str.size();
  static_SB.clean().reserve(6 * len);

  for (string::size_type i = 0; i < len; i++) {
    switch (str[i]) {
      case '&':
        static_SB.append_char('&');
        static_SB.append_char('a');
        static_SB.append_char('m');
        static_SB.append_char('p');
        static_SB.append_char(';');
        break;
      case '"':
        if (!(flags & ENT_NOQUOTES)) {
          static_SB.append_char('&');
          static_SB.append_char('q');
          static_SB.append_char('u');
          static_SB.append_char('o');
          static_SB.append_char('t');
          static_SB.append_char(';');
        } else {
          static_SB.append_char('"');
        }
        break;
      case '\'':
        if (flags & ENT_QUOTES) {
          static_SB.append_char('&');
          static_SB.append_char('#');
          static_SB.append_char('0');
          static_SB.append_char('3');
          static_SB.append_char('9');
          static_SB.append_char(';');
        } else {
          static_SB.append_char('\'');
        }
        break;
      case '<':
        static_SB.append_char('&');
        static_SB.append_char('l');
        static_SB.append_char('t');
        static_SB.append_char(';');
        break;
      case '>':
        static_SB.append_char('&');
        static_SB.append_char('g');
        static_SB.append_char('t');
        static_SB.append_char(';');
        break;
      default:
        static_SB.append_char(str[i]);
    }
  }

  return static_SB.str();
}

string f$htmlspecialchars_decode(const string &str, int64_t flags) {
  if (flags >= 3) {
    php_critical_error ("unsupported parameter flags = %" PRIi64 " in function htmlspecialchars_decode", flags);
  }

  int len = str.size();
  string res(len, false);
  char *p = &res[0];
  for (int i = 0; i < len;) {
    if (str[i] == '&') {
      if (str[i + 1] == 'a' && str[i + 2] == 'm' && str[i + 3] == 'p' && str[i + 4] == ';') {
        *p++ = '&';
        i += 5;
      } else if (str[i + 1] == 'q' && str[i + 2] == 'u' && str[i + 3] == 'o' && str[i + 4] == 't' && str[i + 5] == ';' && !(flags & ENT_NOQUOTES)) {
        *p++ = '"';
        i += 6;
      } else if (str[i + 1] == '#' && str[i + 2] == '0' && str[i + 3] == '3' && str[i + 4] == '9' && str[i + 5] == ';' && (flags & ENT_QUOTES)) {
        *p++ = '\'';
        i += 6;
      } else if (str[i + 1] == 'l' && str[i + 2] == 't' && str[i + 3] == ';') {
        *p++ = '<';
        i += 4;
      } else if (str[i + 1] == 'g' && str[i + 2] == 't' && str[i + 3] == ';') {
        *p++ = '>';
        i += 4;
      } else {
        *p++ = '&';
        i++;
      }
    } else {
      *p++ = str[i];
      i++;
    }
  }
  res.shrink(static_cast<string::size_type>(p - res.c_str()));

  return res;
}

string f$lcfirst(const string &str) {
  int n = str.size();
  if (n == 0) {
    return str;
  }

  string res(n, false);
  res[0] = (char)tolower(str[0]);
  memcpy(&res[1], &str[1], n - 1);

  return res;
}

int64_t f$levenshtein(const string &str1, const string &str2) {
  string::size_type len1 = str1.size();
  string::size_type len2 = str2.size();

  const string::size_type MAX_LEN = 16384;
  if (len1 > MAX_LEN || len2 > MAX_LEN) {
    php_warning("Too long strings of length %u and %u supplied for function levenshtein. Maximum allowed length is %u.", len1, len2, MAX_LEN);
    if (len1 > MAX_LEN) {
      len1 = MAX_LEN;
    }
    if (len2 > MAX_LEN) {
      len2 = MAX_LEN;
    }
  }

  int64_t dp[2][MAX_LEN + 1];

  for (string::size_type j = 0; j <= len2; j++) {
    dp[0][j] = j;
  }

  for (string::size_type i = 1; i <= len1; i++) {
    dp[i & 1][0] = i;
    for (string::size_type j = 1; j <= len2; j++) {
      if (str1[i - 1] == str2[j - 1]) {
        dp[i & 1][j] = dp[(i - 1) & 1][j - 1];
      } else {
        int64_t res = dp[(i - 1) & 1][j - 1];
        if (dp[(i - 1) & 1][j] < res) {
          res = dp[(i - 1) & 1][j];
        }
        if (dp[i & 1][j - 1] < res) {
          res = dp[i & 1][j - 1];
        }
        dp[i & 1][j] = res + 1;
      }
    }
  }
  return dp[len1 & 1][len2];
}

string f$ltrim(const string &s, const string &what) {
  const char *mask = get_mask(what);

  int len = (int)s.size();
  if (len == 0 || !mask[(unsigned char)s[0]]) {
    return s;
  }

  int l = 1;
  while (l < len && mask[(unsigned char)s[l]]) {
    l++;
  }
  return {s.c_str() + l, static_cast<string::size_type>(len - l)};
}

string f$mysql_escape_string(const string &str) {
  int len = str.size();
  static_SB.clean().reserve(2 * len);
  for (int i = 0; i < len; i++) {
    switch (str[i]) {
      case '\0':
      case '\n':
      case '\r':
      case 26:
      case '\'':
      case '\"':
      case '\\':
        static_SB.append_char('\\');
        /* fallthrough */
      default:
        static_SB.append_char(str[i]);
    }
  }
  return static_SB.str();
}

string f$nl2br(const string &str, bool is_xhtml) {
  const char *br = is_xhtml ? "<br />" : "<br>";
  int br_len = (int)strlen(br);

  int len = str.size();

  static_SB.clean().reserve((br_len + 1) * len);

  for (int i = 0; i < len;) {
    if (str[i] == '\n' || str[i] == '\r') {
      static_SB.append_unsafe(br, br_len);
      if (str[i] + str[i + 1] == '\n' + '\r') {
        static_SB.append_char(str[i++]);
      }
    }
    static_SB.append_char(str[i++]);
  }

  return static_SB.str();
}

string f$number_format(double number, int64_t decimals, const string &dec_point, const string &thousands_sep) {
  char *result_begin = php_buf + PHP_BUF_LEN;

  if (decimals < 0 || decimals > 100) {
    php_warning("Wrong parameter decimals (%" PRIi64 ") in function number_format", decimals);
    return {};
  }
  bool negative = false;
  if (number < 0) {
    negative = true;
    number *= -1;
  }

  double frac = number - floor(number);
  number -= frac;

  double mul = pow(10.0, (double)decimals);
  frac = round(frac * mul + 1e-9);

  int64_t old_decimals = decimals;
  while (result_begin > php_buf && decimals--) {
    double x = floor(frac * 0.1 + 0.05);
    auto y = static_cast<int32_t>(frac - x * 10 + 0.05);
    if ((unsigned int)y >= 10u) {
      y = 0;
    }
    frac = x;

    *--result_begin = (char)(y + '0');
  }
  number += frac;

  if (old_decimals > 0) {
    string::size_type i = dec_point.size();
    while (result_begin > php_buf && i > 0) {
      *--result_begin = dec_point[--i];
    }
  }

  int64_t digits = 0;
  do {
    if (digits && digits % 3 == 0) {
      string::size_type i = thousands_sep.size();
      while (result_begin > php_buf && i > 0) {
        *--result_begin = thousands_sep[--i];
      }
    }
    digits++;

    if (result_begin > php_buf) {
      double x = floor(number * 0.1 + 0.05);
      auto y = static_cast<int32_t>((number - x * 10 + 0.05));
      if ((unsigned int)y >= 10u) {
        y = 0;
      }
      number = x;

      *--result_begin = (char)(y + '0');
    }
  } while (result_begin > php_buf && number > 0.5);

  if (result_begin > php_buf && negative) {
    *--result_begin = '-';
  }

  if (result_begin <= php_buf) {
    php_critical_error ("maximum length of result (%d) exceeded", PHP_BUF_LEN);
    return {};
  }

  return {result_begin, static_cast<string::size_type>(php_buf + PHP_BUF_LEN - result_begin)};
}

int64_t f$ord(const string &s) {
  return (unsigned char)s[0];
}

static uint64_t float64_bits(double f) {
  uint64_t bits = 0;
  std::memcpy(&bits, &f, sizeof(uint64_t));
  return bits;
}

static double float64_from_bits(uint64_t bits) {
  double f = 0;
  std::memcpy(&f, &bits, sizeof(uint64_t));
  return f;
}

string f$pack(const string &pattern, const array<mixed> &a) {
  static_SB.clean();
  int cur_arg = 0;
  for (int i = 0; i < (int)pattern.size();) {
    if (pattern[i] == '*') {
      if (i > 0) {
        --i;
      }
    }
    char format = pattern[i++];
    int cnt = 1;
    if ('0' <= pattern[i] && pattern[i] <= '9') {
      cnt = 0;
      do {
        cnt = cnt * 10 + pattern[i++] - '0';
      } while ('0' <= pattern[i] && pattern[i] <= '9');

      if (cnt <= 0) {
        php_warning("Wrong count specifier in pattern \"%s\"", pattern.c_str());
        return {};
      }
    } else if (pattern[i] == '*') {
      cnt = 0;
    }

    int arg_num = cur_arg;
    if (arg_num >= a.count()) {
      if (format == 'A' || format == 'a' || format == 'H' || format == 'h' || cnt != 0) {
        php_warning("Not enough parameters to call function pack");
        return {};
      }
      if (i + 1 != (int)pattern.size()) {
        php_warning("Misplaced symbol '*' in pattern \"%s\"", pattern.c_str());
        return {};
      }
      break;
    }
    cur_arg++;

    mixed arg = a.get_value(arg_num);

    if (arg.is_array()) {
      php_warning("Argument %d of function pack is array", arg_num);
      return {};
    }

    char filler = 0;
    switch (format) {
      case 'A':
        filler = ' ';
        /* fallthrough */
      case 'a': {
        string arg_str = arg.to_string();
        int len = arg_str.size();
        if (!cnt) {
          cnt = len;
          i++;
        }
        static_SB.append(arg_str.c_str(), static_cast<size_t>(min(cnt, len)));
        while (cnt > len) {
          static_SB << filler;
          cnt--;
        }
        break;
      }
      case 'h':
      case 'H': {
        string arg_str = arg.to_string();
        int len = arg_str.size();
        if (!cnt) {
          cnt = len;
          i++;
        }
        for (int j = 0; cnt > 0 && j < len; j += 2) {
          int num_high = hex_to_int(arg_str[j]);
          int num_low = cnt > 1 ? hex_to_int(arg_str[j + 1]) : 0;
          cnt -= 2;
          if (num_high == 16 || num_low == 16) {
            php_warning("Wrong argument \"%s\" supplied for format '%c' in function pack", arg_str.c_str(), format);
            return {};
          }
          if (format == 'H') {
            static_SB << (char)((num_high << 4) + num_low);
          } else {
            static_SB << (char)((num_low << 4) + num_high);
          }
        }
        if (cnt > 0) {
          php_warning("Type %c: not enough characters in string \"%s\" in function pack", format, arg_str.c_str());
        }
        break;
      }

      default:
        do {
          switch (format) {
            case 'c':
            case 'C':
              static_SB << (char)(arg.to_int());
              break;
            case 's':
            case 'S':
            case 'v': {
              unsigned short value = (short)arg.to_int();
              static_SB.append((const char *)&value, 2);
              break;
            }
            case 'n': {
              unsigned short value = (short)arg.to_int();
              static_SB
                << (char)(value >> 8)
                << (char)(value & 255);
              break;
            }
            case 'i':
            case 'I':
            case 'l':
            case 'L':
            case 'V': {
              auto value = static_cast<int32_t>(arg.to_int());
              static_SB.append((const char *)&value, 4);
              break;
            }
            case 'N': {
              auto value = static_cast<uint32_t>(arg.to_int());
              static_SB
                << (char)(value >> 24)
                << (char)((value >> 16) & 255)
                << (char)((value >> 8) & 255)
                << (char)(value & 255);
              break;
            }
            case 'f': {
              float value = (float)arg.to_float();
              static_SB.append((const char *)&value, sizeof(float));
              break;
            }
            case 'e':
            case 'E':
            case 'd': {
              double value = arg.to_float();
              uint64_t value_byteordered = float64_bits(value);
              if (format == 'e') {
                value_byteordered = htole64(value_byteordered);
              } else if (format == 'E') {
                value_byteordered = htobe64(value_byteordered);
              }
              static_SB.append((const char *)&value_byteordered, sizeof(uint64_t));
              break;
            }
            case 'J':
            case 'P':
            case 'Q': {
              // stored in the host machine order by the default (Q flag)
              unsigned long long value_byteordered = static_cast<unsigned long long>(arg.to_string().to_int());
              if (format == 'P') {
                // for P encode in little endian order
                value_byteordered = htole64(value_byteordered);
              } else if (format == 'J') {
                // for J encode in big endian order
                value_byteordered = htobe64(value_byteordered);
              }

              static_SB.append((const char *)&value_byteordered, sizeof(unsigned long long));
              break;
            }
            case 'q': {
              int64_t value = arg.to_string().to_int();
              static_SB.append((const char *)&value, sizeof(long long));
              break;
            }
            default:
              php_warning("Format code \"%c\" not supported", format);
              return {};
          }

          if (cnt > 1) {
            arg_num = cur_arg++;
            if (arg_num >= a.count()) {
              php_warning("Not enough parameters to call function pack");
              return {};
            }

            arg = a.get_value(arg_num);

            if (arg.is_array()) {
              php_warning("Argument %d of function pack is array", arg_num);
              return {};
            }
          }
        } while (--cnt > 0);
    }
  }

  php_assert (cur_arg <= a.count());
  if (cur_arg < a.count()) {
    php_warning("Too much arguments to call pack with format \"%s\"", pattern.c_str());
  }

  return static_SB.str();
}

string f$prepare_search_query(const string &query) {
  const char *s = clean_str(query.c_str());
  if (s == nullptr) {
    s = "";
  }
  return string(s);
}

int64_t f$printf(const string &format, const array<mixed> &a) {
  string to_print = f$sprintf(format, a);
  print(to_print);
  return to_print.size();
}

string f$rtrim(const string &s, const string &what) {
  const char *mask = get_mask(what);

  int len = (int)s.size() - 1;
  if (len == -1 || !mask[(unsigned char)s[len]]) {
    return s;
  }

  while (len > 0 && mask[(unsigned char)s[len - 1]]) {
    len--;
  }

  return {s.c_str(), static_cast<string::size_type>(len)};
}

Optional<string> f$setlocale(int64_t category, const string &locale) {
  const char *loc = locale.c_str();
  if (locale[0] == '0' && locale.size() == 1) {
    loc = nullptr;
  }
  char *res = setlocale(static_cast<int32_t>(category), loc);
  if (res == nullptr) {
    return false;
  }
  return string(res);
}

string f$sprintf(const string &format, const array<mixed> &a) {
  string result;
  result.reserve_at_least(format.size());
  int cur_arg = 0;
  bool error_too_big = false;
  for (int i = 0; i < (int)format.size(); i++) {
    if (format[i] != '%') {
      result.push_back(format[i]);
      continue;
    }
    i++;

    int parsed_arg_num = 0, j;
    for (j = i; '0' <= format[j] && format[j] <= '9'; j++) {
      parsed_arg_num = parsed_arg_num * 10 + format[j] - '0';
    }
    int arg_num = -2;
    if (format[j] == '$') {
      i = j + 1;
      arg_num = parsed_arg_num - 1;
    }

    char sign = 0;
    if (format[i] == '+') {
      sign = format[i++];
    }

    char filler = ' ';
    if (format[i] == '0' || format[i] == ' ') {
      filler = format[i++];
    } else if (format[i] == '\'') {
      i++;
      filler = format[i++];
    }

    int pad_right = false;
    if (format[i] == '-') {
      pad_right = true;
      i++;
    }

    int width = 0;
    while ('0' <= format[i] && format[i] <= '9' && width < PHP_BUF_LEN) {
      width = width * 10 + format[i++] - '0';
    }

    if (width >= PHP_BUF_LEN) {
      error_too_big = true;
      break;
    }

    int precision = -1;
    if (format[i] == '.' && '0' <= format[i + 1] && format[i + 1] <= '9') {
      precision = format[i + 1] - '0';
      i += 2;
      while ('0' <= format[i] && format[i] <= '9' && precision < PHP_BUF_LEN) {
        precision = precision * 10 + format[i++] - '0';
      }
    }

    if (precision >= PHP_BUF_LEN) {
      error_too_big = true;
      break;
    }

    string piece;
    if (format[i] == '%') {
      piece = PERCENT;
    } else {
      if (arg_num == -2) {
        arg_num = cur_arg++;
      }

      if (arg_num >= a.count()) {
        php_warning("Not enough parameters to call function sprintf with format \"%s\"", format.c_str());
        return {};
      }

      if (arg_num == -1) {
        php_warning("Wrong parameter number 0 specified in function sprintf with format \"%s\"", format.c_str());
        return {};
      }

      const mixed &arg = a.get_value(arg_num);

      if (arg.is_array()) {
        php_warning("Argument %d of function sprintf is array", arg_num);
        return {};
      }

      switch (format[i]) {
        case 'b': {
          auto arg_int = static_cast<uint64_t>(arg.to_int());
          int cur_pos = 70;
          do {
            php_buf[--cur_pos] = (char)((arg_int & 1) + '0');
            arg_int >>= 1;
          } while (arg_int > 0);
          piece.assign(php_buf + cur_pos, 70 - cur_pos);
          break;
        }
        case 'c': {
          int64_t arg_int = arg.to_int();
          if (arg_int <= -128 || arg_int > 255) {
            php_warning("Wrong parameter for specifier %%c in function sprintf with format \"%s\"", format.c_str());
          }
          piece.assign(1, (char)arg_int);
          break;
        }
        case 'd': {
          int64_t arg_int = arg.to_int();
          if (sign == '+' && arg_int >= 0) {
            piece = (static_SB.clean() << "+" << arg_int).str();
          } else {
            piece = string(arg_int);
          }
          break;
        }
        case 'u': {
          auto arg_int = static_cast<uint64_t>(arg.to_int());
          int cur_pos = 70;
          do {
            php_buf[--cur_pos] = (char)(arg_int % 10 + '0');
            arg_int /= 10;
          } while (arg_int > 0);
          piece.assign(php_buf + cur_pos, 70 - cur_pos);
          break;
        }
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G': {
          double arg_float = arg.to_float();

          static_SB.clean() << '%';
          if (sign) {
            static_SB << sign;
          }
          if (precision >= 0) {
            static_SB << '.' << precision;
          }
          static_SB << format[i];

          int len = snprintf(php_buf, PHP_BUF_LEN, static_SB.c_str(), arg_float);
          if (len >= PHP_BUF_LEN) {
            error_too_big = true;
            break;
          }

          piece.assign(php_buf, len);
          break;
        }
        case 'o': {
          auto arg_int = static_cast<uint64_t>(arg.to_int());
          int cur_pos = 70;
          do {
            php_buf[--cur_pos] = (char)((arg_int & 7) + '0');
            arg_int >>= 3;
          } while (arg_int > 0);
          piece.assign(php_buf + cur_pos, 70 - cur_pos);
          break;
        }
        case 's': {
          string arg_string = arg.to_string();

          static_SB.clean() << '%';
          if (precision >= 0) {
            static_SB << '.' << precision;
          }
          static_SB << 's';

          int len = snprintf(php_buf, PHP_BUF_LEN, static_SB.c_str(), arg_string.c_str());
          if (len >= PHP_BUF_LEN) {
            error_too_big = true;
            break;
          }

          piece.assign(php_buf, len);
          break;
        }
        case 'x':
        case 'X': {
          const char *hex_digits = (format[i] == 'x' ? lhex_digits : uhex_digits);
          auto arg_int = static_cast<uint64_t>(arg.to_int());

          int cur_pos = 70;
          do {
            php_buf[--cur_pos] = hex_digits[arg_int & 15];
            arg_int >>= 4;
          } while (arg_int > 0);
          piece.assign(php_buf + cur_pos, 70 - cur_pos);
          break;
        }
        default:
          php_warning("Unsupported specifier %%%c in sprintf with format \"%s\"", format[i], format.c_str());
          return {};
      }
    }

    result.append(f$str_pad(piece, width, string(1, filler), pad_right));
  }

  if (error_too_big) {
    php_warning("Too big result in function sprintf");
    return {};
  }

  return result;
}

string f$stripcslashes(const string &str) {
  if (str.empty()) {
    return str;
  }

  // this implementation is an adapted version from php-src

  auto len = str.size();
  auto new_len = len;
  string result(len, false);
  char *result_c_str = &result[0];
  char num_tmp[4]; // we need up to three digits + a space for null-terminator
  int j = 0;

  for (int i = 0; i < len; i++) {
    if (str[i] != '\\' || i + 1 >= len) {
      *result_c_str++ = str[i];
    } else {
      i++; // step over a backslash
      switch (str[i]) {
        case 'n':
          *result_c_str++ = '\n';
          new_len--;
          break;
        case 'r':
          *result_c_str++ = '\r';
          new_len--;
          break;
        case 'a':
          *result_c_str++ = '\a';
          new_len--;
          break;
        case 't':
          *result_c_str++ = '\t';
          new_len--;
          break;
        case 'v':
          *result_c_str++ = '\v';
          new_len--;
          break;
        case 'b':
          *result_c_str++ = '\b';
          new_len--;
          break;
        case 'f':
          *result_c_str++ = '\f';
          new_len--;
          break;
        case '\\':
          *result_c_str++ = '\\';
          new_len--;
          break;
        case 'x': // \\xN or \\xNN
          // collect up to two hex digits and interpret them as char
          if (i+1 < len && isxdigit(static_cast<int>(str[i+1]))) {
            num_tmp[0] = str[++i];
            if (i+1 < len && isxdigit(static_cast<int>(str[i+1]))) {
              num_tmp[1] = str[++i];
              num_tmp[2] = '\0';
              new_len -= 3;
            } else {
              num_tmp[1] = '\0';
              new_len -= 2;
            }
            *result_c_str++ = static_cast<char>(strtol(num_tmp, nullptr, 16));
          } else {
            // not a hex literal, just copy a char as i
            *result_c_str++ = str[i];
            new_len--;
          }
          break;
        default: // \N \NN \NNN
          // collect up to three octal digits and interpret them as char
          j = 0;
          while (i < len && str[i] >= '0' && str[i] <= '7' && j < 3) {
            num_tmp[j++] = str[i++];
          }
          if (j) {
            num_tmp[j] ='\0';
            *result_c_str++ = static_cast<char>(strtol(num_tmp, nullptr, 8));
            new_len -= j;
            i--;
          } else {
            // not an octal literal, just copy a char as is
            *result_c_str++ = str[i];
            new_len--;
          }
      }
    }
  }

  if (new_len != 0) {
    *result_c_str = '\0';
  }
  result.shrink(new_len);
  return result;
}

string f$stripslashes(const string &str) {
  int len = str.size();
  int i;

  string result(len, false);
  char *result_c_str = &result[0];
  for (i = 0; i + 1 < len; i++) {
    if (str[i] == '\\') {
      i++;
      if (str[i] == '0') {
        *result_c_str++ = '\0';
        continue;
      }
    }

    *result_c_str++ = str[i];
  }
  if (i + 1 == len && str[i] != '\\') {
    *result_c_str++ = str[i];
  }
  result.shrink(static_cast<string::size_type>(result_c_str - result.c_str()));
  return result;
}

int64_t f$strcasecmp(const string &lhs, const string &rhs) {
  int n = min(lhs.size(), rhs.size());
  for (int i = 0; i < n; i++) {
    if (tolower(lhs[i]) != tolower(rhs[i])) {
      return tolower(lhs[i]) - tolower(rhs[i]);
    }
  }
  // TODO: for PHP8.2, use <=> operator instead:
  //   return spaceship(static_cast<int64_t>(lhs.size()), static_cast<int64_t>(rhs.size()));
  return static_cast<int64_t>(lhs.size()) - static_cast<int64_t>(rhs.size());
}

int64_t f$strcmp(const string &lhs, const string &rhs) {
  return lhs.compare(rhs);
}

Optional<int64_t> f$stripos(const string &haystack, const string &needle, int64_t offset) {
  if (offset < 0) {
    php_warning("Wrong offset = %" PRIi64 " in function stripos", offset);
    return false;
  }
  if (offset >= haystack.size()) {
    return false;
  }
  if (needle.size() == 0) {
    php_warning("Parameter needle is empty in function stripos");
    return false;
  }

  const char *s = strcasestr(haystack.c_str() + offset, needle.c_str());
  if (s == nullptr) {
    return false;
  }
  return s - haystack.c_str();
}

static bool php_tag_find(const string &tag, const string &allow) {
  if (tag.empty() || allow.empty()) {
    return false;
  }

  string norm;
  int state = 0, done = 0;
  for (int i = 0; tag[i] && !done; i++) {
    char c = (char)tolower(tag[i]);
    switch (c) {
      case '<':
        norm.push_back(c);
        break;
      case '>':
        done = 1;
        break;
      default:
        if (!isspace(c)) {
          // since PHP5.3.4, self-closing tags are interpreted as normal tags,
          // so normalized <br/> = <br>; note that tags from $allow are not normalized
          if (c != '/') {
            norm.push_back(c);
          }
          if (state == 0) {
            state = 1;
          }
        } else {
          if (state == 1) {
            done = 1;
          }
        }
        break;
    }
  }
  norm.push_back('>');
  return memmem(allow.c_str(), allow.size(), norm.c_str(), norm.size()) != nullptr;
}

string f$strip_tags(const string &str, const string &allow) {
  int br = 0, depth = 0, in_q = 0;
  int state = 0;

  const string allow_low = f$strtolower(allow);

  static_SB.clean();
  static_SB_spare.clean();
  char lc = 0;
  int len = str.size();
  for (int i = 0; i < len; i++) {
    char c = str[i];
    switch (c) {
      case '\0':
        break;
      case '<':
        if (!in_q) {
          if (isspace(str[i + 1])) {
            if (state == 0) {
              static_SB << c;
            } else if (state == 1) {
              static_SB_spare << c;
            }
          } else if (state == 0) {
            lc = '<';
            state = 1;
            static_SB_spare << '<';
          } else if (state == 1) {
            depth++;
          }
        }
        break;
      case '(':
        if (state == 2) {
          if (lc != '"' && lc != '\'') {
            lc = '(';
            br++;
          }
        } else if (state == 1) {
          static_SB_spare << c;
        } else if (state == 0) {
          static_SB << c;
        }
        break;
      case ')':
        if (state == 2) {
          if (lc != '"' && lc != '\'') {
            lc = ')';
            br--;
          }
        } else if (state == 1) {
          static_SB_spare << c;
        } else if (state == 0) {
          static_SB << c;
        }
        break;
      case '>':
        if (depth) {
          depth--;
          break;
        }

        if (in_q) {
          break;
        }

        switch (state) {
          case 1: /* HTML/XML */
            lc = '>';
            in_q = state = 0;
            static_SB_spare << '>';
            if (php_tag_find(static_SB_spare.str(), allow_low)) {
              static_SB << static_SB_spare.c_str();
            }
            static_SB_spare.clean();
            break;
          case 2: /* PHP */
            if (!br && lc != '\"' && str[i - 1] == '?') {
              in_q = state = 0;
              static_SB_spare.clean();
            }
            break;
          case 3:
            in_q = state = 0;
            static_SB_spare.clean();
            break;
          case 4: /* JavaScript/CSS/etc... */
            if (i >= 2 && str[i - 1] == '-' && str[i - 2] == '-') {
              in_q = state = 0;
              static_SB_spare.clean();
            }
            break;
          default:
            static_SB << c;
            break;
        }
        break;

      case '"':
      case '\'':
        if (state == 4) {
          /* Inside <!-- comment --> */
          break;
        } else if (state == 2 && str[i - 1] != '\\') {
          if (lc == c) {
            lc = 0;
          } else if (lc != '\\') {
            lc = c;
          }
        } else if (state == 0) {
          static_SB << c;
        } else if (state == 1) {
          static_SB_spare << c;
        }
        if (state && i > 0 && (state == 1 || str[i - 1] != '\\') && (!in_q || c == in_q)) {
          if (in_q) {
            in_q = 0;
          } else {
            in_q = c;
          }
        }
        break;
      case '!':
        /* JavaScript & Other HTML scripting languages */
        if (state == 1 && str[i - 1] == '<') {
          state = 3;
          lc = c;
        } else {
          if (state == 0) {
            static_SB << c;
          } else if (state == 1) {
            static_SB_spare << c;
          }
        }
        break;
      case '-':
        if (state == 3 && i >= 2 && str[i - 1] == '-' && str[i - 2] == '!') {
          state = 4;
        } else {
          if (state == 0) {
            static_SB << c;
          } else if (state == 1) {
            static_SB_spare << c;
          }
        }
        break;
      case '?':
        if (state == 1 && str[i - 1] == '<') {
          br = 0;
          state = 2;
          break;
        }
        /* fall-through */
      case 'E':
      case 'e':
        /* !DOCTYPE exception */
        if (state == 3 && i > 6
            && tolower(str[i - 1]) == 'p'
            && tolower(str[i - 2]) == 'y'
            && tolower(str[i - 3]) == 't'
            && tolower(str[i - 4]) == 'c'
            && tolower(str[i - 5]) == 'o'
            && tolower(str[i - 6]) == 'd') {
          state = 1;
          break;
        }
        /* fall-through */
      case 'l':
      case 'L':
        /* swm: If we encounter '<?xml' then we shouldn't be in
         * state == 2 (PHP). Switch back to HTML.
         */

        if (state == 2 && i > 2 && tolower(str[i - 1]) == 'm' && tolower(str[i - 2]) == 'x') {
          state = 1;
          break;
        }

        /* fall-through */
      default:
        if (state == 0) {
          static_SB << c;
        } else if (state == 1) {
          static_SB_spare << c;
        }
        break;
    }
  }

  return static_SB.str();
}

template <class T>
string strip_tags_string(const array<T> &list) {
  string allow_str;
  if (!list.empty()) {
    allow_str.reserve_at_least(list.count() * strlen("<br>"));
    for (const auto &it : list) {
      const auto &s = it.get_value();
      if (!s.empty()) {
        allow_str.push_back('<');
        allow_str.append(f$strval(s));
        allow_str.push_back('>');
      }
    }
  }
  return allow_str;
}

string f$strip_tags(const string &str, const array<Unknown> &allow_list) {
  php_assert(allow_list.empty());
  return f$strip_tags(str, string());
}

string f$strip_tags(const string &str, const array<string> &allow_list) {
  return f$strip_tags(str, strip_tags_string(allow_list));
}

string f$strip_tags(const string &str, const mixed &allow) {
  if (!allow.is_array()) {
    return f$strip_tags(str, allow.to_string());
  }
  auto allow_list = allow.to_array();
  return f$strip_tags(str, strip_tags_string(allow_list));
}

Optional<string> f$stristr(const string &haystack, const string &needle, bool before_needle) {
  if ((int)needle.size() == 0) {
    php_warning("Parameter needle is empty in function stristr");
    return false;
  }

  const char *s = strcasestr(haystack.c_str(), needle.c_str());
  if (s == nullptr) {
    return false;
  }

  const auto pos = static_cast<string::size_type>(s - haystack.c_str());
  if (before_needle) {
    return haystack.substr(0, pos);
  }
  return haystack.substr(pos, haystack.size() - pos);
}

Optional<string> f$strrchr(const string &haystack, const string &needle) {
  if (needle.empty()) {
    php_warning("Parameter needle is empty in function strrchr");
    return false;
  }
  if (needle.size() > 1) {
    php_warning("Parameter needle contains more than one character, only the first is used");
  }
  const char needle_char = needle[0];
  for (string::size_type pos = haystack.size(); pos != 0; --pos) {
    if (haystack[pos - 1] == needle_char) {
      return haystack.substr(pos - 1, haystack.size() - pos + 1);
    }
  }
  return false;
}

int64_t f$strncmp(const string &lhs, const string &rhs, int64_t len) {
  if (len < 0) {
    return 0;
  }
  return memcmp(lhs.c_str(), rhs.c_str(), min(int64_t{min(lhs.size(), rhs.size())} + 1, len));
}

/*
  Modified for PHP by Andrei Zmievski <andrei@ispi.net>
  Modified for KPHP by Niyaz Nigmatullin

  compare_right, compare_left and strnatcmp_ex functions
  Copyright (C) 2000 by Martin Pool <mbp@humbug.org.au>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

static int64_t compare_right(char const **a, char const *aend, char const **b, char const *bend) {
  int64_t bias = 0;

  /* The longest run of digits wins.  That aside, the greatest
     value wins, but we can't know that it will until we've scanned
     both numbers to know that they have the same magnitude, so we
     remember it in BIAS. */
  for (;; (*a)++, (*b)++) {
    if ((*a == aend || !isdigit((int32_t)(unsigned char)**a)) &&
        (*b == bend || !isdigit((int32_t)(unsigned char)**b))) {
      return bias;
    } else if (*a == aend || !isdigit((int32_t)(unsigned char)**a)) {
      return -1;
    } else if (*b == bend || !isdigit((int32_t)(unsigned char)**b)) {
      return +1;
    } else if (**a < **b) {
      if (!bias) {
        bias = -1;
      }
    } else if (**a > **b) {
      if (!bias) {
        bias = +1;
      }
    }
  }

  return 0;
}

static int64_t compare_left(char const **a, char const *aend, char const **b, char const *bend) {
  /* Compare two left-aligned numbers: the first to have a
     different value wins. */
  for (;; (*a)++, (*b)++) {
    if ((*a == aend || !isdigit((int32_t)(unsigned char)**a)) &&
        (*b == bend || !isdigit((int32_t)(unsigned char)**b))) {
      return 0;
    } else if (*a == aend || !isdigit((int32_t)(unsigned char)**a)) {
      return -1;
    } else if (*b == bend || !isdigit((int32_t)(unsigned char)**b)) {
      return +1;
    } else if (**a < **b) {
      return -1;
    } else if (**a > **b) {
      return +1;
    }
  }

  return 0;
}

static int64_t strnatcmp_ex(char const *a, size_t a_len, char const *b, size_t b_len, int64_t fold_case) {
  unsigned char ca, cb;
  char const *ap, *bp;
  char const *aend = a + a_len,
    *bend = b + b_len;
  bool fractional = false;
  int64_t result = 0;
  short leading = 1;

  if (a_len == 0 || b_len == 0) {
    return (a_len == b_len ? 0 : (a_len > b_len ? 1 : -1));
  }

  ap = a;
  bp = b;
  while (true) {
    ca = *ap;
    cb = *bp;

    /* skip over leading zeros */
    while (leading && ca == '0' && (ap + 1 < aend) && isdigit((int32_t)(unsigned char)*(ap + 1))) {
      ca = *++ap;
    }

    while (leading && cb == '0' && (bp + 1 < bend) && isdigit((int32_t)(unsigned char)*(bp + 1))) {
      cb = *++bp;
    }

    leading = 0;

    /* Skip consecutive whitespace */
    while (isspace((int32_t)(unsigned char)ca)) {
      ca = *++ap;
    }

    while (isspace((int32_t)(unsigned char)cb)) {
      cb = *++bp;
    }

    /* process run of digits */
    if (isdigit((int32_t)(unsigned char)ca) && isdigit((int32_t)(unsigned char)cb)) {
      fractional = (ca == '0' || cb == '0');

      if (fractional) {
        result = compare_left(&ap, aend, &bp, bend);
      } else {
        result = compare_right(&ap, aend, &bp, bend);
      }

      if (result != 0) {
        return result;
      }

      if (ap == aend && bp == bend) {
        /* End of the strings. Let caller sort them out. */
        return 0;
      } else {
        /* Keep on comparing from the current point. */
        ca = *ap;
        cb = *bp;
      }
    }

    if (fold_case) {
      ca = static_cast<unsigned char>(toupper(ca));
      cb = static_cast<unsigned char>(toupper(cb));
    }

    if (ca < cb) {
      return -1;
    } else if (ca > cb) {
      return +1;
    }

    ++ap;
    ++bp;
    if (ap >= aend && bp >= bend) {
      /* The strings compare the same.  Perhaps the caller
         will want to call strcmp to break the tie. */
      return 0;
    } else if (ap >= aend) {
      return -1;
    } else if (bp >= bend) {
      return 1;
    }
  }
}


int64_t f$strnatcmp(const string &lhs, const string &rhs) {
  return strnatcmp_ex(lhs.c_str(), lhs.size(), rhs.c_str(), rhs.size(), 0);
}

int64_t f$strspn(const string &hayshack, const string &char_list, int64_t offset) noexcept {
  return strspn(hayshack.c_str() + hayshack.get_correct_offset_clamped(offset), char_list.c_str());
}

int64_t f$strcspn(const string &hayshack, const string &char_list, int64_t offset) noexcept {
  return strcspn(hayshack.c_str() + hayshack.get_correct_offset_clamped(offset), char_list.c_str());
}

Optional<string> f$strpbrk(const string &haystack, const string &char_list) {
  const char *pos = strpbrk(haystack.c_str(), char_list.c_str());
  if (pos == nullptr) {
    return false;
  }

  return string(pos, static_cast<string::size_type>(haystack.size() - (pos - haystack.c_str())));
}

Optional<int64_t> f$strpos(const string &haystack, const string &needle, int64_t offset) {
  if (offset < 0) {
    php_warning("Wrong offset = %" PRIi64 " in function strpos", offset);
    return false;
  }
  if (offset > int64_t{haystack.size()}) {
    return false;
  }
  if (needle.size() <= 1) {
    if (needle.size() == 0) {
      php_warning("Parameter needle is empty in function strpos");
      return false;
    }

    const char *s = static_cast <const char *> (memchr(haystack.c_str() + offset, needle[0], haystack.size() - offset));
    if (s == nullptr) {
      return false;
    }
    return s - haystack.c_str();
  }

  const char *s = static_cast <const char *> (memmem(haystack.c_str() + offset, haystack.size() - offset, needle.c_str(), needle.size()));
  if (s == nullptr) {
    return false;
  }
  return s - haystack.c_str();
}

Optional<int64_t> f$strrpos(const string &haystack, const string &needle, int64_t offset) {
  const char *end = haystack.c_str() + haystack.size();
  if (offset < 0) {
    offset += haystack.size() + 1;
    if (offset < 0) {
      return false;
    }

    end = haystack.c_str() + offset;
    offset = 0;
  }
  if (offset >= haystack.size()) {
    return false;
  }
  if (needle.size() == 0) {
    php_warning("Parameter needle is empty in function strrpos");
    return false;
  }

  const char *s = static_cast <const char *> (memmem(haystack.c_str() + offset, haystack.size() - offset, needle.c_str(), needle.size())), *t;
  if (s == nullptr || s >= end) {
    return false;
  }
  while ((t = static_cast <const char *> (memmem(s + 1, haystack.c_str() + haystack.size() - s - 1, needle.c_str(), needle.size()))) != nullptr && t < end) {
    s = t;
  }
  return s - haystack.c_str();
}

Optional<int64_t> f$strripos(const string &haystack, const string &needle, int64_t offset) {
  const char *end = haystack.c_str() + haystack.size();
  if (offset < 0) {
    offset += haystack.size() + 1;
    if (offset < 0) {
      return false;
    }

    end = haystack.c_str() + offset;
    offset = 0;
  }
  if (offset >= haystack.size()) {
    return false;
  }
  if (needle.size() == 0) {
    php_warning("Parameter needle is empty in function strripos");
    return false;
  }

  const char *s = strcasestr(haystack.c_str() + offset, needle.c_str()), *t;
  if (s == nullptr || s >= end) {
    return false;
  }
  while ((t = strcasestr(s + 1, needle.c_str())) != nullptr && t < end) {
    s = t;
  }
  return s - haystack.c_str();
}

string f$strrev(const string &str) {
  int n = str.size();

  string res(n, false);
  for (int i = 0; i < n; i++) {
    res[n - i - 1] = str[i];
  }

  return res;
}

Optional<string> f$strstr(const string &haystack, const string &needle, bool before_needle) {
  if ((int)needle.size() == 0) {
    php_warning("Parameter needle is empty in function strstr");
    return false;
  }

  const char *s = static_cast <const char *> (memmem(haystack.c_str(), haystack.size(), needle.c_str(), needle.size()));
  if (s == nullptr) {
    return false;
  }

  const auto pos = static_cast<string::size_type>(s - haystack.c_str());
  if (before_needle) {
    return haystack.substr(0, pos);
  }
  return haystack.substr(pos, haystack.size() - pos);
}

string f$strtolower(const string &str) {
  int n = str.size();

  // if there is no upper case char inside the string, we can
  // return the argument unchanged, avoiding the allocation and data copying;
  // while at it, memorize the first upper case char, so we can
  // use memcpy to copy everything before that pos;
  // note: do not use islower() here, the compiler does not inline that function call;
  // it could be beneficial to use 256-byte LUT here, but SIMD approach could be even better
  const char *end = str.c_str() + n;
  const char *uppercase_pos = std::find_if(str.c_str(), end, [](unsigned char ch) {
    return ch >= 'A' && ch <= 'Z';
  });
  if (uppercase_pos == end) {
    return str;
  }

  string res(n, false);
  int64_t lowercase_prefix = uppercase_pos - str.c_str();
  if (lowercase_prefix != 0) { // avoid unnecessary function call
    std::memcpy(res.buffer(), str.c_str(), lowercase_prefix);
  }
  for (int i = lowercase_prefix; i < n; i++) {
    res[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
  }

  return res;
}

string f$strtoupper(const string &str) {
  int n = str.size();

  // same optimization as in strtolower
  const char *end = str.c_str() + n;
  const char *lowercase_pos = std::find_if(str.c_str(), end, [](unsigned char ch) {
    return ch >= 'a' && ch <= 'z';
  });
  if (lowercase_pos == end) {
    return str;
  }

  string res(n, false);
  int64_t uppercase_prefix = lowercase_pos - str.c_str();
  if (uppercase_prefix != 0) { // avoid unnecessary function call
    std::memcpy(res.buffer(), str.c_str(), uppercase_prefix);
  }
  for (int i = uppercase_prefix; i < n; i++) {
    res[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(str[i])));
  }

  return res;
}

string f$strtr(const string &subject, const string &from, const string &to) {
  int n = subject.size();
  string result(n, false);
  for (int i = 0; i < n; i++) {
    const char *p = static_cast <const char *> (memchr(static_cast <const void *> (from.c_str()), (int)(unsigned char)subject[i], (size_t)from.size()));
    if (p == nullptr || static_cast<string::size_type>(p - from.c_str()) >= to.size()) {
      result[i] = subject[i];
    } else {
      result[i] = to[static_cast<string::size_type>(p - from.c_str())];
    }
  }
  return result;
}

string f$str_pad(const string &input, int64_t len, const string &pad_str, int64_t pad_type) {
  string::size_type old_len = input.size();
  if (len <= old_len) {
    return input;
  }
  if (len > string::max_size()) {
    php_critical_error ("tried to allocate too big string of size %" PRIi64, len);
  }

  const auto strlen = static_cast<string::size_type>(len);

  string::size_type pad_left = 0;
  string::size_type pad_right = 0;
  if (pad_type == STR_PAD_RIGHT) {
    pad_right = strlen - old_len;
  } else if (pad_type == STR_PAD_LEFT) {
    pad_left = strlen - old_len;
  } else if (pad_type == STR_PAD_BOTH) {
    pad_left = (strlen - old_len) / 2;
    pad_right = (strlen - old_len + 1) / 2;
  } else {
    php_warning("Wrong parameter pad_type in function str_pad");
    return input;
  }

  string::size_type pad_len = pad_str.size();
  if (pad_len == 0) {
    php_warning("Wrong parameter pad_str (empty string) in function str_pad");
    return input;
  }

  string res(strlen, false);
  for (string::size_type i = 0; i < pad_left; i++) {
    res[i] = pad_str[i % pad_len];
  }
  memcpy(&res[pad_left], input.c_str(), old_len);
  for (string::size_type i = 0; i < pad_right; i++) {
    res[i + pad_left + old_len] = pad_str[i % pad_len];
  }

  return res;
}

string f$str_repeat(const string &s, int64_t multiplier) {
  const string::size_type len = s.size();
  if (multiplier <= 0 || len == 0) {
    return {};
  }

  auto mult = static_cast<string::size_type>(multiplier);
  if (string::max_size() / len < mult) {
    php_critical_error ("tried to allocate too big string of size %" PRIi64, multiplier * len);
  }

  if (len == 1) {
    return {mult, s[0]};
  }

  string result(mult * len, false);
  if (len >= 5) {
    while (mult--) {
      memcpy(&result[mult * len], s.c_str(), len);
    }
  } else {
    for (string::size_type i = 0; i < mult; i++) {
      for (string::size_type j = 0; j < len; j++) {
        result[i * len + j] = s[j];
      }
    }
  }
  return result;
}

static string str_replace_char(char c, const string &replace, const string &subject, int64_t &replace_count, bool with_case) {
  int count = 0;
  const char *piece = subject.c_str();
  const char *piece_end = subject.c_str() + subject.size();

  string result;
  if (!replace.empty()) {
    result.reserve_at_least(subject.size());
  }

  while (true) {
    const char *pos = nullptr;
    if (with_case) {
      pos = static_cast<const char *>(memchr(piece, c, piece_end - piece));
    } else {
      const char needle[] = {c, '\0'};
      pos = strcasestr(piece, needle);
    }

    if (pos == nullptr) {
      if (count == 0) {
        return subject;
      }
      replace_count += count;
      result.append(piece, static_cast<string::size_type>(piece_end - piece));
      return result;
    }

    ++count;

    result.append(piece, static_cast<string::size_type>(pos - piece));
    result.append(replace);

    piece = pos + 1;
  }
  php_assert (0); // unreachable
  return {};
}

static const char *find_substr(const char *where, const char *where_end, const string &what, bool with_case) {
  if (with_case) {
    return static_cast<char *>(memmem(where, where_end - where, what.c_str(), what.size()));
  }

  return strcasestr(where, what.c_str());
}

void str_replace_inplace(const string &search, const string &replace, string &subject, int64_t &replace_count, bool with_case) {
  if (search.empty()) {
    php_warning("Parameter search is empty in function str_replace");
    return;
  }

  subject.make_not_shared();

  int count = 0;
  const char *piece = subject.c_str();
  const char *piece_end = subject.c_str() + subject.size();

  char *output = subject.buffer();
  bool length_no_change = search.size() == replace.size();

  while (true) {
    const char *pos = find_substr(piece, piece_end, search, with_case);
    if (pos == nullptr) {
      if (count == 0) {
        return;
      }
      replace_count += count;
      if (!length_no_change) {
        memmove(output, piece, piece_end - piece);
      }
      output += piece_end - piece;
      if (!length_no_change) {
        subject.shrink(static_cast<string::size_type>(output - subject.c_str()));
      }
      return;
    }

    ++count;

    if (!length_no_change) {
      memmove(output, piece, pos - piece);
    }
    output += pos - piece;
    memcpy(output, replace.c_str(), replace.size());
    output += replace.size();

    piece = pos + search.size();
  }
  php_assert (0); // unreachable
}

string str_replace(const string &search, const string &replace, const string &subject, int64_t &replace_count, bool with_case) {
  if (search.empty()) {
    php_warning("Parameter search is empty in function str_replace");
    return subject;
  }

  int count = 0;
  const char *piece = subject.c_str();
  const char *piece_end = subject.c_str() + subject.size();

  string result;
  while (true) {
    const char *pos = find_substr(piece, piece_end, search, with_case);
    if (pos == nullptr) {
      if (count == 0) {
        return subject;
      }
      replace_count += count;
      result.append(piece, static_cast<string::size_type>(piece_end - piece));
      return result;
    }

    ++count;

    result.append(piece, static_cast<string::size_type>(pos - piece));
    result.append(replace);

    piece = pos + search.size();
  }
  php_assert (0); // unreachable
  return {};
}

// common for f$str_replace(string) and f$str_ireplace(string)
string str_replace_gen(const string &search, const string &replace, const string &subject, int64_t &replace_count, bool with_case);

string str_replace_string(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count, bool with_case) {
  if (search.is_array() && replace.is_array()) {
    return str_replace_string_array(search.as_array(""), replace.as_array(""), subject, replace_count, with_case);
  } else if (search.is_array()) {
    string result = subject;
    const string &replace_value = replace.to_string();

    for (array<mixed>::const_iterator it = search.begin(); it != search.end(); ++it) {
      const string &search_string = f$strval(it.get_value());
      if (search_string.size() >= replace_value.size()) {
        str_replace_inplace(search_string, replace_value, result, replace_count, with_case);
      } else {
        result = str_replace(search_string, replace_value, result, replace_count, with_case);
      }
    }
    return result;
  } else {
    if (replace.is_array()) {
      php_warning("Parameter mismatch, search is a string while replace is an array");
      //return false;
    }

    return str_replace_gen(f$strval(search), f$strval(replace), subject, replace_count, with_case);
  }
}

// common for f$str_replace(string) and f$str_ireplace(string)
string str_replace_gen(const string &search, const string &replace, const string &subject, int64_t &replace_count, bool with_case) {
  replace_count = 0;
  if (search.size() == 1) {
    return str_replace_char(search[0], replace, subject, replace_count, with_case);
  } else {
    return str_replace(search, replace, subject, replace_count, with_case);
  }
}

string f$str_replace(const string &search, const string &replace, const string &subject, int64_t &replace_count) {
  return str_replace_gen(search, replace, subject, replace_count, true);
}

string f$str_ireplace(const string &search, const string &replace, const string &subject, int64_t &replace_count) {
  return str_replace_gen(search, replace, subject, replace_count, false);
}

string f$str_replace(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count) {
  return str_replace_string(search, replace, subject, replace_count, true);
}

string f$str_ireplace(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count) {
  return str_replace_string(search, replace, subject, replace_count, false);
}

// common for f$str_replace(mixed) and f$str_ireplace(mixed)
mixed str_replace_gen(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count, bool with_case) {
  replace_count = 0;
  if (subject.is_array()) {
    array<mixed> result;
    for (array<mixed>::const_iterator it = subject.begin(); it != subject.end(); ++it) {
      mixed cur_result = str_replace_string(search, replace, it.get_value().to_string(), replace_count, with_case);
      if (!cur_result.is_null()) {
        result.set_value(it.get_key(), cur_result);
      }
    }
    return result;
  } else {
    return str_replace_string(search, replace, subject.to_string(), replace_count, with_case);
  }
}

mixed f$str_replace(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count) {
  return str_replace_gen(search, replace, subject, replace_count, true);
}

mixed f$str_ireplace(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count) {
  return str_replace_gen(search, replace, subject, replace_count, false);
}

array<string> f$str_split(const string &str, int64_t split_length) {
  if (split_length <= 0) {
    php_warning ("Wrong parameter split_length = %" PRIi64 " in function str_split", split_length);
    array<string> result(array_size(1, 0, true));
    result.set_value(0, str);
    return result;
  }

  array<string> result(array_size((str.size() + split_length - 1) / split_length, 0, true));
  string::size_type i = 0;
  for (i = 0; i + split_length <= str.size(); i += static_cast<string::size_type>(split_length)) {
    result.push_back(str.substr(i, static_cast<string::size_type>(split_length)));
  }
  if (i < str.size()) {
    result.push_back(str.substr(i, str.size() - i));
  }
  return result;
}

Optional<string> f$substr(const string &str, int64_t start, int64_t length) {
  if (!wrap_substr_args(str.size(), start, length)) {
    return false;
  }
  return str.substr(static_cast<string::size_type>(start), static_cast<string::size_type>(length));
}

Optional<string> f$substr(tmp_string str, int64_t start, int64_t length) {
  if (!wrap_substr_args(str.size, start, length)) {
    return false;
  }
  return string(str.data + start, length);
}

tmp_string f$_tmp_substr(const string &str, int64_t start, int64_t length) {
  if (!wrap_substr_args(str.size(), start, length)) {
    return {};
  }
  return {str.c_str() + start, static_cast<string::size_type>(length)};
}

tmp_string f$_tmp_substr(tmp_string str, int64_t start, int64_t length) {
  if (!wrap_substr_args(str.size, start, length)) {
    return {};
  }
  return {str.data + start, static_cast<string::size_type>(length)};
}

int64_t f$substr_count(const string &haystack, const string &needle, int64_t offset, int64_t length) {
  offset = haystack.get_correct_offset(offset);
  if (offset >= haystack.size()) {
    return 0;
  }
  if (length > haystack.size() - offset) {
    length = haystack.size() - offset;
  }

  int64_t ans = 0;
  const char *s = haystack.c_str() + offset, *end = haystack.c_str() + offset + length;
  if (needle.empty()) {
    php_warning("Needle is empty in function substr_count");
    return end - s;
  }
  do {
    s = static_cast <const char *> (memmem(static_cast <const void *> (s), (size_t)(end - s), static_cast <const void *> (needle.c_str()), (size_t)needle.size()));
    if (s == nullptr) {
      return ans;
    }
    ans++;
    s += needle.size();
  } while (true);
}

string f$substr_replace(const string &str, const string &replacement, int64_t start, int64_t length) {
  int64_t str_len = str.size();

  // if $start is negative, count $start from the end of the string
  start = str.get_correct_offset_clamped(start);

  // if $length is negative, set it to the length needed
  // needed to stop that many chars from the end of the string
  if (length < 0) {
    length = (str_len - start) + length;
    if (length < 0) {
      length = 0;
    }
  }

  if (length > str_len) {
    length = str_len;
  }
  if ((start + length) > str_len) {
    length = str_len - start;
  }

  auto result = str.substr(0, static_cast<string::size_type>(start));
  result.append(replacement);
  const auto offset = static_cast<string::size_type>(start + length);
  result.append(str.substr(offset, str.size() - offset));
  return result;
}

Optional<int64_t> f$substr_compare(const string &main_str, const string &str, int64_t offset, int64_t length, bool case_insensitivity) {
  int64_t str_len = main_str.size();

  if (length < 0) {
    php_warning("The length must be greater than or equal to zero in substr_compare function call");
    return false;
  }

  offset = main_str.get_correct_offset(offset);

  // > and >= signs depend on version of PHP7.2 and could vary unpredictably. We put `>` sign which corresponds to behaviour of PHP7.2.22
  if (offset > str_len) {
    php_warning("The start position cannot exceed initial string length in substr_compare function call");
    return false;
  }

  if (case_insensitivity) {
    return strncasecmp(main_str.c_str() + offset, str.c_str(), length);
  } else {
    return strncmp(main_str.c_str() + offset, str.c_str(), length);
  }
}

bool f$str_starts_with(const string &haystack, const string &needle) {
  return haystack.starts_with(needle);
}

bool f$str_ends_with(const string &haystack, const string &needle) {
  return haystack.ends_with(needle);
}

tmp_string trim_impl(const char *s, string::size_type s_len, const string &what) {
  const char *mask = get_mask(what);

  int len = s_len;
  if (len == 0 || (!mask[(unsigned char)s[len - 1]] && !mask[(unsigned char)s[0]])) {
    return {s, s_len};
  }

  while (len > 0 && mask[(unsigned char)s[len - 1]]) {
    len--;
  }

  if (len == 0) {
    return {};
  }

  int l = 0;
  while (mask[(unsigned char)s[l]]) {
    l++;
  }
  return {s + l, static_cast<string::size_type>(len - l)};
}

tmp_string f$_tmp_trim(tmp_string s, const string &what) {
  return trim_impl(s.data, s.size, what);
}

tmp_string f$_tmp_trim(const string &s, const string &what) {
  return trim_impl(s.c_str(), s.size(), what);
}

string f$trim(tmp_string s, const string &what) {
  return materialize_tmp_string(trim_impl(s.data, s.size, what));
}

string f$trim(const string &s, const string &what) {
  tmp_string result = trim_impl(s.c_str(), s.size(), what);
  if (result.data == s.c_str() && result.size == s.size()) {
    return s;
  }
  return materialize_tmp_string(result);
}

string f$ucfirst(const string &str) {
  int n = str.size();
  if (n == 0) {
    return str;
  }

  string res(n, false);
  res[0] = (char)toupper(str[0]);
  memcpy(&res[1], &str[1], n - 1);

  return res;
}

string f$ucwords(const string &str) {
  int n = str.size();

  bool in_word = false;
  string res(n, false);
  for (int i = 0; i < n; i++) {
    int cur = str[i] & 0xdf;
    if ('A' <= cur && cur <= 'Z') {
      if (in_word) {
        res[i] = str[i];
      } else {
        res[i] = (char)cur;
        in_word = true;
      }
    } else {
      res[i] = str[i];
      in_word = false;
    }
  }

  return res;
}

Optional<array<mixed>> f$unpack(const string &pattern, const string &data) {
  array<mixed> result;

  int data_len = data.size(), data_pos = 0;
  for (int i = 0; i < (int)pattern.size();) {
    char format = pattern[i++];
    int cnt = -1;
    if ('0' <= pattern[i] && pattern[i] <= '9') {
      cnt = 0;
      do {
        cnt = cnt * 10 + pattern[i++] - '0';
      } while ('0' <= pattern[i] && pattern[i] <= '9');

      if (cnt <= 0) {
        php_warning("Wrong count specifier in pattern \"%s\"", pattern.c_str());
        return false;
      }
    } else if (pattern[i] == '*') {
      cnt = 0;
      i++;
    }
    if (data_pos >= data_len) {
      if (format == 'A' || format == 'a' || format == 'H' || format == 'h' || cnt != 0) {
        php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
        return false;
      }
      return result;
    }

    const char *key_end = strchrnul(&pattern[i], '/');
    string key_prefix(pattern.c_str() + i, static_cast<string::size_type>(key_end - pattern.c_str() - i));
    i = (int)(key_end - pattern.c_str());
    if (i < (int)pattern.size()) {
      i++;
    }

    if (cnt == 0 && i != (int)pattern.size()) {
      php_warning("Misplaced symbol '*' in pattern \"%s\"", pattern.c_str());
      return false;
    }

    char filler = 0;
    switch (format) {
      case 'A':
        filler = ' ';
        /* fallthrough */
      case 'a': {
        if (cnt == 0) {
          cnt = data_len - data_pos;
        } else if (cnt == -1) {
          cnt = 1;
        }
        int read_len = cnt;
        if (read_len + data_pos > data_len) {
          php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
          return false;
        }
        while (cnt > 0 && data[data_pos + cnt - 1] == filler) {
          cnt--;
        }

        if (key_prefix.empty()) {
          key_prefix = ONE;
        }

        result.set_value(key_prefix, string(data.c_str() + data_pos, cnt));

        data_pos += read_len;
        break;
      }
      case 'h':
      case 'H': {
        if (cnt == 0) {
          cnt = (data_len - data_pos) * 2;
        } else if (cnt == -1) {
          cnt = 1;
        }

        int read_len = (cnt + 1) / 2;
        if (read_len + data_pos > data_len) {
          php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
          return false;
        }

        string value(cnt, false);
        for (int j = data_pos; cnt > 0; j++, cnt -= 2) {
          unsigned char ch = data[j];
          char num_high = lhex_digits[ch >> 4];
          char num_low = lhex_digits[ch & 15];
          if (format == 'h') {
            swap(num_high, num_low);
          }

          value[(j - data_pos) * 2] = num_high;
          if (cnt > 1) {
            value[(j - data_pos) * 2 + 1] = num_low;
          }
        }
        php_assert (cnt == 0 || cnt == -1);

        if (key_prefix.empty()) {
          key_prefix = ONE;
        }

        result.set_value(key_prefix, value);

        data_pos += read_len;
        break;
      }

      default: {
        if (key_prefix.empty() && cnt == -1) {
          key_prefix = ONE;
        }
        int counter = 1;
        do {
          mixed value;
          int value_int;
          if (data_pos >= data_len) {
            php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
            return false;
          }

          switch (format) {
            case 'c':
            case 'C':
              value_int = (int)data[data_pos++];
              if (format != 'c' && value_int < 0) {
                value_int += 256;
              }
              value = value_int;
              break;
            case 's':
            case 'S':
            case 'v':
              value_int = (unsigned char)data[data_pos];
              if (data_pos + 1 < data_len) {
                value_int |= data[data_pos + 1] << 8;
              }
              data_pos += 2;
              if (format != 's' && value_int < 0) {
                value_int += 65536;
              }
              value = value_int;
              break;
            case 'n':
              value_int = (unsigned char)data[data_pos] << 8;
              if (data_pos + 1 < data_len) {
                value_int |= (unsigned char)data[data_pos + 1];
              }
              data_pos += 2;
              value = value_int;
              break;
            case 'i':
            case 'I':
            case 'l':
            case 'L':
            case 'V':
              value_int = (unsigned char)data[data_pos];
              if (data_pos + 1 < data_len) {
                value_int |= (unsigned char)data[data_pos + 1] << 8;
                if (data_pos + 2 < data_len) {
                  value_int |= (unsigned char)data[data_pos + 2] << 16;
                  if (data_pos + 3 < data_len) {
                    value_int |= data[data_pos + 3] << 24;
                  }
                }
              }
              data_pos += 4;
              value = value_int;
              break;
            case 'N':
              value_int = (unsigned char)data[data_pos] << 24;
              if (data_pos + 1 < data_len) {
                value_int |= (unsigned char)data[data_pos + 1] << 16;
                if (data_pos + 2 < data_len) {
                  value_int |= (unsigned char)data[data_pos + 2] << 8;
                  if (data_pos + 3 < data_len) {
                    value_int |= (unsigned char)data[data_pos + 3];
                  }
                }
              }
              data_pos += 4;
              value = value_int;
              break;
            case 'f': {
              if (data_pos + (int)sizeof(float) > data_len) {
                php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
                return false;
              }
              value = (double)*(float *)(data.c_str() + data_pos);
              data_pos += (int)sizeof(float);
              break;
            }
            case 'e':
            case 'E':
            case 'd': {
              if (data_pos + (int)sizeof(double) > data_len) {
                php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
                return false;
              }
              uint64_t value_byteordered = 0;
              memcpy(&value_byteordered, data.c_str() + data_pos, sizeof(double));
              if (format == 'e') {
                value_byteordered = le64toh(value_byteordered);
              } else if (format == 'E') {
                value_byteordered = be64toh(value_byteordered);
              }
              value = float64_from_bits(value_byteordered);
              data_pos += (int)sizeof(double);
              break;
            }
            case 'J':
            case 'P':
            case 'Q': {
              if (data_pos + (int)sizeof(unsigned long long) > data_len) {
                php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
                return false;
              }

              // stored in the host machine order by the default (Q flag)
              unsigned long long value_byteordered = 0;
              memcpy(&value_byteordered, data.c_str() + data_pos, sizeof(value_byteordered));
              if (format == 'P') {
                // for P encode in little endian order
                value_byteordered = le64toh(value_byteordered);
              } else if (format == 'J') {
                // for J encode in big endian order
                value_byteordered = be64toh(value_byteordered);
              }

              const size_t buf_size = 20;
              char buf[buf_size];
              value = string{buf, static_cast<string::size_type>(simd_uint64_to_string(value_byteordered, buf) - buf)};
              data_pos += (int)sizeof(unsigned long long);
              break;
            }
            case 'q': {
              if (data_pos + (int)sizeof(long long) > data_len) {
                php_warning("Not enough data to unpack with format \"%s\"", pattern.c_str());
                return false;
              }
              long long value_ll = *reinterpret_cast<const long long *>(data.c_str() + data_pos);
              value = f$strval(static_cast<int64_t>(value_ll));
              data_pos += (int)sizeof(long long);
              break;
            }
            default:
              php_warning("Format code \"%c\" not supported", format);
              return false;
          }

          string key = key_prefix;
          if (cnt != -1) {
            key.append(string(counter++));
          }

          result.set_value(key, value);

          if (cnt == 0) {
            if (data_pos >= data_len) {
              return result;
            }
          }
        } while (cnt == 0 || --cnt > 0);
      }
    }
  }
  return result;
}

int64_t f$vprintf(const string &format, const array<mixed> &args) {
  return f$printf(format, args);
}

string f$vsprintf(const string &format, const array<mixed> &args) {
  return f$sprintf(format, args);
}

string f$wordwrap(const string &str, int64_t width, const string &brk, bool cut) {
  if (width <= 0) {
    php_warning("Wrong parameter width = %" PRIi64 " in function wordwrap", width);
    return str;
  }

  string result;
  string::size_type first = 0;
  const string::size_type n = str.size();
  int64_t last_space = -1;
  for (string::size_type i = 0; i < n; i++) {
    if (str[i] == ' ') {
      last_space = i;
    }
    if (i >= first + width && (cut || last_space > first)) {
      if (last_space <= first) {
        result.append(str, first, i - first);
        first = i;
      } else {
        result.append(str, first, static_cast<string::size_type>(last_space) - first);
        first = static_cast<string::size_type>(last_space + 1);
      }
      result.append(brk);
    }
  }
  result.append(str, first, str.size() - first);
  return result;
}

string f$xor_strings(const string &s, const string &t) {
  string::size_type length = min(s.size(), t.size());
  string result(length, ' ');
  const char *s_str = s.c_str();
  const char *t_str = t.c_str();
  char *res_str = result.buffer();
  for (string::size_type i = 0; i < length; i++) {
    *res_str = *s_str ^ *t_str;
    ++s_str;
    ++t_str;
    ++res_str;
  }
  return result;
}

namespace impl_ {
// Based on the original PHP implementation
// https://github.com/php/php-src/blob/e8678fcb42c5cb1ea38ff9c6819baca74c2bb5ea/ext/standard/string.c#L3375-L3418
inline size_t php_similar_str(vk::string_view first, vk::string_view second, size_t &pos1, size_t &pos2, size_t &count) {
  size_t max = 0;
  count = 0;
  for (const char *p = first.begin(); p != first.end(); ++p) {
    for (const char *q = second.begin(); q != second.end(); ++q) {
      size_t l = 0;
      for (; (p + l < first.end()) && (q + l < second.end()) && (p[l] == q[l]); ++l) {
      }
      if (l > max) {
        max = l;
        ++count;
        pos1 = p - first.begin();
        pos2 = q - second.begin();
      }
    }
  }
  return max;
}

size_t php_similar_char(vk::string_view first, vk::string_view second) {
  size_t pos1 = 0;
  size_t pos2 = 0;
  size_t count = 0;

  const size_t max = php_similar_str(first, second, pos1, pos2, count);
  size_t sum = max;
  if (sum) {
    if (pos1 && pos2 && count > 1) {
      sum += php_similar_char(first.substr(0, pos1), second.substr(0, pos2));
    }
    pos1 += max;
    pos2 += max;
    if (pos1 < first.size() && pos2 < second.size()) {
      sum += php_similar_char(first.substr(pos1), second.substr(pos2));
    }
  }
  return sum;
}

double default_similar_text_percent_stub{0.0};
} // namespace impl_

int64_t f$similar_text(const string &first, const string &second, double &percent) {
  if (first.empty() && second.empty()) {
    percent = 0.0;
    return 0;
  }
  const size_t sim = impl_::php_similar_char(vk::string_view{first.c_str(), first.size()}, vk::string_view{second.c_str(), second.size()});
  percent = static_cast<double>(sim) * 200.0 / (first.size() + second.size());
  return static_cast<int64_t>(sim);
}

string str_concat(const string &s1, const string &s2) {
  // for 2 argument concatenation it's not so uncommon to have at least one empty string argument;
  // it happens in cases like `$prefix . $s` where $prefix could be empty depending on some condition
  // real-world applications analysis shows that ~17.6% of all two arguments concatenations have
  // at least one empty string argument
  //
  // checking both lengths for 0 is almost free, but when we step into those 17.6%, we get almost x10
  // faster concatenation and no heap allocations
  //
  // this idea is borrowed from the Go runtime
  if (s1.empty()) {
    return s2;
  }
  if (s2.empty()) {
    return s1;
  }
  auto new_size = s1.size() + s2.size();
  return string(new_size, true).append_unsafe(s1).append_unsafe(s2).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2) {
  auto new_size = s1.size + s2.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3) {
  auto new_size = s1.size + s2.size + s3.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4) {
  auto new_size = s1.size + s2.size + s3.size + s4.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).append_unsafe(s4.as_tmp_string()).finish_append();
}

string str_concat(str_concat_arg s1, str_concat_arg s2, str_concat_arg s3, str_concat_arg s4, str_concat_arg s5) {
  auto new_size = s1.size + s2.size + s3.size + s4.size + s5.size;
  return string(new_size, true).append_unsafe(s1.as_tmp_string()).append_unsafe(s2.as_tmp_string()).append_unsafe(s3.as_tmp_string()).append_unsafe(s4.as_tmp_string()).append_unsafe(s5.as_tmp_string()).finish_append();
}
