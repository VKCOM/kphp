// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "vkext/vkext.h"

#include <execinfo.h>
#include <php_ini.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "common/version-string.h"

#if PHP_VERSION_ID >= 80000
#include "vkext/vkext_arginfo.h"
#else
#include "vkext/vkext_legacy_arginfo.h"
#endif

#include "vkext/vkext-errors.h"
#include "vkext/vkext-flex.h"
#include "vkext/vkext-iconv.h"
#include "vkext/vkext-json.h"
#include "vkext/vkext-rpc-tl-serialization.h"
#include "vkext/vkext-rpc.h"
#include "vkext/vkext-tl-memcache.h"

#if __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wdate-time"
#endif

PHP_INI_MH (on_change_conffile);

PHP_INI_BEGIN ()
    PHP_INI_ENTRY ("tl.conffile", 0, PHP_INI_ALL, on_change_conffile)
    PHP_INI_ENTRY ("tl.conffile_autoreload", 0, PHP_INI_ALL, 0)
    PHP_INI_ENTRY ("vkext.ping_timeout", 0, PHP_INI_ALL, 0)
    PHP_INI_ENTRY ("vkext.use_unix", 0, PHP_INI_ALL, 0)
    PHP_INI_ENTRY ("vkext.unix_socket_directory", "/var/run/engine", PHP_INI_ALL, 0)
PHP_INI_END ()

zend_module_entry vkext_module_entry = {
  STANDARD_MODULE_HEADER,
  VKEXT_NAME,
  ext_functions,
  PHP_MINIT(vkext),
  PHP_MSHUTDOWN(vkext),
  PHP_RINIT(vkext),
  PHP_RSHUTDOWN(vkext),
  NULL,
  VKEXT_VERSION,
  STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(vkext)

int verbosity = 0;

char upcase_char(char c) {
  if (c >= 'a' && c <= 'z') {
    return c + 'A' - 'a';
  } else {
    return c;
  }
}

int utf8_to_win_char(int c) {
  if (c < 0x80) {
    return c;
  }
  if (c == 8238 || c == 8236 || c == 8235) {
    return 0xda;
  }
  switch (c & ~0xff) {
    case 0x400:
      return utf8_to_win_convert_0x400[c & 0xff];
    case 0x2000:
      return utf8_to_win_convert_0x2000[c & 0xff];
    case 0xff00:
      return utf8_to_win_convert_0xff00[c & 0xff];
    case 0x2100:
      return utf8_to_win_convert_0x2100[c & 0xff];
    case 0x000:
      return utf8_to_win_convert_0x000[c & 0xff];
  }
  return -1;
}

double get_double_time_since_epoch() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + 1e-6 * tv.tv_usec;
}

#define BUFF_LEN (1 << 16)
static char buff[BUFF_LEN];
char *wptr;
//int buff_pos;

char *result_buff;
int result_buff_len;
int result_buff_pos;
#define cur_buff_len ((wptr - buff) + result_buff_pos)


void init_buff(int tmp) {
  //buff_pos = 0;
  wptr = buff;
  result_buff_len = 0;
  result_buff_pos = 0;
}

void free_buff() {
  if (result_buff_len) {
    efree (result_buff);
  }
}

void realloc_buff() {
  if (!result_buff_len) {
    result_buff = static_cast<char *>(emalloc (BUFF_LEN));
    result_buff_len = BUFF_LEN;
  } else {
    result_buff = static_cast<char *>(erealloc (result_buff, 2 * result_buff_len));
    result_buff_len *= 2;
  }
}

void flush_buff() {
  while (result_buff_pos + (wptr - buff) > result_buff_len) {
    realloc_buff();
  }
  memcpy(result_buff + result_buff_pos, buff, wptr - buff);
  result_buff_pos += (wptr - buff);
  wptr = buff;
}

char *finish_buff() {
  if (result_buff_len) {
    flush_buff();
    return result_buff;
  } else {
    return buff;
  }
}

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#define min(x, y) ((x) < (y) ? (x) : (y))

