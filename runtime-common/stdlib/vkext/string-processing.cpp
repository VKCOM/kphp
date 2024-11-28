// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC Â«V KontakteÂ»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/vkext/string-processing.h"

#include <stdlib.h>

#include "common/unicode/utf8-utils.h"

static int cmp_char(const void *a, const void *b) noexcept {
  return (int)(*(char *)a) - (int)(*(char *)b);
}

string sp_sort(const string &s) noexcept {
  string t = s.copy_and_make_not_shared();
  qsort(t.buffer(), t.size(), sizeof(char), cmp_char);

  return t;
}

string sp_to_upper(const string &s) noexcept {
  string t{s.size(), false};
  for (size_t i = 0; i <= t.size(); i++) {
    switch ((unsigned char)s[i]) {
      case 'a' ... 'z':
        t[i] = s[i] + 'A' - 'a';
        break;
      case 0xE0 ... 0xFF:
        t[i] = s[i] - 32;
        break;
      case 0x83:
        t[i] = 0x81;
        break;
      case 0xA2:
        t[i] = 0xA1;
        break;
      case 0xB8:
        t[i] = 0xA8;
        break;
      case 0xBC:
        t[i] = 0xA3;
        break;
      case 0xB4:
        t[i] = 0xA5;
        break;
      case 0xB3:
      case 0xBE:
        t[i] = s[i] - 1;
        break;
      case 0x98:
      case 0xA0:
      case 0xAD:
        t[i] = ' ';
        break;
      case 0x90:
      case 0x9A:
      case 0x9C ... 0x9F:
      case 0xBA:
      case 0xBF:
        t[i] = s[i] - 16;
        break;
      default:
        t[i] = s[i];
    }
  }

  return t;
}

static char to_lower(const char c) noexcept {
  switch ((unsigned char)c) {
    case 'A' ... 'Z':
    case 0xC0 ... 0xDF:
      return c + 'a' - 'A';
    case 0x81:
      return 0x83;
    case 0xA3:
      return 0xBC;
    case 0xA5:
      return 0xB4;
    case 0xA1:
    case 0xB2:
    case 0xBD:
      return c + 1;
    case 0x98:
    case 0xA0:
    case 0xAD:
      return ' ';
    case 0x80:
    case 0x8A:
    case 0x8C ... 0x8F:
    case 0xA8:
    case 0xAA:
    case 0xAF:
      return c + 16;
  }
  return c;
}

string sp_to_lower(const string &s) noexcept {
  string t{s.size(), false};
  for (size_t i = 0; i <= s.size(); i++) {
    t[i] = to_lower(s[i]);
  }

  return t;
}

static char simplify(const char c) noexcept {
  unsigned char cc = to_lower(c);
  switch (cc) {
    case '0' ... '9':
    case 'a' ... 'z':
    case 0xE0 ... 0xFF:
    case 0:
      return cc;
    case 0x83:     // CYRILLIC SMALL LETTER GJE
    case 0xB4:     // CYRILLIC SMALL LETTER GHE WITH UPTURN
      return 0xE3; // CYRILLIC SMALL LETTER GHE
    case 0xA2:     // CYRILLIC SMALL LETTER SHORT U
      return 0xF3; // CYRILLIC SMALL LETTER U
    case 0xB8:     // CYRILLIC SMALL LETTER IO
      return 0xE5; // CYRILLIC SMALL LETTER IE
    case 0xB3:     // CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
    case 0xBF:     // CYRILLIC SMALL LETTER YI
      return 'i';
    case 0xBE: // CYRILLIC SMALL LETTER DZE
      return 's';
    case 0x9A:     // CYRILLIC SMALL LETTER LJE
      return 0xEB; // CYRILLIC SMALL LETTER EL
    case 0x9C:     // CYRILLIC SMALL LETTER NJE
      return 0xED; // CYRILLIC SMALL LETTER EN
    case 0x9D:     // CYRILLIC SMALL LETTER KJE
      return 0xEA; // CYRILLIC SMALL LETTER KA
    case 0x90:     // CYRILLIC SMALL LETTER DJE
    case 0x9E:     // CYRILLIC SMALL LETTER TSHE
      return 'h';
    case 0xBA:     // CYRILLIC SMALL LETTER UKRAINIAN IE
      return 0xFD; // CYRILLIC SMALL LETTER E
    case 0xBC:     // CYRILLIC SMALL LETTER JE
      return 'j';
    case 0xA9: // COPYRIGHT SIGN
      return 'c';
    case 0xAE: // REGISTERED SIGN
      return 'r';
    case 0xB5: // MICRO SIGN
      return 'm';
  }
  return 0;
}

string sp_simplify(const string &s) noexcept {
  string t{s.size(), false};
  size_t nl = 0;
  for (size_t i = 0; i < s.size(); i++) {
    char c = simplify(s[i]);
    if (c != 0) {
      t[nl++] = c;
    }
  }
  t.shrink(nl);

  return t;
}

