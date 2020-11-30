// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/misc.h"

#include <errno.h>
#include <iconv.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "runtime/critical_section.h"
#include "runtime/datetime.h"
#include "runtime/exception.h"
#include "runtime/files.h"
#include "runtime/interface.h"
#include "runtime/math_functions.h"
#include "runtime/string_functions.h"
#include "runtime/vkext.h"
#include "server/json-logger.h"
#include "server/php-engine-vars.h"

string f$uniqid(const string &prefix, bool more_entropy) {
  if (!more_entropy) {
    f$usleep(1);
  }

  dl::enter_critical_section();//OK

  struct timeval tv;
  gettimeofday(&tv, nullptr);
  int sec = (int)tv.tv_sec;
  int usec = (int)(tv.tv_usec & 0xFFFFF);

  char buf[30];
  static_SB.clean() << prefix;

  if (more_entropy) {
    sprintf(buf, "%08x%05x%.8f", sec, usec, f$lcg_value() * 10);
    static_SB.append(buf, 23);
  } else {
    sprintf(buf, "%08x%05x", sec, usec);
    static_SB.append(buf, 13);
  }

  dl::leave_critical_section();
  return static_SB.str();
}


Optional<string> f$iconv(const string &input_encoding, const string &output_encoding, const string &input_str) {
  iconv_t cd;
  if ((cd = iconv_open(output_encoding.c_str(), input_encoding.c_str())) == (iconv_t)-1) {
    php_warning("unsupported iconv from \"%s\" to \"%s\"", input_encoding.c_str(), output_encoding.c_str());
    return false;
  }

  for (int mul = 4; mul <= 16; mul *= 4) {
    string output_str(mul * input_str.size(), false);

    size_t input_len = input_str.size();
    size_t output_len = output_str.size();
    char *input_buf = const_cast <char *> (input_str.c_str());
    char *output_buf = output_str.buffer();

    size_t res = iconv(cd, &input_buf, &input_len, &output_buf, &output_len);
    if (res != (size_t)-1 || errno != E2BIG) {
      output_str.shrink(static_cast<string::size_type>(output_buf - output_str.c_str()));
      iconv_close(cd);
      return output_str;
    }
/*
    if (errno != E2BIG) {
      php_warning ("Error in iconv from \"%s\" to \"%s\" string \"%s\" at character %d at pos %d: %m", input_encoding.c_str(), output_encoding.c_str(), input_str.c_str(), (int)*input_buf, (int)(input_buf - input_str.c_str()));
      break;
    }
*/
    iconv(cd, nullptr, nullptr, nullptr, nullptr);
  }

  iconv_close(cd);
  return false;
}


void f$sleep(int64_t seconds) {
  if (seconds <= 0 || seconds > 1800) {
    php_warning("Wrong parameter seconds (%ld) specified in function sleep, must be in seconds", seconds);
    return;
  }

  f$usleep(seconds * 1000000);
}

void f$usleep(int64_t micro_seconds) {
  int64_t sleep_time = micro_seconds;
  if (sleep_time <= 0) {
    php_warning("Wrong parameter micro_seconds (%ld) specified in function usleep", sleep_time);
    return;
  }

  struct itimerval timer, old_timer;
  memset(&timer, 0, sizeof(timer));
  dl::enter_critical_section();//OK
  setitimer(ITIMER_REAL, &timer, &old_timer);
  dl::leave_critical_section();
  long long time_left = old_timer.it_value.tv_sec * 1000000ll + old_timer.it_value.tv_usec;

//  fprintf (stderr, "time_left = %lld, sleep_time = %d\n", time_left, sleep_time);
  if (time_left == 0) {
    dl::enter_critical_section();//OK
    usleep(static_cast<uint32_t>(sleep_time));
    dl::leave_critical_section();
    return;
  }

  if (time_left > sleep_time) {
    double start_time = microtime_monotonic();

    dl::enter_critical_section();//OK
    usleep(static_cast<uint32_t>(sleep_time));
    dl::leave_critical_section();

    time_left -= static_cast<int64_t>((microtime_monotonic() - start_time) * 1000000);
  } else {
    time_left = 1;
  }

  if (time_left <= 1) {
//    raise (SIGALRM);
//    return;
    time_left = 1;
  }

  timer.it_value.tv_sec = time_left / 1000000;
  timer.it_value.tv_usec = time_left % 1000000;
  dl::enter_critical_section();//OK
  setitimer(ITIMER_REAL, &timer, nullptr);
  dl::leave_critical_section();
}