void write_buff(const char *s, int l) {
  while (l > 0) {
    if (unlikely (wptr == buff + BUFF_LEN)) {
      flush_buff();
    }
    int ll = min (l, buff + BUFF_LEN - wptr);
    memcpy(wptr, s, ll);
    wptr += ll;
    s += ll;
    l -= ll;
  }

}

int write_buff_get_pos() {
  return cur_buff_len;
}

void write_buff_set_pos(int pos) {
  if (pos > cur_buff_len) {
    return;
  }
  if (pos >= result_buff_pos) {
    wptr = (pos - result_buff_pos) + buff;
    return;
  }
  //result_buff_pos -= (pos - (wptr - buff));
  result_buff_pos = pos;
  wptr = buff;
}

void write_buff_char_pos(char c, int pos) {
  if (pos > cur_buff_len) {
    return;
  }
  if (pos >= result_buff_pos) {
    *((pos - result_buff_pos) + buff) = c;
    return;
  }
  //*(result_buff + result_buff_pos - (pos - (wptr - buff))) = c;
  *(result_buff + pos) = c;
}


void write_buff_char(char c) {
  if (unlikely (wptr == buff + BUFF_LEN)) {
    flush_buff();
  }
  *(wptr++) = c;
}

void write_buff_char_2(char c1, char c2) {
  if (unlikely (wptr >= buff + BUFF_LEN - 1)) {
    flush_buff();
  }
  *(wptr++) = c1;
  *(wptr++) = c2;
}

void write_buff_char_3(char c1, char c2, char c3) {
  if (unlikely (wptr >= buff + BUFF_LEN - 2)) {
    flush_buff();
  }
  *(wptr++) = c1;
  *(wptr++) = c2;
  *(wptr++) = c3;
}

void write_buff_char_4(char c1, char c2, char c3, char c4) {
  if (unlikely (wptr >= buff + BUFF_LEN - 3)) {
    flush_buff();
  }
  *(wptr++) = c1;
  *(wptr++) = c2;
  *(wptr++) = c3;
  *(wptr++) = c4;
}

inline void write_buff_char_5(char c1, char c2, char c3, char c4, char c5) {
  if (unlikely (wptr >= buff + BUFF_LEN - 4)) {
    flush_buff();
  }
  *wptr++ = c1;
  *wptr++ = c2;
  *wptr++ = c3;
  *wptr++ = c4;
  *wptr++ = c5;
}

inline void write_buff_char_6(char c1, char c2, char c3, char c4, char c5, char c6) {
  if (unlikely (wptr >= buff + BUFF_LEN - 5)) {
    flush_buff();
  }
  *wptr++ = c1;
  *wptr++ = c2;
  *wptr++ = c3;
  *wptr++ = c4;
  *wptr++ = c5;
  *wptr++ = c6;
}

void write_buff_long(long x) {
  if (unlikely (wptr + 25 > buff + BUFF_LEN)) {
    flush_buff();
  }
  wptr += snprintf(wptr, 25, "%ld", x);
}

void write_buff_double(double x) {
  if (unlikely (wptr + 100 > buff + BUFF_LEN)) {
    flush_buff();
  }
  wptr += snprintf(wptr, 100, "%f", x);
}