static char conv_letter(const char c) noexcept {
  switch (c) {
    case 'a':
      return 'à';
    case 'b':
      return 'â';
    case 'c':
      return 'ñ';
    case 'd':
      return 'd';
    case 'e':
      return 'å';
    case 'f':
      return 'f';
    case 'g':
      return 'g';
    case 'h':
      return 'í';
    case 'i':
      return 'i';
    case 'j':
      return 'j';
    case 'k':
      return 'ê';
    case 'l':
      return 'l';
    case 'm':
      return 'ò';
    case 'n':
      return 'ï';
    case 'o':
      return 'î';
    case 'p':
      return 'ð';
    case 'q':
      return 'q';
    case 'r':
      return 'r';
    case 's':
      return 's';
    case 't':
      return 'ò';
    case 'u':
      return 'è';
    case 'v':
      return 'v';
    case 'w':
      return 'w';
    case 'x':
      return 'õ';
    case 'y':
      return 'ó';
    case 'z':
      return 'z';
    case 'ì':
      return 'ò';
    case 'ú':
    case 'ü':
      return 'â';
    case 'à' ... 'ë':
    case 'í' ... 'ù':
    case 'û':
    case 'ý' ... 'ÿ':
      return c;
    case '0':
      return 'î';
    case '3':
      return 'ç';
    case '4':
      return '÷';
    case '6':
      return 'á';
    case '1' ... '2':
    case '5':
    case '7' ... '9':
      return c;
    default:
      return c;
  }
}

static char next_character(const char *s, size_t *_i) noexcept {
  int i = *_i;
  char cur = s[i];
  if (cur == '&') {
    if (s[i + 1] == 'a' && s[i + 2] == 'm' && s[i + 3] == 'p' && s[i + 4] == ';') {
      i += 4;
    } else if (s[i + 1] == '#') {
      int r = 0, ti = i;
      for (i += 2; '0' <= s[i] && s[i] <= '9'; i++) {
        r = r * 10 + s[i] - '0';
      }
      if (s[i] == ';') {
        int c = simplify_character(r);
        if (c <= 255) {
          cur = c;
        } else {
          cur = 0;
        }
      } else {
        i = ti;
      }
    } else if (s[i + 1] == 'l' && s[i + 2] == 't' && s[i + 3] == ';') {
      i += 3, cur = '<';
    } else if (s[i + 1] == 'g' && s[i + 2] == 't' && s[i + 3] == ';') {
      i += 3, cur = '>';
    } else if (s[i + 1] == 'q' && s[i + 2] == 'u' && s[i + 3] == 'o' && s[i + 4] == 't' && s[i + 5] == ';') {
      i += 5, cur = '"';
    }
  } else if (cur == '<') {
    if (s[i + 1] == 'b' && s[i + 2] == 'r' && s[i + 3] == '>') {
      i += 3, cur = '\n';
    }
  }
  *_i = i;

  return cur;
}

string sp_full_simplify(const string &s) noexcept {
  string t{s.size(), false};

  size_t nl = 0;
  for (size_t i = 0; i < s.size(); i++) {
    char c = next_character(s.c_str(), &i);
    c = simplify(c);
    if (c != 0) {
      t[nl++] = conv_letter(c);
    }
  }
  t.shrink(nl);

  return t;
}

string sp_deunicode(const string &s) noexcept {
  string t{s.size(), false};

  size_t nl = 0;
  for (size_t i = 0; i < s.size(); i++) {
    char c = next_character(s.c_str(), &i);
    if (c != 0) {
      t[nl++] = c;
    }
  }
  t.shrink(nl);

  return t;
}

string sp_remove_repeats(const string &s) noexcept {
  string t{s.size(), false};

  size_t nl = 0;
  for (size_t i = 0; i < s.size(); i++) {
    if (i == 0 || s[i] != s[i - 1]) {
      t[nl++] = s[i];
    }
  }
  t.shrink(nl);

  return t;
}

string sp_to_cyrillic(const string &s) noexcept {
  string t{s.size(), false};

  for (size_t i = 0; i <= s.size(); i++) {
    char c = simplify(s[i]);
    if (c == 0) {
      t[i] = s[i];
    } else {
      t[i] = conv_letter(c);
    }
  }

  return t;
}

string sp_words_only(const string &s) noexcept {
  string t{2 + s.size(), false};

  t[0] = ' ';
  size_t nl = 1;
  for (size_t i = 0; i < s.size(); i++) {
    char c = simplify(s[i]);
    if (c != 0) {
      t[nl++] = c;
    } else if (t[nl - 1] != ' ') {
      t[nl++] = ' ';
    }
  }
  if (t[nl - 1] != ' ') {
    t[nl++] = ' ';
  }
  t[nl] = 0;
  t.shrink(nl);

  return t;
}