static const char php_sig_gif[3] = {'G', 'I', 'F'};
static const char php_sig_jpg[3] = {(char)0xff, (char)0xd8, (char)0xff};
static const char php_sig_png[8] = {(char)0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
static const char php_sig_jpc[4] = {(char)0xff, 0x4f, (char)0xff, 0x51};
static const char php_sig_jp2[12] = {0x00, 0x00, 0x00, 0x0c, 'j', 'P', ' ', ' ', 0x0d, 0x0a, (char)0x87, 0x0a};

static const char *mime_type_string[11] = {
  "",
  "image/gif",
  "image/jpeg",
  "image/png",
  "application/x-shockwave-flash",
  "image/psd",
  "image/x-ms-bmp",
  "image/tiff",
  "image/tiff",
  "application/octet-stream",
  "image/jp2"
};


static const int M_SOF0 = 0xC0;
static const int M_SOF1 = 0xC1;
static const int M_SOF2 = 0xC2;
static const int M_SOF3 = 0xC3;
static const int M_SOF5 = 0xC5;
static const int M_SOF6 = 0xC6;
static const int M_SOF7 = 0xC7;
static const int M_SOF9 = 0xC9;
static const int M_SOF10 = 0xCA;
static const int M_SOF11 = 0xCB;
static const int M_SOF13 = 0xCD;
static const int M_SOF14 = 0xCE;
static const int M_SOF15 = 0xCF;
static const int M_EOI = 0xD9;
static const int M_SOS = 0xDA;
static const int M_COM = 0xFE;

static const int M_PSEUDO = 0xFFD8;

mixed f$getimagesize(const string &name) {
  dl::enter_critical_section();//OK
  struct stat stat_buf;
  int read_fd = open(name.c_str(), O_RDONLY);
  if (read_fd < 0) {
    dl::leave_critical_section();
    return false;
  }
  if (fstat(read_fd, &stat_buf) < 0) {
    close(read_fd);
    dl::leave_critical_section();
    return false;
  }

  if (!S_ISREG (stat_buf.st_mode)) {
    close(read_fd);
    dl::leave_critical_section();
    php_warning("Regular file expected as first argument in function getimagesize, \"%s\" is given", name.c_str());
    return false;
  }

  const size_t min_size = 3 * 256 + 64;
  unsigned char buf[min_size];
  size_t size = stat_buf.st_size, read_size = min_size;
  if (size < min_size) {
    read_size = size;
  }

  if (read_size < 12 || read_safe(read_fd, buf, read_size) < (ssize_t)read_size) {
    close(read_fd);
    dl::leave_critical_section();
    return false;
  }

  int width = 0, height = 0, bits = 0, channels = 0, type = IMAGETYPE_UNKNOWN;
  switch (buf[0]) {
    case 'G': //gif
      if (!strncmp((const char *)buf, php_sig_gif, sizeof(php_sig_gif))) {
        type = IMAGETYPE_GIF;
        width = buf[6] | (buf[7] << 8);
        height = buf[8] | (buf[9] << 8);
        bits = buf[10] & 0x80 ? (buf[10] & 0x07) + 1 : 0;
        channels = 3;
      } else {
        close(read_fd);
        dl::leave_critical_section();
        return false;
      }
      break;
    case 0xff: //jpg or jpc
      if (!strncmp((const char *)buf, php_sig_jpg, sizeof(php_sig_jpg))) {
        type = IMAGETYPE_JPEG;

        unsigned char *image = (unsigned char *)dl::allocate(size);
        if (image == nullptr) {
          php_warning("Not enough memory to process file \"%s\" in getimagesize", name.c_str());
          close(read_fd);
          dl::leave_critical_section();
          return false;
        }
        memcpy(image, buf, read_size);
        if (read_safe(read_fd, image + read_size, size - read_size) < (ssize_t)(size - read_size)) {
          dl::deallocate(image, size);
          close(read_fd);
          dl::leave_critical_section();
          return false;
        }

        int marker = M_PSEUDO;
        size_t cur_pos = 2;

        while (height == 0 && width == 0 && marker != M_SOS && marker != M_EOI) {
          int a = 0, comment_correction = 1 + (marker == M_COM), new_marker;

          do {
            if (cur_pos == size) {
              new_marker = M_EOI;
              break;
            }
            new_marker = image[cur_pos++];
            if (marker == M_COM && comment_correction > 0) {
              if (new_marker != 0xFF) {
                new_marker = 0xFF;
                comment_correction--;
              } else {
                marker = M_PSEUDO;
              }
            }
            a++;
          } while (new_marker == 0xff);

          if (a < 2 || (marker == M_COM && comment_correction)) {
            new_marker = M_EOI;
          }

          marker = new_marker;

          switch (marker) {
            case M_SOF0:
            case M_SOF1:
            case M_SOF2:
            case M_SOF3:
            case M_SOF5:
            case M_SOF6:
            case M_SOF7:
            case M_SOF9:
            case M_SOF10:
            case M_SOF11:
            case M_SOF13:
            case M_SOF14:
            case M_SOF15:
              if (cur_pos + 8 > size) {
                dl::deallocate(image, size);
                close(read_fd);
                dl::leave_critical_section();
                return false;
              }
              bits = image[cur_pos + 2];
              height = (image[cur_pos + 3] << 8) + image[cur_pos + 4];
              width = (image[cur_pos + 5] << 8) + image[cur_pos + 6];
              channels = image[cur_pos + 7];
              cur_pos += 8;

            case M_SOS:
            case M_EOI:
              break;

            default: {
              size_t length = (image[cur_pos] << 8) + image[cur_pos + 1];

              if (length < 2 || cur_pos + length > size) {
                dl::deallocate(image, size);
                close(read_fd);
                dl::leave_critical_section();
                return false;
              }
              cur_pos += length;
              break;
            }
          }
        }
        dl::deallocate(image, size);
      } else if (!strncmp((const char *)buf, php_sig_jpc, sizeof(php_sig_jpc)) && (int)read_size >= 42) {
        type = IMAGETYPE_JPEG;

        width = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11];
        height = (buf[12] << 24) + (buf[13] << 16) + (buf[14] << 8) + buf[15];
        channels = (buf[40] << 8) + buf[41];

        if (channels < 0 || channels > 256 || (int)read_size < 42 + 3 * channels || width <= 0 || height <= 0) {
          close(read_fd);
          dl::leave_critical_section();
          return false;
        }

        bits = 0;
        for (int i = 0; i < channels; i++) {
          int cur_bits = buf[42 + 3 * i];
          if (cur_bits > bits) {
            bits = cur_bits;
          }
        }
        bits++;
      } else {
        close(read_fd);
        dl::leave_critical_section();
        return false;
      }
      break;
    case 0x00: //jp2
      if (read_size >= 54 && !strncmp((const char *)buf, php_sig_jp2, sizeof(php_sig_jp2))) {
        type = IMAGETYPE_JP2;

        bool found = false;

        int buf_pos = 12;
        size_t file_pos = 12;
        while ((int)read_size >= 42 + buf_pos + 8) {
          const unsigned char *s = buf + buf_pos;
          int box_length = (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3];
          if (box_length == 1 || box_length > 1000000000) {
            break;
          }
          if (s[4] == 'j' && s[5] == 'p' && s[6] == '2' && s[7] == 'c') {
            s += 8;

            width = (s[8] << 24) + (s[9] << 16) + (s[10] << 8) + s[11];
            height = (s[12] << 24) + (s[13] << 16) + (s[14] << 8) + s[15];
            channels = (s[40] << 8) + s[41];

            if (channels < 0 || channels > 256 || (int)read_size < 42 + buf_pos + 8 + 3 * channels || width <= 0 || height <= 0) {
              break;
            }

            bits = 0;
            for (int i = 0; i < channels; i++) {
              int cur_bits = s[42 + 3 * i];
              if (cur_bits > bits) {
                bits = cur_bits;
              }
            }
            bits++;

            found = true;
            break;
          }

          if (box_length <= 8) {
            break;
          }
          file_pos += box_length;
          if (file_pos >= size || (off_t)file_pos != (ssize_t)file_pos || (ssize_t)file_pos < 0) {
            break;
          }

          read_size = min_size;
          if (size - file_pos < min_size) {
            read_size = size - file_pos;
          }

          if (read_size < 50 || pread(read_fd, buf, read_size, (off_t)file_pos) < (ssize_t)read_size) {
            break;
          }

          buf_pos = 0;
        }

        if (!found) {
          close(read_fd);
          dl::leave_critical_section();
          return false;
        }
      } else {
        close(read_fd);
        dl::leave_critical_section();
        return false;
      }
      break;
    case 0x89: //png
      if (read_size >= 25 && !strncmp((const char *)buf, php_sig_png, sizeof(php_sig_png))) {
        type = IMAGETYPE_PNG;
        width = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + buf[19];
        height = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + buf[23];
        bits = buf[24];
      } else {
        close(read_fd);
        dl::leave_critical_section();
        return false;
      }
      break;
    default:
      close(read_fd);
      dl::leave_critical_section();
      return false;
  }
  close(read_fd);
  dl::leave_critical_section();

  array<mixed> result(array_size(4, 3, false));
  result.push_back(width);
  result.push_back(height);
  result.push_back(type);
  int len = sprintf((char *)buf, "width=\"%d\" height=\"%d\"", width, height);
  result.push_back(string((const char *)buf, len));
  if (bits != 0) {
    result.set_value(string("bits", 4), bits);
  }
  if (channels != 0) {
    result.set_value(string("channels", 8), channels);
  }
  result.set_value(string("mime", 4), string(mime_type_string[type]));

  return result;
}