int utf8_to_win(const char *s, int len, int max_len, int exit_on_error) {
  int st = 0;
  int acc = 0;
  int i;
  if (max_len && len > 3 * max_len) {
    len = 3 * max_len;
  }
  for (i = 0; i < len; i++) {
    if (max_len && cur_buff_len > max_len) {
      break;
    }
    unsigned char c = s[i];
    if (c < 0x80) {
      if (st) {
        if (exit_on_error) {
          return -1;
        }
        write_buff("?1?", 3);
      }
      write_buff_char(c);
      st = 0;
      continue;
    }
    if ((c & 0xc0) == 0x80) {
      if (!st) {
        if (exit_on_error) {
          return -1;
        }
        write_buff("?2?", 3);
        continue;
      }
      acc <<= 6;
      acc += c - 0x80;
      if (!--st) {
        if (exit_on_error && acc < 0x80) {
          return -1;
        }
        if (acc < 0x80) {
          write_buff("?3?", 3);
        } else {
          int d = utf8_to_win_char(acc);
          if (d != -1 && d) {
            write_buff_char(d);
          } else {
            write_buff_char('&');
            write_buff_char('#');
            write_buff_long(acc);
            write_buff_char(';');
          }
        }
      }
      continue;
    }
    if ((c & 0xc0) == 0xc0) {
      if (st) {
        if (exit_on_error) {
          return -1;
        }
        write_buff("?4?", 3);
      }
      c -= 0xc0;
      st = 0;
      if (c < 32) {
        acc = c;
        st = 1;
      } else if (c < 48) {
        acc = c - 32;
        st = 2;
      } else if (c < 56) {
        acc = c - 48;
        st = 3;
      } else {
        if (exit_on_error) {
          return -1;
        }
        write_buff("?5?", 3);
      }
    }
  }
  if (st) {
    if (exit_on_error) {
      return -1;
    }
    write_buff("?6?", 3);
  }
  return 1;
}

