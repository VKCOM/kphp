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
#include "runtime/datetime/datetime_functions.h"
#include "runtime/exception.h"
#include "runtime/files.h"
#include "runtime/interface.h"
#include "runtime/json-functions.h"
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

  size_t buf_size = 30;
  char buf[buf_size];
  static_SB.clean() << prefix;

  if (more_entropy) {
    snprintf(buf, buf_size, "%08x%05x%.8f", sec, usec, f$lcg_value() * 10);
    static_SB.append(buf, 23);
  } else {
    snprintf(buf, buf_size, "%08x%05x", sec, usec);
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
    php_warning("Wrong parameter seconds (%" PRIi64 ") specified in function sleep, must be in seconds", seconds);
    return;
  }

  f$usleep(seconds * 1000000);
}

void f$usleep(int64_t micro_seconds) {
  int64_t sleep_time = micro_seconds;
  if (sleep_time < 0) {
    php_warning("Wrong parameter micro_seconds (%" PRIi64 ") specified in function usleep", sleep_time);
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

  if (read_size < 12 || read_safe(read_fd, buf, read_size, name) < static_cast<ssize_t>(read_size)) {
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
        if (read_safe(read_fd, image + read_size, size - read_size, name) < static_cast<ssize_t>(size - read_size)) {
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
      } else if (!strncmp((const char *)buf, php_sig_jpc, sizeof(php_sig_jpc)) && static_cast<int>(read_size) >= 42) {
        type = IMAGETYPE_JPEG;

        width = (buf[8] << 24) + (buf[9] << 16) + (buf[10] << 8) + buf[11];
        height = (buf[12] << 24) + (buf[13] << 16) + (buf[14] << 8) + buf[15];
        channels = (buf[40] << 8) + buf[41];

        if (channels < 0 || channels > 256 || static_cast<int>(read_size) < 42 + 3 * channels || width <= 0 || height <= 0) {
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
        while (static_cast<int>(read_size) >= 42 + buf_pos + 8) {
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

            if (channels < 0 || channels > 256 || static_cast<int>(read_size) < 42 + buf_pos + 8 + 3 * channels || width <= 0 || height <= 0) {
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
          if (file_pos >= size || static_cast<off_t>(file_pos) != static_cast<ssize_t>(file_pos) || static_cast<ssize_t>(file_pos) < 0) {
            break;
          }

          read_size = min_size;
          if (size - file_pos < min_size) {
            read_size = size - file_pos;
          }

          if (read_size < 50 || pread(read_fd, buf, read_size, static_cast<off_t>(file_pos)) < static_cast<ssize_t>(read_size)) {
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
  int len = snprintf(reinterpret_cast<char *>(buf), min_size, "width=\"%d\" height=\"%d\"", width, height);
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
  return {};
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
  return {};
}

string f$cp1251(const string &utf8_string) {
  return f$vk_utf8_to_win(utf8_string);
}

void f$kphp_set_context_on_error(const array<mixed> &tags, const array<mixed> &extra_info, const string& env) {
  auto &json_logger = vk::singleton<JsonLogger>::get();
  static_SB.clean();

  const auto get_json_string_from_SB_without_brackets = [] {
    vk::string_view json_str{static_SB.c_str(), static_SB.size()};
    php_assert(json_str.size() >= 2 && json_str.front() == '{' && json_str.back() == '}');
    json_str.remove_prefix(1);
    json_str.remove_suffix(1);
    return json_str;
  };

  if (impl_::JsonEncoder(JSON_FORCE_OBJECT, false).encode(tags)) {
    auto tags_json = get_json_string_from_SB_without_brackets();
    dl::CriticalSectionGuard critical_section;
    json_logger.set_tags(tags_json);
  }
  static_SB.clean();

  if (impl_::JsonEncoder(JSON_FORCE_OBJECT, false).encode(extra_info)) {
    auto extra_info_json = get_json_string_from_SB_without_brackets();
    dl::CriticalSectionGuard critical_section;
    json_logger.set_extra_info(extra_info_json);
  }
  static_SB.clean();

  dl::CriticalSectionGuard critical_section;
  json_logger.set_env({env.c_str(), env.size()});
}