int64_t f$posix_getpid() {
  dl::enter_critical_section();//OK
  auto result = getpid();
  dl::leave_critical_section();
  return result;
}

int64_t f$posix_getuid() {
  dl::enter_critical_section();//OK
  auto result = getuid();
  dl::leave_critical_section();
  return result;
}

Optional<array<mixed>> f$posix_getpwuid(int64_t uid) {
  dl::enter_critical_section();//OK
  passwd *pwd = getpwuid(static_cast<uint32_t>(uid));
  dl::leave_critical_section();
  if (!pwd) {
    return false;
  }
  array<mixed> result(array_size(0, 7, false));
  result.set_value(string("name", 4), string(pwd->pw_name));
  result.set_value(string("passwd", 6), string(pwd->pw_passwd));
  result.set_value(string("uid", 3), static_cast<int64_t>(pwd->pw_uid));
  result.set_value(string("gid", 3), static_cast<int64_t>(pwd->pw_gid));
  result.set_value(string("gecos", 5), string(pwd->pw_gecos));
  result.set_value(string("dir", 3), string(pwd->pw_dir));
  result.set_value(string("shell", 5), string(pwd->pw_shell));
  return result;
}


static inline void do_serialize(bool b) {
  static_SB.reserve(4);
  static_SB.append_char('b');
  static_SB.append_char(':');
  static_SB.append_char(static_cast<char>(b + '0'));
  static_SB.append_char(';');
}

static inline void do_serialize(int64_t i) {
  static_SB.reserve(24);
  static_SB.append_char('i');
  static_SB.append_char(':');
  static_SB << i;
  static_SB.append_char(';');
}

static inline void do_serialize(double f) {
  static_SB.append("d:", 2);
  static_SB << f << ';';
}

static inline void do_serialize(const string &s) {
  string::size_type len = s.size();
  static_SB.reserve(25 + len);
  static_SB.append_char('s');
  static_SB.append_char(':');
  static_SB << len;
  static_SB.append_char(':');
  static_SB.append_char('"');
  static_SB.append_unsafe(s.c_str(), len);
  static_SB.append_char('"');
  static_SB.append_char(';');
}