void write_char_utf8(int64_t c) {
  if (!c) {
    return;
  }
  if (c < 128) {
    write_buff_char(c);
    return;
  }
  // 2 bytes(11): 110x xxxx 10xx xxxx
  if (c < 0x800) {
    write_buff_char_2((char)(0xC0 + (c >> 6)), (char)(0x80 + (c & 63)));
    return;
  }

  // 3 bytes(16): 1110 xxxx 10xx xxxx 10xx xxxx
  if (c < 0x10000) {
    write_buff_char_3((char)(0xE0 + (c >> 12)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }

  // 4 bytes(21): 1111 0xxx 10xx xxxx 10xx xxxx 10xx xxxx
  if (c < 0x200000) {
    write_buff_char_4((char)(0xF0 + (c >> 18)), (char)(0x80 + ((c >> 12) & 63)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }

  // 5 bytes(26): 1111 10xx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
  if (c < 0x4000000) {
    write_buff_char_5((char)(0xF8 + (c >> 24)), (char)(0x80 + ((c >> 18) & 63)), (char)(0x80 + ((c >> 12) & 63)), (char)(0x80 + ((c >> 6) & 63)),
                      (char)(0x80 + (c & 63)));
    return;
  }

  // 6 bytes(31): 1111 110x 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
  if (c < 0x80000000) {
    write_buff_char_6((char)(0xFC + (c >> 30)), (char)(0x80 + ((c >> 24) & 63)), (char)(0x80 + ((c >> 18) & 63)), (char)(0x80 + ((c >> 12) & 63)),
                      (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }

  write_buff_char_2('$', '#');
  write_buff_long(c);
  write_buff_char(';');
}

void write_char_utf8_no_escape(int64_t c) {
  if (!c) {
    return;
  }
  if (c < 128) {
    write_buff_char((char)c);
    return;
  }
  // 2 bytes(11): 110x xxxx 10xx xxxx
  if (c < 0x800) {
    write_buff_char_2((char)(0xC0 + (c >> 6)), (char)(0x80 + (c & 63)));
    return;
  }

  // 3 bytes(16): 1110 xxxx 10xx xxxx 10xx xxxx
  if (c < 0x10000) {
    write_buff_char_3((char)(0xE0 + (c >> 12)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }

  // 4 bytes(21): 1111 0xxx 10xx xxxx 10xx xxxx 10xx xxxx
  if (c < 0x200000) {
    write_buff_char_4((char)(0xF0 + (c >> 18)), (char)(0x80 + ((c >> 12) & 63)), (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }

  // 5 bytes(26): 1111 10xx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
  if (c < 0x4000000) {
    write_buff_char_5((char)(0xF8 + (c >> 24)), (char)(0x80 + ((c >> 18) & 63)), (char)(0x80 + ((c >> 12) & 63)), (char)(0x80 + ((c >> 6) & 63)),
                      (char)(0x80 + (c & 63)));
    return;
  }

  // 6 bytes(31): 1111 110x 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx 10xx xxxx
  if (c < 0x80000000) {
    write_buff_char_6((char)(0xFC + (c >> 30)), (char)(0x80 + ((c >> 24) & 63)), (char)(0x80 + ((c >> 18) & 63)), (char)(0x80 + ((c >> 12) & 63)),
                      (char)(0x80 + ((c >> 6) & 63)), (char)(0x80 + (c & 63)));
    return;
  }
}

int win_to_utf8(const char *s, int len, bool escape) {
  int state = 0;
  int save_pos = -1;
  int64_t cur_num = 0;
  for (int i = 0; i < len; i++) {
    if (state == 0 && s[i] == '&') {
      save_pos = write_buff_get_pos();
      cur_num = 0;
      state++;
    } else if (state == 1 && s[i] == '#') {
      state++;
    } else if (state == 2 && s[i] >= '0' && s[i] <= '9') {
      if (cur_num < 0x80000000) {
        cur_num = s[i] - '0' + cur_num * 10;
      }
    } else if (state == 2 && s[i] == ';') {
      state++;
    } else {
      state = 0;
    }
    if (state == 3 && 0xd800 <= cur_num && cur_num <= 0xdfff) {
      cur_num = 32;
    }
    if (state == 3 && (!escape || (cur_num >= 32 && cur_num != 33 && cur_num != 34 && cur_num != 36 && cur_num != 39 && cur_num != 60 && cur_num != 62 && cur_num != 92 && cur_num != 8232 && cur_num != 8233 && cur_num < 0x80000000))) {
      write_buff_set_pos(save_pos);
      assert (save_pos == write_buff_get_pos());
      (escape ? write_char_utf8 : write_char_utf8_no_escape)(cur_num);
    } else if (state == 3 && cur_num >= 0x80000000) {
      write_char_utf8(win_to_utf8_convert[(unsigned char)(s[i])]);
      write_buff_char_pos('$', save_pos);
    } else {
      write_char_utf8(win_to_utf8_convert[(unsigned char)(s[i])]);
    }
    if (state == 3) {
      state = 0;
    }
  }
  return cur_buff_len;
}

void print_backtrace() {
  void *buffer[64];
  int nptrs = backtrace(buffer, 64);
  write(2, "\n------- Stack Backtrace -------\n", 33);
  backtrace_symbols_fd(buffer, nptrs, 2);
  write(2, "-------------------------------\n", 32);
}

void sigsegv_debug_handler(const int sig) {
  write(2, "SIGSEGV caught, terminating program\n", 36);
  print_backtrace();
  exit(EXIT_FAILURE);
}

void sigabrt_debug_handler(const int sig) {
  write(2, "SIGABRT caught, terminating program\n", 36);
  print_backtrace();
  exit(EXIT_FAILURE);
}

void set_debug_handlers() {
  signal(SIGSEGV, sigsegv_debug_handler);
  signal(SIGABRT, sigabrt_debug_handler);
}

PHP_MINIT_FUNCTION (vkext) {
  set_debug_handlers();
  REGISTER_INI_ENTRIES();
  rpc_on_minit(module_number);

  init_version_string("vkext");
  return SUCCESS;
}

PHP_INI_MH (on_change_conffile) {
  if (!new_value) {
    return FAILURE;
  }
  int x = read_tl_config(VK_ZSTR_VAL (new_value));
  return x < 0 ? FAILURE : SUCCESS;
}

PHP_RINIT_FUNCTION (vkext) {
  check_reload_tl_config();
  rpc_on_rinit(module_number);
  return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION (vkext) {
  UNREGISTER_INI_ENTRIES();
  return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION (vkext) {
  rpc_on_rshutdown(module_number);
  return SUCCESS;
}

PHP_FUNCTION (vk_utf8_to_win) {
  char *text;
  VK_LEN_T text_len = 0;
  long max_len = 0;
  long exit_on_error = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|lb", &text, &text_len, &max_len, &exit_on_error) == FAILURE) {
    return;
  }
  init_buff(0);
  int r = utf8_to_win(text, text_len, max_len, exit_on_error);
  write_buff_char(0);
  char *res = finish_buff();
  if (max_len && cur_buff_len > max_len) {
    res[max_len] = 0;
  }
  res = r >= 0 ? res : text;
  VK_RETVAL_STRING_DUP (res);
  free_buff();
}


PHP_FUNCTION (vk_win_to_utf8) {
  char *text;
  long text_len = 0;
  bool escape = true;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|b", &text, &text_len, &escape) == FAILURE) {
    return;
  }
  init_buff(0);
  win_to_utf8(text, text_len, escape);
  write_buff_char(0);
  char *res = finish_buff();
  char *new_res = estrdup (res);
  free_buff();
  VK_RETURN_STRING_NOD (new_res);
}

PHP_FUNCTION (vk_upcase) {
  char *text;
  VK_LEN_T text_length = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &text, &text_length) == FAILURE) {
    return;
  }
  char *new_text = estrdup (text);
  for (VK_LEN_T i = 0; i < text_length; i++) {
    new_text[i] = upcase_char(new_text[i]);
  }
  VK_RETURN_STRING_NOD (new_text);
}

PHP_FUNCTION (vk_flex) {
  char *name;
  long name_len = 0;
  char *case_name;
  long case_name_len = 0;
  long sex = 0;
  char *type;
  long type_len = 0;
  long lang_id = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "sslsl", &name, &name_len, &case_name, &case_name_len, &sex, &type, &type_len, &lang_id) == FAILURE) {
    return;
  }
  if (verbosity) {
    fprintf(stderr, "name = %s, name_len = %ld\n", name, name_len);
    fprintf(stderr, "case_name = %s, case_name_len = %ld\n", case_name, case_name_len);
    fprintf(stderr, "sex = %ld\n", sex);
    fprintf(stderr, "type = %s, type_len = %ld\n", type, type_len);
    fprintf(stderr, "lang_id = %ld\n", lang_id);
  }
  char *res = do_flex(name, name_len, case_name, case_name_len, sex == 1, type, type_len, lang_id);
  VK_RETURN_STRING_NOD (res);
}

PHP_FUNCTION (vk_json_encode) {
  zval *parameter;

  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &parameter) == FAILURE) {
    RETURN_FALSE;
  }

  init_buff(0);
  if (!vk_json_encode(parameter)) {
    free_buff();
    RETURN_FALSE;
  }

  write_buff_char(0);
  char *res = finish_buff();
  char *new_res = estrdup (res);
  free_buff();
  VK_RETURN_STRING_NOD (new_res);
}

char ws[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
//char lb[256] = {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static inline int is_html_opt_symb(char c) {
  return (c == '<' || c == '>' || c == '(' || c == ')' || c == '{' || c == '}' || c == '/' || c == '"' || c == ':' || c == ',' || c == ';');
}

static inline int is_space(char c) {
  return ws[(unsigned char)c];
  //return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

static inline int is_linebreak(char c) {
  //return lb[(unsigned char)c];
  return c == '\n';
}

static inline int is_pre_tag(const char *s) {
  if (s[1] == 'p') {
    return s[2] == 'r' && s[3] == 'e' && s[4] == '>';
  } else if (s[1] == 'c') {
    return s[2] == 'o' && s[3] == 'd' && s[4] == 'e' && s[5] == '>';
  } else if (s[1] == '/') {
    if (s[1] == '/') {
      return -(s[3] == 'r' && s[4] == 'e' && s[5] == '>');
    } else {
      return -(s[3] == 'o' && s[4] == 'd' && s[5] == 'e' && s[6] == '>');
    }
  }
  /*if (*(int *)s == *(int *)"<pre" || *(int *)(s + 1) == *(int *)"code") {
    return 1;
  }
  if (*(int *)(s + 1) == *(int *)"/pre" || (s[1] == '/' && *(int *)(s + 2) == *(int *)"code")) {
    return -1;
  }*/
  /*if (!strncmp (s, "<pre>", 5) || !strncmp (s, "<code>", 6)) {
    return 1;
  }
  if (!strncmp (s, "</pre>", 6) || !strncmp (s, "</code>", 7)) {
    return -1;
  }*/
  return 0;
}

PHP_FUNCTION (vk_whitespace_pack) {
  char *text, *ctext;
  VK_LEN_T text_length = 0;
  long html_opt = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|l", &text, &text_length, &html_opt) == FAILURE) {
    return;
  }
  init_buff(text_length);
  //init_buff (0);
  int level = 0;
  //int i = 0;
  ctext = text;
  char *start = text;
  //fprintf (stderr, "%lf\n", get_double_time_since_epoch ());
  //const char *ctext = text;
  while (*text) {
    if (is_space(*text) && !level) {
      int linebreak = 0;
      while (is_space(*text) && *text) {
        if (is_linebreak(*text)) {
          linebreak = 1;
        }
        text++;
      }
      if (!html_opt || ((ctext != start && !is_html_opt_symb(*(ctext - 1))) && (*text && !is_html_opt_symb(*text)))) {
        //if (0) {
        write_buff_char(linebreak ? '\n' : ' ');
      }
      ctext = text;
    } else {
      while (1) {
        while ((level || !is_space(*text)) && *text) {
          if (*text == '<') {
            level += is_pre_tag(text);
          }
          if (level < 0) {
            level = 1000000000;
          }
          text++;
        }
        if (!html_opt && *text && !is_space(*(text + 1))) {
          text++;
          continue;
        }
        break;
      }
      write_buff(ctext, text - ctext);
    }
    ctext = text;
  }
  //fprintf (stderr, "\n");
  write_buff_char(0);
  //fprintf (stderr, "%lf\n", get_double_time_since_epoch ());
  char *res = finish_buff();
  //fprintf (stderr, "%lf\n", get_double_time_since_epoch ());
  char *new_res = estrdup (res);
  //fprintf (stderr, "%lf\n", get_double_time_since_epoch ());
  VK_RETVAL_STRING_NOD(new_res);
  free_buff();
  //fprintf (stderr, "%lf\n", get_double_time_since_epoch ());
}


PHP_FUNCTION (vk_hello_world) {
  char *s = static_cast<char *>(emalloc (4));
  s[0] = 'A';
  s[1] = 0;
  s[2] = 'A';
  s[3] = 0;
  VK_RETURN_STRINGL_NOD(s, 4);
}

PHP_FUNCTION (vkext_full_version) {
  ADD_CNT(total);
  START_TIMER(total);
  VK_RETVAL_STRING_DUP(get_version_string());
  END_TIMER(total);
}

PHP_FUNCTION (new_rpc_connection) {
  ADD_CNT(total);
  START_TIMER(total);
  php_new_rpc_connection(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_clean) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_clean(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_send) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_send(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_send_noflush) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_send_noflush(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_flush) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_flush(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_get_and_parse) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_get_and_parse(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_get) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_get(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_parse) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_parse(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (set_fail_rpc_on_int32_overflow) {
  ADD_CNT(total);
  START_TIMER(total);
  set_fail_rpc_on_int32_overflow(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_int) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_int(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_long) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_long(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_string) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_string(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_double) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_double(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_float) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_float(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_many) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_many(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (store_header) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_store_header(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_int) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_int(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_lookup_int) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_lookup_int(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_lookup_data) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_lookup_data(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_memcache_value) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_memcache_value(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}


PHP_FUNCTION (fetch_long) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_long(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_double) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_double(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_float) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_float(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_string) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_string(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_end) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_end(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (fetch_eof) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_fetch_end(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_queue_create) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_queue_create(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_queue_empty) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_queue_empty(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_queue_next) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_queue_next(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_queue_push) {
  ADD_CNT(total);
  START_TIMER(total);
  php_rpc_queue_push(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (vk_clear_stats) {
  memset(&stats, 0, offsetof (struct stats, malloc));
}

PHP_FUNCTION (rpc_tl_pending_queries_count) {
  ADD_CNT(total);
  START_TIMER(total);
  vkext_reset_error();
  RETURN_LONG(vk_queries_count(INTERNAL_FUNCTION_PARAM_PASSTHRU));
  END_TIMER(total);
}

PHP_FUNCTION (rpc_tl_query) {
  ADD_CNT(total);
  START_TIMER(total);
  vkext_reset_error();
  vk_rpc_tl_query(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_tl_query_one) {
  ADD_CNT(total);
  START_TIMER(total);
  vkext_reset_error();
  vk_rpc_tl_query_one(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (typed_rpc_tl_query) {
  ADD_CNT(total);
  START_TIMER(total);
  vkext_reset_error();
  typed_mode = 1;
  vk_rpc_tl_query(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  typed_mode = 0;
  tl_current_function_name = NULL;
  END_TIMER(total);
}

PHP_FUNCTION (typed_rpc_tl_query_one) {
  ADD_CNT(total);
  START_TIMER(total);
  vkext_reset_error();
  typed_mode = 1;
  vk_rpc_tl_query_one(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  typed_mode = 0;
  tl_current_function_name = NULL;
  END_TIMER(total);
}

PHP_FUNCTION (rpc_tl_query_result) {
  ADD_CNT(total);
  START_TIMER(total);
  vk_rpc_tl_query_result(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (rpc_tl_query_result_one) {
  ADD_CNT(total);
  START_TIMER(total);
  vk_rpc_tl_query_result_one(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (typed_rpc_tl_query_result_one) {
  ADD_CNT(total);
  START_TIMER(total);
  typed_mode = 1;
  vk_rpc_tl_query_result_one(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  typed_mode = 0;
  tl_current_function_name = NULL;
  END_TIMER(total);
}

PHP_FUNCTION (typed_rpc_tl_query_result) {
  ADD_CNT(total);
  START_TIMER(total);
  typed_mode = 1;
  vk_rpc_tl_query_result(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  typed_mode = 0;
  tl_current_function_name = NULL;
  END_TIMER(total);
}

PHP_FUNCTION (rpc_get_last_send_error) {
  ADD_CNT(total);
  START_TIMER(total);
  vkext_get_errors(return_value);
  END_TIMER(total);
}

extern long error_verbosity;

PHP_FUNCTION (vk_set_error_verbosity) {
  ADD_CNT(total);
  START_TIMER(total);
  long t;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &t) == FAILURE) {
    END_TIMER(total);
    return;
  }
  error_verbosity = t;
  END_TIMER(total);
  RETURN_TRUE;
}

PHP_FUNCTION (tl_config_load_file) {
  ADD_CNT(total);
  START_TIMER(total);
  php_tl_config_load_file(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (vkext_prepare_stats) {
  ADD_CNT(total);
  START_TIMER(total);
  php_vk_prepare_stats(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (enable_internal_rpc_queries) {
  ADD_CNT(total);
  START_TIMER(total);
  enable_internal_rpc_queries(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}

PHP_FUNCTION (disable_internal_rpc_queries) {
  ADD_CNT(total);
  START_TIMER(total);
  disable_internal_rpc_queries(INTERNAL_FUNCTION_PARAM_PASSTHRU);
  END_TIMER(total);
}