void do_serialize(const mixed &v) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      static_SB.reserve(2);
      static_SB.append_char('N');
      static_SB.append_char(';');
      return;
    case mixed::type::BOOLEAN:
      return do_serialize(v.as_bool());
    case mixed::type::INTEGER:
      return do_serialize(v.as_int());
    case mixed::type::FLOAT:
      return do_serialize(v.as_double());
    case mixed::type::STRING:
      return do_serialize(v.as_string());
    case mixed::type::ARRAY: {
      static_SB.append("a:", 2);
      static_SB << v.as_array().count();
      static_SB.append(":{", 2);
      for (array<mixed>::const_iterator p = v.as_array().begin(); p != v.as_array().end(); ++p) {
        const array<mixed>::key_type &key = p.get_key();
        if (array<mixed>::is_int_key(key)) {
          do_serialize(key.to_int());
        } else {
          do_serialize(key.to_string());
        }
        do_serialize(p.get_value());
      }
      static_SB << '}';
      return;
    }
    default:
      __builtin_unreachable();
  }
}

string f$serialize(const mixed &v) {
  static_SB.clean();

  do_serialize(v);

  return static_SB.str();
}

static int do_unserialize(const char *s, int s_len, mixed &out_var_value) {
  if (!out_var_value.is_null()) {
    out_var_value = mixed{};
  }
  switch (s[0]) {
    case 'N':
      if (s[1] == ';') {
        return 2;
      }
      break;
    case 'b':
      if (s[1] == ':' && ((unsigned int)(s[2] - '0') < 2u) && s[3] == ';') {
        out_var_value = static_cast<bool>(s[2] - '0');
        return 4;
      }
      break;
    case 'd':
      if (s[1] == ':') {
        s += 2;
        char *end_ptr;
        double floatval = strtod(s, &end_ptr);
        if (*end_ptr == ';' && end_ptr > s) {
          out_var_value = floatval;
          return (int)(end_ptr - s + 3);
        }
      }
      break;
    case 'i':
      if (s[1] == ':') {
        s += 2;
        int j = 0;
        while (s[j]) {
          if (s[j] == ';') {
            int64_t intval = 0;
            if (php_try_to_int(s, j, &intval)) {
              s += j + 1;
              out_var_value = intval;
              return j + 3;
            }

            int k = 0;
            if (s[k] == '-' || s[k] == '+') {
              k++;
            }
            while ('0' <= s[k] && s[k] <= '9') {
              k++;
            }
            if (k == j) {
              out_var_value = mixed(s, j);
              return j + 3;
            }
            return 0;
          }
          j++;
        }
      }
      break;
    case 's':
      if (s[1] == ':') {
        s += 2;
        uint32_t j = 0;
        uint32_t len = 0;
        while ('0' <= s[j] && s[j] <= '9') {
          len = len * 10 + s[j++] - '0';
        }
        if (j > 0 && s[j] == ':' && s[j + 1] == '"' && len < string::max_size() && j + 2 + len < s_len) {
          s += j + 2;

          if (s[len] == '"' && s[len + 1] == ';') {
            out_var_value = mixed(s, len);
            return len + 6 + j;
          }
        }
      }
      break;
    case 'a':
      if (s[1] == ':') {
        const char *s_begin = s;
        s += 2;
        int j = 0, len = 0;
        while ('0' <= s[j] && s[j] <= '9') {
          len = len * 10 + s[j++] - '0';
        }
        if (j > 0 && len >= 0 && s[j] == ':' && s[j + 1] == '{') {
          s += j + 2;
          s_len -= j + 4;

          array_size size(0, len, false);
          if (s[0] == 'i') {//try to cheat
            size = array_size(len, 0, s[1] == ':' && s[2] == '0' && s[3] == ';');
          }
          array<mixed> res(size);

          while (len-- > 0) {
            if (s[0] == 'i' && s[1] == ':') {
              s += 2;
              int k = 0;
              while (s[k]) {
                if (s[k] == ';') {
                  int64_t intval = 0;
                  if (php_try_to_int(s, k, &intval)) {
                    s += k + 1;
                    s_len -= k + 3;
                    int length = do_unserialize(s, s_len, res[intval]);
                    if (!length) {
                      return 0;
                    }
                    s += length;
                    s_len -= length;
                    break;
                  }

                  int q = 0;
                  if (s[q] == '-' || s[q] == '+') {
                    q++;
                  }
                  while ('0' <= s[q] && s[q] <= '9') {
                    q++;
                  }
                  if (q == k) {
                    string key(s, k);
                    s += k + 1;
                    s_len -= k + 3;
                    int length = do_unserialize(s, s_len, res[key]);
                    if (!length) {
                      return 0;
                    }
                    s += length;
                    s_len -= length;
                    break;
                  }
                  return 0;
                }
                k++;
              }
            } else if (s[0] == 's' && s[1] == ':') {
              s += 2;
              uint32_t k = 0;
              uint32_t str_len = 0;
              while ('0' <= s[k] && s[k] <= '9') {
                str_len = str_len * 10 + s[k++] - '0';
              }
              if (k > 0 && s[k] == ':' && s[k + 1] == '"' && str_len < string::max_size() && k + 2 + str_len < s_len) {
                s += k + 2;

                if (s[str_len] == '"' && s[str_len + 1] == ';') {
                  string key(s, str_len);
                  s += str_len + 2;
                  s_len -= str_len + 6 + k;
                  int length = do_unserialize(s, s_len, res[key]);
                  if (!length) {
                    return 0;
                  }
                  s += length;
                  s_len -= length;
                }
              } else {
                return 0;
              }

            } else {
              return 0;
            }
          }

          if (s[0] == '}') {
            out_var_value = std::move(res);
            return (int)(s - s_begin + 1);
          }
        }
      }
      break;
  }
  return 0;
}

mixed unserialize_raw(const char *v, int32_t v_len) {
  mixed result;

  if (do_unserialize(v, v_len, result) == v_len) {
    return result;
  }

  return false;
}


mixed f$unserialize(const string &v) {
  return unserialize_raw(v.c_str(), v.size());
}


static void json_append_one_char(unsigned int c) {
  static_SB.append_char('\\');
  static_SB.append_char('u');
  static_SB.append_char("0123456789abcdef"[c >> 12]);
  static_SB.append_char("0123456789abcdef"[(c >> 8) & 15]);
  static_SB.append_char("0123456789abcdef"[(c >> 4) & 15]);
  static_SB.append_char("0123456789abcdef"[c & 15]);
}

static bool json_append_char(unsigned int c) {
  if (c < 0x10000) {
    if (0xD7FF < c && c < 0xE000) {
      return false;
    }
    json_append_one_char(c);
    return true;
  } else if (c <= 0x10ffff) {
    c -= 0x10000;
    json_append_one_char(0xD800 | (c >> 10));
    json_append_one_char(0xDC00 | (c & 0x3FF));
    return true;
  } else {
    return false;
  }
}

static bool do_json_encode_string_php(const char *s, int len, int64_t options) {
  int begin_pos = static_SB.size();
  if (options & JSON_UNESCAPED_UNICODE) {
    static_SB.reserve(2 * len + 2);
  } else {
    static_SB.reserve(6 * len + 2);
  }
  static_SB.append_char('"');

#define ERROR {static_SB.set_pos (begin_pos); static_SB.append ("null", 4); return false;}
#define CHECK(x) if (!(x)) {php_warning ("Not a valid utf-8 character at pos %d in function json_encode", pos); ERROR}
#define APPEND_CHAR(x) CHECK(json_append_char (x))

  int a, b, c, d;
  for (int pos = 0; pos < len; pos++) {
    switch (s[pos]) {
      case '"':
        static_SB.append_char('\\');
        static_SB.append_char('"');
        break;
      case '\\':
        static_SB.append_char('\\');
        static_SB.append_char('\\');
        break;
      case '/':
        static_SB.append_char('\\');
        static_SB.append_char('/');
        break;
      case '\b':
        static_SB.append_char('\\');
        static_SB.append_char('b');
        break;
      case '\f':
        static_SB.append_char('\\');
        static_SB.append_char('f');
        break;
      case '\n':
        static_SB.append_char('\\');
        static_SB.append_char('n');
        break;
      case '\r':
        static_SB.append_char('\\');
        static_SB.append_char('r');
        break;
      case '\t':
        static_SB.append_char('\\');
        static_SB.append_char('t');
        break;
      case 0 ... 7:
      case 11:
      case 14 ... 31:
        json_append_one_char(s[pos]);
        break;
      case -128 ... -1:
        a = s[pos];
        CHECK ((a & 0x40) != 0);

        b = s[++pos];
        CHECK((b & 0xc0) == 0x80);
        if ((a & 0x20) == 0) {
          CHECK((a & 0x1e) > 0);
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
          } else {
            APPEND_CHAR(((a & 0x1f) << 6) | (b & 0x3f));
          }
          break;
        }

        c = s[++pos];
        CHECK((c & 0xc0) == 0x80);
        if ((a & 0x10) == 0) {
          CHECK(((a & 0x0f) | (b & 0x20)) > 0);
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
            static_SB.append_char(static_cast<char>(c));
          } else {
            APPEND_CHAR(((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f));
          }
          break;
        }

        d = s[++pos];
        CHECK((d & 0xc0) == 0x80);
        if ((a & 0x08) == 0) {
          CHECK(((a & 0x07) | (b & 0x30)) > 0);
          if (options & JSON_UNESCAPED_UNICODE) {
            static_SB.append_char(static_cast<char>(a));
            static_SB.append_char(static_cast<char>(b));
            static_SB.append_char(static_cast<char>(c));
            static_SB.append_char(static_cast<char>(d));
          } else {
            APPEND_CHAR(((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f));
          }
          break;
        }

        CHECK(0);
        break;
      default:
        static_SB.append_char(s[pos]);
        break;
    }
  }

  static_SB.append_char('"');
  return true;
#undef ERROR
#undef CHECK
#undef APPEND_CHAR
}

static bool do_json_encode_string_vkext(const char *s, int len) {
  static_SB.reserve(2 * len + 2);
  if (static_SB.string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    return false;
  }

  static_SB.append_char('"');

  for (int pos = 0; pos < len; pos++) {
    char c = s[pos];
    if (unlikely ((unsigned int)c < 32u)) {
      switch (c) {
        case '\b':
          static_SB.append_char('\\');
          static_SB.append_char('b');
          break;
        case '\f':
          static_SB.append_char('\\');
          static_SB.append_char('f');
          break;
        case '\n':
          static_SB.append_char('\\');
          static_SB.append_char('n');
          break;
        case '\r':
          static_SB.append_char('\\');
          static_SB.append_char('r');
          break;
        case '\t':
          static_SB.append_char('\\');
          static_SB.append_char('t');
          break;
      }
    } else {
      if (c == '"' || c == '\\' || c == '/') {
        static_SB.append_char('\\');
      }
      static_SB.append_char(c);
    }
  }

  static_SB.append_char('"');

  return true;
}

bool do_json_encode(const mixed &v, int64_t options, bool simple_encode) {
  switch (v.get_type()) {
    case mixed::type::NUL:
      static_SB.append("null", 4);
      return true;
    case mixed::type::BOOLEAN:
      if (v.as_bool()) {
        static_SB.append("true", 4);
      } else {
        static_SB.append("false", 5);
      }
      return true;
    case mixed::type::INTEGER:
      static_SB << v.as_int();
      return true;
    case mixed::type::FLOAT:
      if (is_ok_float(v.as_double())) {
        static_SB << (simple_encode ? f$number_format(v.as_double(), 6, DOT, string()) : string(v.as_double()));
      } else {
        php_warning("strange double %lf in function json_encode", v.as_double());
        if (options & JSON_PARTIAL_OUTPUT_ON_ERROR) {
          static_SB.append("0", 1);
        } else {
          return false;
        }
      }
      return true;
    case mixed::type::STRING:
      if (simple_encode) {
        return do_json_encode_string_vkext(v.as_string().c_str(), v.as_string().size());
      }

      return do_json_encode_string_php(v.as_string().c_str(), v.as_string().size(), options);
    case mixed::type::ARRAY: {
      bool is_vector = v.as_array().is_vector();
      const bool force_object = static_cast<bool>(JSON_FORCE_OBJECT & options);
      if (!force_object && !is_vector && v.as_array().size().string_size == 0) {
        int n = 0;
        for (array<mixed>::const_iterator p = v.as_array().begin(); p != v.as_array().end(); ++p) {
          if (p.get_key().to_int() != n) {
            break;
          }
          n++;
        }
        if (n == v.as_array().count()) {
          if (v.as_array().get_next_key() == v.as_array().count()) {
            is_vector = true;
          } else {
            php_warning("Corner case in json conversion, [] could be easy transformed to {}");
          }
        }
      }
      is_vector &= !force_object;

      static_SB << "{["[is_vector];

      for (array<mixed>::const_iterator p = v.as_array().begin(); p != v.as_array().end(); ++p) {
        if (p != v.as_array().begin()) {
          static_SB << ',';
        }

        if (!is_vector) {
          const array<mixed>::key_type key = p.get_key();
          if (array<mixed>::is_int_key(key)) {
            static_SB << '"' << key.to_int() << '"';
          } else {
            if (!do_json_encode(key, options, simple_encode)) {
              if (!(options & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
                return false;
              }
            }
          }
          static_SB << ':';
        }

        if (!do_json_encode(p.get_value(), options, simple_encode)) {
          if (!(options & JSON_PARTIAL_OUTPUT_ON_ERROR)) {
            return false;
          }
        }
      }

      static_SB << "}]"[is_vector];
      return true;
    }
    default:
      __builtin_unreachable();
  }
}

Optional<string> f$json_encode(const mixed &v, int64_t options, bool simple_encode) {
  bool has_unsupported_option = static_cast<bool>(options & ~JSON_AVAILABLE_OPTIONS);
  if (has_unsupported_option) {
    php_warning("Wrong parameter options = %ld in function json_encode", options);
    return false;
  }

  static_SB.clean();

  if (!do_json_encode(v, options, simple_encode)) {
    return false;
  }

  return static_SB.str();
}

string f$vk_json_encode_safe(const mixed &v, bool simple_encode) {
  static_SB.clean();
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  do_json_encode(v, 0, simple_encode);
  if (string_buffer::string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) {
    static_SB.clean();
    string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
    THROW_EXCEPTION (new_Exception(string(__FILE__), __LINE__, string("json_encode buffer overflow", 27)));
    return string();
  }
  string_buffer::string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return static_SB.str();
}


static void json_skip_blanks(const char *s, int &i) {
  while (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n') {
    i++;
  }
}

static bool do_json_decode(const char *s, int s_len, int &i, mixed &v) {
  if (!v.is_null()) {
    v.destroy();
  }
  json_skip_blanks(s, i);
  switch (s[i]) {
    case 'n':
      if (s[i + 1] == 'u' &&
          s[i + 2] == 'l' &&
          s[i + 3] == 'l') {
        i += 4;
        return true;
      }
      break;
    case 't':
      if (s[i + 1] == 'r' &&
          s[i + 2] == 'u' &&
          s[i + 3] == 'e') {
        i += 4;
        new(&v) mixed(true);
        return true;
      }
      break;
    case 'f':
      if (s[i + 1] == 'a' &&
          s[i + 2] == 'l' &&
          s[i + 3] == 's' &&
          s[i + 4] == 'e') {
        i += 5;
        new(&v) mixed(false);
        return true;
      }
      break;
    case '"': {
      int j = i + 1;
      int slashes = 0;
      while (j < s_len && s[j] != '"') {
        if (s[j] == '\\') {
          slashes++;
          j++;
        }
        j++;
      }
      if (j < s_len) {
        int len = j - i - 1 - slashes;

        string value(len, false);

        i++;
        int l;
        for (l = 0; l < len && i < j; l++) {
          char c = s[i];
          if (c == '\\') {
            i++;
            switch (s[i]) {
              case '"':
              case '\\':
              case '/':
                value[l] = s[i];
                break;
              case 'b':
                value[l] = '\b';
                break;
              case 'f':
                value[l] = '\f';
                break;
              case 'n':
                value[l] = '\n';
                break;
              case 'r':
                value[l] = '\r';
                break;
              case 't':
                value[l] = '\t';
                break;
              case 'u':
                if (isxdigit(s[i + 1]) && isxdigit(s[i + 2]) && isxdigit(s[i + 3]) && isxdigit(s[i + 4])) {
                  int num = 0;
                  for (int t = 0; t < 4; t++) {
                    char c = s[++i];
                    if ('0' <= c && c <= '9') {
                      num = num * 16 + c - '0';
                    } else {
                      c |= 0x20;
                      if ('a' <= c && c <= 'f') {
                        num = num * 16 + c - 'a' + 10;
                      }
                    }
                  }

                  if (0xD7FF < num && num < 0xE000) {
                    if (s[i + 1] == '\\' && s[i + 2] == 'u' &&
                        isxdigit(s[i + 3]) && isxdigit(s[i + 4]) && isxdigit(s[i + 5]) && isxdigit(s[i + 6])) {
                      i += 2;
                      int u = 0;
                      for (int t = 0; t < 4; t++) {
                        char c = s[++i];
                        if ('0' <= c && c <= '9') {
                          u = u * 16 + c - '0';
                        } else {
                          c |= 0x20;
                          if ('a' <= c && c <= 'f') {
                            u = u * 16 + c - 'a' + 10;
                          }
                        }
                      }

                      if (0xD7FF < u && u < 0xE000) {
                        num = (((num & 0x3FF) << 10) | (u & 0x3FF)) + 0x10000;
                      } else {
                        i -= 6;
                        return false;
                      }
                    } else {
                      return false;
                    }
                  }

                  if (num < 128) {
                    value[l] = (char)num;
                  } else if (num < 0x800) {
                    value[l++] = (char)(0xc0 + (num >> 6));
                    value[l] = (char)(0x80 + (num & 63));
                  } else if (num < 0xffff) {
                    value[l++] = (char)(0xe0 + (num >> 12));
                    value[l++] = (char)(0x80 + ((num >> 6) & 63));
                    value[l] = (char)(0x80 + (num & 63));
                  } else {
                    value[l++] = (char)(0xf0 + (num >> 18));
                    value[l++] = (char)(0x80 + ((num >> 12) & 63));
                    value[l++] = (char)(0x80 + ((num >> 6) & 63));
                    value[l] = (char)(0x80 + (num & 63));
                  }
                  break;
                }
                /* fallthrough */
              default:
                return false;
            }
            i++;
          } else {
            value[l] = s[i++];
          }
        }
        value.shrink(l);

        new(&v) mixed(value);
        i++;
        return true;
      }
      break;
    }
    case '[': {
      array<mixed> res;
      i++;
      json_skip_blanks(s, i);
      if (s[i] != ']') {
        do {
          mixed value;
          if (!do_json_decode(s, s_len, i, value)) {
            return false;
          }
          res.push_back(value);
          json_skip_blanks(s, i);
        } while (s[i++] == ',');

        if (s[i - 1] != ']') {
          return false;
        }
      } else {
        i++;
      }

      new(&v) mixed(res);
      return true;
    }
    case '{': {
      array<mixed> res;
      i++;
      json_skip_blanks(s, i);
      if (s[i] != '}') {
        do {
          mixed key;
          if (!do_json_decode(s, s_len, i, key) || !key.is_string()) {
            return false;
          }
          json_skip_blanks(s, i);
          if (s[i++] != ':') {
            return false;
          }

          if (!do_json_decode(s, s_len, i, res[key])) {
            return false;
          }
          json_skip_blanks(s, i);
        } while (s[i++] == ',');

        if (s[i - 1] != '}') {
          return false;
        }
      } else {
        i++;
      }

      new(&v) mixed(res);
      return true;
    }
    default: {
      int j = i;
      while (s[j] == '-' || ('0' <= s[j] && s[j] <= '9') || s[j] == 'e' || s[j] == 'E' || s[j] == '+' || s[j] == '.') {
        j++;
      }
      if (j > i) {
        int64_t intval = 0;
        if (php_try_to_int(s + i, j - i, &intval)) {
          i = j;
          new(&v) mixed(intval);
          return true;
        }

        char *end_ptr;
        double floatval = strtod(s + i, &end_ptr);
        if (end_ptr == s + j) {
          i = j;
          new(&v) mixed(floatval);
          return true;
        }
      }
      break;
    }
  }

  return false;
}

mixed f$json_decode(const string &v, bool assoc) {
  if (!assoc) {
//    php_warning ("json_decode doesn't support decoding to class, returning array");
  }

  mixed result;
  int i = 0;
  if (do_json_decode(v.c_str(), v.size(), i, result)) {
    json_skip_blanks(v.c_str(), i);
    if (i == (int)v.size()) {
      return result;
    }
  }

  return mixed();
}

void do_print_r(const mixed &v, int depth) {
  if (depth == 10) {
    php_warning("Depth %d reached. Recursion?", depth);
    return;
  }

  switch (v.get_type()) {
    case mixed::type::NUL:
      break;
    case mixed::type::BOOLEAN:
      if (v.as_bool()) {
        *coub << '1';
      }
      break;
    case mixed::type::INTEGER:
      *coub << v.as_int();
      break;
    case mixed::type::FLOAT:
      *coub << v.as_double();
      break;
    case mixed::type::STRING:
      *coub << v.as_string();
      break;
    case mixed::type::ARRAY: {
      *coub << "Array\n";

      string shift(depth << 3, ' ');
      *coub << shift << "(\n";

      for (array<mixed>::const_iterator it = v.as_array().begin(); it != v.as_array().end(); ++it) {
        *coub << shift << "    [" << it.get_key() << "] => ";
        do_print_r(it.get_value(), depth + 1);
        *coub << '\n';
      }

      *coub << shift << ")\n";
      break;
    }
    default:
      __builtin_unreachable();
  }
}

void do_var_dump(const mixed &v, int depth) {
  if (depth == 10) {
    php_warning("Depth %d reached. Recursion?", depth);
    return;
  }

  string shift(depth * 2, ' ');

  switch (v.get_type()) {
    case mixed::type::NUL:
      *coub << shift << "NULL";
      break;
    case mixed::type::BOOLEAN:
      *coub << shift << "bool(" << (v.as_bool() ? "true" : "false") << ')';
      break;
    case mixed::type::INTEGER:
      *coub << shift << "int(" << v.as_int() << ')';
      break;
    case mixed::type::FLOAT:
      *coub << shift << "float(" << v.as_double() << ')';
      break;
    case mixed::type::STRING:
      *coub << shift << "string(" << (int)v.as_string().size() << ") \"" << v.as_string() << '"';
      break;
    case mixed::type::ARRAY: {
      *coub << shift << (false && v.as_array().is_vector() ? "vector(" : "array(") << v.as_array().count() << ") {\n";

      for (array<mixed>::const_iterator it = v.as_array().begin(); it != v.as_array().end(); ++it) {
        *coub << shift << "  [";
        if (array<mixed>::is_int_key(it.get_key())) {
          *coub << it.get_key();
        } else {
          *coub << '"' << it.get_key() << '"';
        }
        *coub << "]=>\n";
        do_var_dump(it.get_value(), depth + 1);
      }

      *coub << shift << "}";
      break;
    }
    default:
      __builtin_unreachable();
  }
  *coub << '\n';
}

void var_export_escaped_string(const string &s) {
  for (string::size_type i = 0; i < s.size(); i++) {
    switch (s[i]) {
      case '\'':
      case '\\':
        *coub << "\\" << s[i];
        break;
      case '\0':
        *coub << "\' . \"\\0\" . \'";
        break;
      default:
        *coub << s[i];
    }
  }
}

void do_var_export(const mixed &v, int depth, char endc = 0) {
  if (depth == 10) {
    php_warning("Depth %d reached. Recursion?", depth);
    return;
  }

  string shift(depth * 2, ' ');

  switch (v.get_type()) {
    case mixed::type::NUL:
      *coub << shift << "NULL";
      break;
    case mixed::type::BOOLEAN:
      *coub << shift << (v.as_bool() ? "true" : "false");
      break;
    case mixed::type::INTEGER:
      *coub << shift << v.as_int();
      break;
    case mixed::type::FLOAT:
      *coub << shift << v.as_double();
      break;
    case mixed::type::STRING:
      *coub << shift << '\'';
      var_export_escaped_string(v.as_string());
      *coub << '\'';
      break;
    case mixed::type::ARRAY: {
      const bool is_vector = v.as_array().is_vector();
      *coub << shift << "array(\n";

      for (array<mixed>::const_iterator it = v.as_array().begin(); it != v.as_array().end(); ++it) {
        if (!is_vector) {
          *coub << shift;
          if (array<mixed>::is_int_key(it.get_key())) {
            *coub << it.get_key();
          } else {
            *coub << '\'' << it.get_key() << '\'';
          }
          *coub << " =>";
          if (it.get_value().is_array()) {
            *coub << "\n";
            do_var_export(it.get_value(), depth + 1, ',');
          } else {
            do_var_export(it.get_value(), 1, ',');
          }
        } else {
          do_var_export(it.get_value(), depth + 1, ',');
        }
      }

      *coub << shift << ")";
      break;
    }
    default:
      __builtin_unreachable();
  }
  if (endc != 0) {
    *coub << endc;
  }
  *coub << '\n';
}


string f$print_r(const mixed &v, bool buffered) {
  if (buffered) {
    f$ob_start();
    do_print_r(v, 0);
    return f$ob_get_clean().val();
  }

  do_print_r(v, 0);
  if (run_once && f$ob_get_level() == 0) {
    dprintf(kstdout, "%s", f$ob_get_contents().c_str());
    f$ob_clean();
  }
  return string();
}

void f$var_dump(const mixed &v) {
  do_var_dump(v, 0);
  if (run_once && f$ob_get_level() == 0) {
    const string &to_print = f$ob_get_contents();
    if (to_print.size() != write(kstdout, to_print.c_str(), to_print.size())) {
      php_warning("Couldn't write %u bytes to stdout in var_dump", to_print.size());
    }
    f$ob_clean();
  }
}

string f$var_export(const mixed &v, bool buffered) {
  if (buffered) {
    f$ob_start();
    do_var_export(v, 0);
    return f$ob_get_clean().val();
  }
  do_var_export(v, 0);
  if (run_once && f$ob_get_level() == 0) {
    dprintf(kstdout, "%s", f$ob_get_contents().c_str());
    f$ob_clean();
  }
  return string();
}

string f$cp1251(const string &utf8_string) {
  return f$vk_utf8_to_win(utf8_string);
}


int64_t f$system(const string &query) {
  return system(query.c_str());
}

void f$kphp_set_context_on_error(const array<mixed> &tags, const array<mixed> &extra_info, const string& env) {
  auto &context = JsonLogger::get();
  static_SB.clean();

  if (do_json_encode(tags, 0, false)) {
    context.set_tags(static_SB.c_str(), static_SB.size());
  }
  static_SB.clean();

  if (do_json_encode(extra_info, 0, false)) {
    context.set_extra_info(static_SB.c_str(), static_SB.size());
  }
  static_SB.clean();

  context.set_env(env.c_str(), env.size());
}
