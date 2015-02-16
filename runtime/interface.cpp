#define _FILE_OFFSET_BITS 64

#include "interface.h"

#include <arpa/inet.h>
#include <netdb.h>

#include "PHP/common-net-functions.h"

#include "array_functions.h"
#include "bcmath.h"
#include "curl.h"
#include "datetime.h"
#include "drivers.h"
#include "exception.h"
#include "files.h"
#include "net_events.h"
#include "openssl.h"
#include "regexp.h"
#include "resumable.h"
#include "rpc.h"
#include "streams.h"
#include "string_functions.h"
#include "url.h"
#include "zlib.h"

static enum {QUERY_TYPE_NONE, QUERY_TYPE_CONSOLE, QUERY_TYPE_HTTP, QUERY_TYPE_RPC} query_type;

static const string HTTP_DATE ("D, d M Y H:i:s \\G\\M\\T", 21);

static const int OB_MAX_BUFFERS = 50;
static int ob_cur_buffer;

static string_buffer oub[OB_MAX_BUFFERS];
string_buffer *coub;
static int http_need_gzip;

void f$ob_clean (void) {
  coub->clean();
}

bool f$ob_end_clean (void) {
  if (ob_cur_buffer == 0) {
    return false;
  }

  coub = &oub[--ob_cur_buffer];
  return true;
}

OrFalse <string> f$ob_get_clean (void) {
  if (ob_cur_buffer == 0) {
    return false;
  }

  string result = coub->str();
  coub = &oub[--ob_cur_buffer];

  return result;
}

string f$ob_get_contents (void) {
  return coub->str();
}

void f$ob_start (const string &callback) {
  if (ob_cur_buffer + 1 == OB_MAX_BUFFERS) {
    php_warning ("Maximum nested level of output buffering reached. Can't do ob_start(%s)", callback.c_str());
    return;
  }
  if (!callback.empty()) {
    if (ob_cur_buffer == 0 && callback == string ("ob_gzhandler", 12)) {
      http_need_gzip |= 4;
    } else {
      php_critical_error ("unsupported callback %s at buffering level %d", callback.c_str(), ob_cur_buffer + 1);
    }
  }

  coub = &oub[++ob_cur_buffer];
  coub->clean();
}

static int http_return_code;
static string http_status_line;
static char headers_storage[sizeof (array <string>)];
static array <string> *headers = reinterpret_cast <array <string> *> (headers_storage);
static long long header_last_query_num = -1;

static bool check_status_line_int (const char *str, int str_len, int *pos) {
  if (*pos != str_len && str[*pos] == '0') {
    (*pos)++;
    return true;
  }

  for (int i = 0; i <= 9; i++) {//allow up to 9 digits total
    if (*pos == str_len || str[*pos] < '0' || str[*pos] > '9') {
      return i > 0;
    }
    (*pos)++;
  }
  return false;
}

static bool check_status_line (const char *str, int str_len) {
  //skip check for beginning with "HTTP/"

  int pos = 5;
  if (!check_status_line_int (str, str_len, &pos)) {
    return false;
  }
  if (pos == str_len || str[pos] != '.') {
    return false;
  }
  pos++;
  if (!check_status_line_int (str, str_len, &pos)) {
    return false;
  }

  if (pos == str_len || str[pos] != ' ') {
    return false;
  }
  pos++;

  if (pos == str_len || str[pos] < '1' || str[pos] > '9') {
    return false;
  }
  pos++;
  if (pos == str_len || str[pos] < '0' || str[pos] > '9') {
    return false;
  }
  pos++;
  if (pos == str_len || str[pos] < '0' || str[pos] > '9') {
    return false;
  }
  pos++;

  if (pos == str_len || str[pos] != ' ') {
    return false;
  }
  pos++;

  while (pos != str_len) {
    if ((0 <= str[pos] && str[pos] <= 31) || str[pos] == 127) {
      return false;
    }
    pos++;
  }

  return true;
}

static void header (const char *str, int str_len, bool replace = true, int http_response_code = 0) {
  if (dl::query_num != header_last_query_num) {
    new (headers_storage) array <string>();
    header_last_query_num = dl::query_num;
  }

  //status line
  if (str_len >= 5 && !strncasecmp (str, "HTTP/", 5)) {
    if (check_status_line (str, str_len)) {
      http_status_line = string (str, str_len);
      int pos = 5;
      while (str[pos] != ' ') {
        pos++;
      }
      sscanf (str + pos, "%d", &http_return_code);
    } else {
      php_critical_error ("wrong status line '%s' specified in function header", str);
    }
    return;
  }

  //regular header
  const char *p = strchr (str, ':');
  if (p == NULL) {
    php_warning ("Wrong header line specified: \"%s\"", str);
    return;
  }
  string name = f$trim (string (str, (dl::size_type)(p - str)));
  if (strpbrk (name.c_str(), "()<>@,;:\\\"/[]?={}") != NULL) {
    php_warning ("Wrong header name: \"%s\"", name.c_str());
    return;
  }
  for (int i = 0; i < (int)name.size(); i++) {
    if (name[i] <= 32 || name[i] >= 127) {
      php_warning ("Wrong header name: \"%s\"", name.c_str());
      return;
    }
  }
  
  for (int i = (int)(p - str + 1); i < str_len; i++) {
    if ((0 <= str[i] && str[i] <= 31) || str[i] == 127) {
      if (i + 2 < str_len && str[i] == '\r' && str[i + 1] == '\n' && (str[i + 2] == ' ' || str[i + 2] == '\t')) {
        i += 2;
        continue;
      }

      php_warning ("Wrong header value: \"%s\"", p + 1);
      return;
    }
  }

  string value = string (name.size() + (str_len - (p - str)) + 2, false);
  memcpy (value.buffer(), name.c_str(), name.size());
  memcpy (value.buffer() + name.size(), p, str_len - (p - str));
  value[value.size() - 2] = '\r';
  value[value.size() - 1] = '\n';

  name = f$strtolower (name);
  if (replace || !headers->has_key (name)) {
    headers->set_value (name, value);
  } else {
    (*headers)[name].append (value);
  }

  if (str_len >= 9 && !strncasecmp (str, "Location:", 9) && http_response_code == 0) {
    http_response_code = 302;
  }

  if (str_len && http_response_code > 0 && http_response_code != http_return_code) {
    http_return_code = http_response_code;
    http_status_line = string();
  }
}

void f$header (const string &str, bool replace, int http_response_code) {
  header (str.c_str(), (int)str.size(), replace, http_response_code);
}

void f$setrawcookie (const string &name, const string &value, int expire, const string &path, const string &domain, bool secure, bool http_only) {
  string date = f$gmdate (HTTP_DATE, expire);

  static_SB_spare.clean();
  static_SB_spare + "Set-Cookie: " + name + '=';
  if (value.empty()) {
    static_SB_spare += "DELETED; expires=Thu, 01 Jan 1970 00:00:01 GMT";
  } else {
    static_SB_spare += value;

    if (expire != 0) {
      static_SB_spare + "; expires=" + date;
    }
  }
  if (!path.empty()) {
    static_SB_spare + "; path=" + path;
  }
  if (!domain.empty()) {
    static_SB_spare + "; domain=" + domain;
  }
  if (secure) {
    static_SB_spare + "; secure";
  }
  if (http_only) {
    static_SB_spare + "; HttpOnly";
  }
  header (static_SB_spare.c_str(), (int)static_SB_spare.size(), false);
}

void f$setcookie (const string &name, const string &value, int expire, const string &path, const string &domain, bool secure, bool http_only) {
  f$setrawcookie (name, f$urlencode (value), expire, path, domain, secure, http_only);
}

static inline const char *http_get_error_msg_text (int *code) {
  if (*code == 200) {
    return "OK";
  }
  if (*code < 100 || *code > 999) {
    *code = 500;
  }
  switch (*code) {
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 206: return "Partial Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 307: return "Temporary Redirect";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 408: return "Request Timeout";
    case 411: return "Length Required";
    case 413: return "Request Entity Too Large";
    case 414: return "Request-URI Too Long";
    case 418: return "I'm a teapot";
    case 480: return "Temporarily Unavailable";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
  }
  return "Extension Code";
}

static const string_buffer *get_headers (int content_length) {//can't use static_SB, returns pointer to static_SB_spare
  string date = f$gmdate (HTTP_DATE);
  static_SB_spare.clean() + "Date: " + date;
  header (static_SB_spare.c_str(), (int)static_SB_spare.size());

  static_SB_spare.clean() + "Content-Length: " + content_length;
  header (static_SB_spare.c_str(), (int)static_SB_spare.size());

  php_assert (dl::query_num == header_last_query_num);

  static_SB_spare.clean();
  if (http_status_line.size()) {
    static_SB_spare + http_status_line + "\r\n";
  } else {
    const char *message = http_get_error_msg_text (&http_return_code);
    static_SB_spare + "HTTP/1.1 " + http_return_code + " " + message + "\r\n";
  }

  const array <string> *arr = headers;
  for (array <string>::const_iterator p = arr->begin(); p != arr->end(); ++p) {
    static_SB_spare += p.get_value();
  }
  static_SB_spare += "\r\n";

  return &static_SB_spare;
}

static var (*shutdown_function) (void);
static bool finished;
static bool flushed;

void f$fastcgi_finish_request (void) {
  if (flushed) {
    return;
  }

  flushed = true;

  php_assert (ob_cur_buffer >= 0);
  int first_not_empty_buffer = 0;
  while (first_not_empty_buffer < ob_cur_buffer && (int)oub[first_not_empty_buffer].size() == 0) {
    first_not_empty_buffer++;
  }

  for (int i = first_not_empty_buffer + 1; i <= ob_cur_buffer; i++) {
    oub[first_not_empty_buffer].append (oub[i].c_str(), oub[i].size());
  }

  switch (query_type) {
    case QUERY_TYPE_CONSOLE: {
      //TODO console_set_result
      fflush (stderr);

      write_safe (1, oub[first_not_empty_buffer].buffer(), oub[first_not_empty_buffer].size());

      free_static();//TODO move to finish_script

      break;
    }
    case QUERY_TYPE_HTTP: {
      php_assert (http_set_result != NULL);

      const string_buffer *compressed;
      if ((http_need_gzip & 5) == 5) {
        header ("Content-Encoding: gzip", 22, true);
        compressed = zlib_encode (oub[first_not_empty_buffer].c_str(), oub[first_not_empty_buffer].size(), 6, ZLIB_ENCODE);
      } else if ((http_need_gzip & 6) == 6) {
        header ("Content-Encoding: deflate", 25, true);
        compressed = zlib_encode (oub[first_not_empty_buffer].c_str(), oub[first_not_empty_buffer].size(), 6, ZLIB_COMPRESS);
      } else {
        compressed = &oub[first_not_empty_buffer];
      }

      const string_buffer *headers = get_headers (compressed->size());
      http_set_result (headers->buffer(), headers->size(), compressed->buffer(), compressed->size(), 0);//TODO remove exit_code parameter

      break;
    }
    case QUERY_TYPE_RPC: {
      php_assert (rpc_set_result != NULL);
      rpc_set_result (oub[first_not_empty_buffer].buffer(), oub[first_not_empty_buffer].size(), 0);

      break;
    }
    default:
      php_assert (0);
      exit (1);
  }

  ob_cur_buffer = 0;
  coub = &oub[ob_cur_buffer];
  coub->clean();
}

void f$register_shutdown_function (var (*f) (void)) {
  shutdown_function = f;
}

void finish (int exit_code) {
  if (!finished && shutdown_function) {
    finished = true;
    shutdown_function();
  }

  f$fastcgi_finish_request();

  finish_script (exit_code);

  //unreachable
  php_assert (0);
}

bool f$exit (const var &v) {
  if (v.is_string()) {
    *coub += v;
    finish (0);
  } else {
    finish (v.to_int());
  }
  return true;
}

bool f$die (const var &v) {
  return f$exit (v);
}


OrFalse <int> f$ip2long (const string &ip) {
  struct in_addr result;
  if (inet_pton (AF_INET, ip.c_str(), &result) != 1) {
    return false;
  }
  return (int)ntohl (result.s_addr);
}

OrFalse <string> f$ip2ulong (const string &ip) {
  struct in_addr result;
  if (inet_pton (AF_INET, ip.c_str(), &result) != 1) {
    return false;
  }

  char buf[25];
  int len = sprintf (buf, "%u", ntohl (result.s_addr));
  return string (buf, len);
}

string f$long2ip (int num) {
  static_SB.clean().reserve (100);
  for (int i = 3; i >= 0; i--) {
    static_SB += (num >> (i * 8)) & 255;
    if (i) {
      static_SB.append_char ('.');
    }
  }
  return static_SB.str();
}

OrFalse <array <string> > f$gethostbynamel (const string &name) {
  dl::enter_critical_section();//OK
  struct hostent *hp = gethostbyname (name.c_str());
  if (hp == NULL || hp->h_addr_list == NULL) {
    dl::leave_critical_section();
    return false;
  }
  dl::leave_critical_section();

  array <string> result;
  for (int i = 0; hp->h_addr_list[i] != 0; i++) {
    dl::enter_critical_section();//OK
    const char *ip = inet_ntoa (*(struct in_addr *)hp->h_addr_list[i]);
    dl::leave_critical_section();
    result.push_back (string (ip, (dl::size_type)strlen (ip)));
  }

  return result;
}

OrFalse <string> f$inet_pton (const string &address) {
  int af, size;
  if (strchr (address.c_str(), ':')) {
    af = AF_INET6;
    size = 16;
  } else if (strchr (address.c_str(), '.')) {
    af = AF_INET;
    size = 4;
  } else {
    php_warning ("Unrecognized address \"%s\"", address.c_str());
    return false;
  }

  char buffer[17] = {0};
  dl::enter_critical_section();//OK
  if (inet_pton (af, address.c_str(), buffer) <= 0) {
    dl::leave_critical_section();
    php_warning ("Unrecognized address \"%s\"", address.c_str());
    return false;
  }
  dl::leave_critical_section();

  return string (buffer, size);
}

extern int run_once;

int print (const char *s) {
  if (run_once) {
    dl::enter_critical_section();//OK
    fprintf (stdout, "%s", s);
    dl::leave_critical_section();
    return 1;    
  }
  *coub += s;
  return 1;
}

int print (const char *s, int s_len) {
  if (run_once) {
    dl::enter_critical_section();//OK
    fwrite (s, s_len, 1, stdout);
    dl::leave_critical_section();
    return 1;
  }
  coub->append (s, s_len);
  return 1;
}

int print (const string &s) {
  if (run_once) {
    dl::enter_critical_section();//OK
    fwrite (s.c_str(), s.size(), 1, stdout);
    dl::leave_critical_section();
    return 1;
  }
  *coub += s;
  return 1;
}

int print (const string_buffer &sb) {
  if (run_once) {
    dl::enter_critical_section();//OK
    fwrite (sb.buffer(), sb.size(), 1, stdout);
    dl::leave_critical_section();
    return 1;
  }
  coub->append (sb.buffer(), sb.size());
  return 1;
}

int dbg_echo (const char *s) {
  dl::enter_critical_section();//OK
  fprintf (stderr, "%s", s);
  dl::leave_critical_section();
  return 1;
}

int dbg_echo (const char *s, int s_len) {
  dl::enter_critical_section();//OK
  fwrite (s, s_len, 1, stderr);
  dl::leave_critical_section();
  return 1;
}

int dbg_echo (const string &s) {
  dl::enter_critical_section();//OK
  fwrite (s.c_str(), s.size(), 1, stderr);
  dl::leave_critical_section();
  return 1;
}

int dbg_echo (const string_buffer &sb) {
  dl::enter_critical_section();//OK
  fwrite (sb.buffer(), sb.size(), 1, stderr);
  dl::leave_critical_section();
  return 1;
}


bool f$get_magic_quotes_gpc (void) {
  return false;
}

string v$d$PHP_SAPI __attribute__ ((weak));


static string php_sapi_name (void) {
  switch (query_type) {
    case QUERY_TYPE_CONSOLE:
      return string ("cli", 3);
    case QUERY_TYPE_HTTP:
      return string ("Kitten PHP", 10);
    case QUERY_TYPE_RPC:
      if (run_once) {
        return string ("cli", 3);
      } else {
        return string ("Kitten PHP", 10);
      }
    default:
      php_assert (0);
      exit (1);
  }
}

string f$php_sapi_name (void) {
  return v$d$PHP_SAPI;
}


var v$_SERVER  __attribute__ ((weak));
var v$_GET     __attribute__ ((weak));
var v$_POST    __attribute__ ((weak));
var v$_FILES   __attribute__ ((weak));
var v$_COOKIE  __attribute__ ((weak));
var v$_REQUEST __attribute__ ((weak));
var v$_ENV     __attribute__ ((weak));

var v$argc  __attribute__ ((weak));
var v$argv  __attribute__ ((weak));

static char uploaded_files_storage[sizeof (array <int>)];
static array <int> *uploaded_files = reinterpret_cast <array <int> *> (uploaded_files_storage);
static long long uploaded_files_last_query_num = -1;

static const int MAX_FILES = 100;

static string raw_post_data;

bool f$is_uploaded_file (const string &filename) {
  return (dl::query_num == uploaded_files_last_query_num && uploaded_files->get_value (filename) == 1);
}

bool f$move_uploaded_file (const string &oldname, const string &newname) {
  if (!f$is_uploaded_file (oldname)) {
    return false;
  }

  dl::enter_critical_section();//NOT OK: uploaded_files
  if (f$rename (oldname, newname)) {
    uploaded_files->unset (oldname);
    dl::leave_critical_section();
    return true;
  }
  dl::leave_critical_section();

  return false;
}


class post_reader {
  char *buf;
  int post_len;
  int buf_pos;
  int buf_len;

  const string boundary;

  post_reader (const post_reader &);//DISABLE copy constructor
  post_reader operator = (const post_reader &);//DISABLE copy assignment

public:
  post_reader (const char *post, int post_len, const string &boundary): post_len (post_len), buf_pos (0), boundary (boundary) {
    if (post == NULL) {
      buf = php_buf;
      buf_len = 0;
    } else {
      buf = (char *)post;
      buf_len = post_len;
    }
  }

  int operator [] (int i) {
    php_assert (i >= buf_pos);
    php_assert (i <= post_len);
    if (i >= post_len) {
      return 0;
    }

    i -= buf_pos;
    while (i >= buf_len) {
      int left = post_len - buf_pos - buf_len;
      int chunk_size = (int)boundary.size() + 65536 + 10;
//      fprintf (stderr, "Load at pos %d. i = %d, buf_len = %d, left = %d, chunk_size = %d\n", i + buf_pos, i, buf_len, left, chunk_size);
      if (buf_len > 0) {
        int to_leave = chunk_size;
        int to_erase = buf_len - to_leave;

        php_assert (left > 0);
        php_assert (to_erase >= to_leave);

        memcpy (buf, buf + to_erase, to_leave);
        buf_pos += to_erase;
        i -= to_erase;

        buf_len = to_leave + http_load_long_query (buf + to_leave, min (to_leave, left), min (PHP_BUF_LEN - to_leave, left));
      } else {
        buf_len = http_load_long_query (buf, min (2 * chunk_size, left), min (PHP_BUF_LEN, left));
      }
    }

    return buf[i];
  }

  bool is_boundary (int i) {
    php_assert (i >= buf_pos);
    php_assert (i <= post_len);
    if (i >= post_len) {
      return true;
    }

    if (i > 0) {
      if ((*this)[i] == '\r') {
        i++;
      }
      if ((*this)[i] == '\n') {
        i++;
      } else {
        return false;
      }
    }
    if ((*this)[i] == '-' && (*this)[i + 1] == '-') {
      i += 2;
    } else {
      return false;
    }

    if (i + (int)boundary.size() > post_len) {
      return false;
    }

    if (i - buf_pos + (int)boundary.size() <= buf_len) {
      return !memcmp (buf + i - buf_pos, boundary.c_str(), boundary.size());
    }

    for (int j = 0; j < (int)boundary.size(); j++) {
      if ((*this)[i + j] != boundary[j]) {
        return false;
      }
    }
    return true;
  }

  int upload_file (const string &file_name, int &pos, int max_file_size) {
    php_assert (pos > 0 && buf_len > 0 && buf_pos <= pos && pos <= post_len);

    if (pos == post_len) {
      return -UPLOAD_ERR_PARTIAL;
    }

    if (!f$is_writeable (file_name)) {
      return -UPLOAD_ERR_CANT_WRITE;
    }

    dl::enter_critical_section();//OK
    int file_fd = open_safe (file_name.c_str(), O_WRONLY | O_TRUNC, 0644);
    if (file_fd < 0) {
      dl::leave_critical_section();
      return -UPLOAD_ERR_NO_FILE;
    }

    struct stat stat_buf;
    if (fstat (file_fd, &stat_buf) < 0) {
      close_safe (file_fd);
      dl::leave_critical_section();
      return -UPLOAD_ERR_CANT_WRITE;
    }
    dl::leave_critical_section();

    php_assert (S_ISREG (stat_buf.st_mode));

    if (buf_len == post_len) {
      int i = pos;
      while (!is_boundary (i)) {
        i++;
      }
      if (i == post_len) {
        dl::enter_critical_section();//OK
        close_safe (file_fd);
        dl::leave_critical_section();
        return -UPLOAD_ERR_PARTIAL;
      }

      int file_size = i - pos;
      if (file_size > max_file_size) {
        dl::enter_critical_section();//OK
        close_safe (file_fd);
        dl::leave_critical_section();
        return -UPLOAD_ERR_FORM_SIZE;
      }

      dl::enter_critical_section();//OK
      if (write_safe (file_fd, buf + pos, (size_t)file_size) < (ssize_t)file_size) {
        file_size = -UPLOAD_ERR_CANT_WRITE;
      }

      close_safe (file_fd);
      dl::leave_critical_section();
      return file_size;
    } else {
      long long file_size = 0;
      const char *s;

      while (true) {
        (*this)[pos];
        int i = pos - buf_pos;

        while (true) {
          php_assert (0 <= i && i <= buf_len);

          s = static_cast <const char *>(memmem (buf + i, buf_len - i, boundary.c_str(), boundary.size()));
          if (s == NULL) {
            break;
          } else {
            int r = s - buf;
            if (r > i + 2 && buf[r - 1] == '-' && buf[r - 2] == '-' && buf[r - 3] == '\n') {
              r -= 3;
              if (r > i && buf[r - 1] == '\r') {
                r--;
              }

              s = buf + r;
              break;
            } else {
              i = r + 1;
              continue;
            }
          }
        }
        if (s != NULL) {
          break;
        }

        int left = post_len - buf_pos - buf_len;
        int to_leave = (int)boundary.size() + 10;
        int to_erase = buf_len - to_leave;
        int to_write = to_erase - (pos - buf_pos);

//        fprintf (stderr, "Load at pos %d. buf_len = %d, left = %d, to_leave = %d, to_erase = %d, to_write = %d.\n", buf_len + buf_pos, buf_len, left, to_leave, to_erase, to_write);

        if (left == 0) {
          dl::enter_critical_section();//OK
          close_safe (file_fd);
          dl::leave_critical_section();
          return -UPLOAD_ERR_PARTIAL;
        }
        file_size += to_write;
        if (file_size > max_file_size) {
          dl::enter_critical_section();//OK
          close_safe (file_fd);
          dl::leave_critical_section();
          return -UPLOAD_ERR_FORM_SIZE;
        }

        php_assert (to_erase >= to_leave);

        dl::enter_critical_section();//OK
        if (write_safe (file_fd, buf + pos - buf_pos, (size_t)to_write) < (ssize_t)to_write) {
          close_safe (file_fd);
          dl::leave_critical_section();
          return -UPLOAD_ERR_CANT_WRITE;
        }
        dl::leave_critical_section();

        memcpy (buf, buf + to_erase, to_leave);
        buf_pos += to_erase;
        pos += to_write;

        buf_len = to_leave + http_load_long_query (buf + to_leave, min (PHP_BUF_LEN - to_leave, left), min (PHP_BUF_LEN - to_leave, left));
      }

      php_assert (s != NULL);

      dl::enter_critical_section();//OK
      int to_write = (int)(s - (buf + pos - buf_pos));
      if (write_safe (file_fd, buf + pos - buf_pos, (size_t)to_write) < (ssize_t)to_write) {
        close_safe (file_fd);
        dl::leave_critical_section();
        return -UPLOAD_ERR_CANT_WRITE;
      }
      pos += to_write;

      close_safe (file_fd);
      dl::leave_critical_section();

      return (int)(file_size + to_write);
    }
  }
};

static int parse_multipart_one (post_reader &data, int i) {
  string content_type ("text/plain", 10);
  string name;
  string filename;
  int max_file_size = INT_MAX;
  while (!data.is_boundary (i) && 33 <= data[i] && data[i] <= 126) {
    int j;
    string header_name;
    for (j = i; !data.is_boundary (j) && 33 <= data[j] && data[j] <= 126 && data[j] != ':'; j++) {
      header_name.push_back (data[j]);
    }
    if (data[j] != ':') {
      return j;
    }

    header_name = f$strtolower (header_name);
    i = j + 1;

    string header_value;
    do {
      while (!data.is_boundary (i + 1) && (data[i] != '\r' || data[i + 1] != '\n') && data[i] != '\n') {
        header_value.push_back (data[i++]);
      }
      if (data[i] == '\r') {
        i++;
      }
      if (data[i] == '\n') {
        i++;
      }

      if (data.is_boundary (i) || (33 <= data[i] && data[i] <= 126) || (data[i] == '\r' && data[i + 1] == '\n') || data[i] == '\n') {
        break;
      }
    } while (1);
    header_value = f$trim (header_value);

    if (!strcmp (header_name.c_str(), "content-disposition")) {
      if (strncmp (header_value.c_str(), "form-data;", 10)) {
        return i;
      }
      const char *p = header_value.c_str() + 10;
      while (1) {
        while (*p && *p == ' ') {
          p++;
        }
        if (*p == 0) {
          break;
        }
        const char *p_end = p;
        while (*p_end && *p_end != '=') {
          p_end++;
        }
        if (*p_end == 0) {
          break;
        }
        const string key = f$trim (string (p, (dl::size_type)(p_end - p)));
        p = ++p_end;
        while (*p_end && *p_end != ';') {
          p_end++;
        }
        string value = f$trim (string (p, (dl::size_type)(p_end - p)));
        if ((int)value.size() > 1 && value[0] == '"' && value[value.size() - 1] == '"') {
          value.assign (value, 1, value.size() - 2);
        }
        p = p_end;
        if (*p) {
          p++;
        }

        if (!strcmp (key.c_str(), "name")) {
          name = value;
        } else if (!strcmp (key.c_str(), "filename")) {
          filename = value;
        } else {
//          fprintf (stderr, "Unknown key %s\n", key.c_str());
        }
      }
    } else if (!strcmp (header_name.c_str(), "content-type")) {
      content_type = f$strtolower (header_value);
    } else if (!strcmp (header_name.c_str(), "max-file-size")) {
      max_file_size = header_value.to_int();
    } else {
//      fprintf (stderr, "Unknown header %s\n", header_name.c_str());
    }
  }
  if (data.is_boundary (i)) {
    return i;
  }
  if (data[i] == '\r') {
    i++;
  }
  if (data[i] == '\n') {
    i++;
  } else {
    return i;
  }

  if (filename.empty()) {
    if (!name.empty()) {
      string post_data;
      while (!data.is_boundary (i) && (int)post_data.size() < 65536) {
        post_data.push_back (data[i++]);
      }
      if ((int)post_data.size() < 65536) {
        if (!strncmp (content_type.c_str(), "application/x-www-form-urlencoded", 33)) {
          f$parse_str (post_data, v$_POST[name]);
        } else {
          //TODO
          v$_POST.set_value (name, post_data);
        }
      }
    }
  } else {
    int file_size;
    OrFalse <string> tmp_name;
    if (v$_FILES.count() < MAX_FILES) {
      if (dl::query_num != uploaded_files_last_query_num) {
        new (uploaded_files_storage) array <int>();
        uploaded_files_last_query_num = dl::query_num;
      }

      dl::enter_critical_section();//NOT OK: uploaded_files
      tmp_name = f$tempnam (string(), string());
      uploaded_files->set_value (tmp_name.val(), 1);
      dl::leave_critical_section();

      if (f$boolval (tmp_name)) {
        file_size = data.upload_file (tmp_name.val(), i, max_file_size);

        if (file_size < 0) {
          dl::enter_critical_section();//NOT OK: uploaded_files
          f$unlink (tmp_name.val());
          uploaded_files->unset (tmp_name.val());
          dl::leave_critical_section();
        }
      } else {
        file_size = -UPLOAD_ERR_NO_TMP_DIR;
      }
    } else {
      file_size = -UPLOAD_ERR_NO_FILE;
    }

    if (name.size() >= 2 && name[name.size() - 2] == '[' && name[name.size() - 1] == ']') {
      var &file = v$_FILES[name.substr (0, name.size() - 2)];
      if (file_size >= 0) {
        file[string ("name", 4)].push_back (filename);
        file[string ("type", 4)].push_back (content_type);
        file[string ("size", 4)].push_back (file_size);
        file[string ("tmp_name", 8)].push_back (tmp_name.val());
        file[string ("error", 5)].push_back (UPLOAD_ERR_OK);
      } else {
        file[string ("name", 4)].push_back (string());
        file[string ("type", 4)].push_back (string());
        file[string ("size", 4)].push_back (0);
        file[string ("tmp_name", 8)].push_back (string());
        file[string ("error", 5)].push_back (-file_size);
      }
    } else {
      var &file = v$_FILES[name];
      if (file_size >= 0) {
        file.set_value (string ("name", 4), filename);
        file.set_value (string ("type", 4), content_type);
        file.set_value (string ("size", 4), file_size);
        file.set_value (string ("tmp_name", 8), tmp_name.val());
        file.set_value (string ("error", 5), UPLOAD_ERR_OK);
      } else {
        file.set_value (string ("size", 4), 0);
        file.set_value (string ("tmp_name", 8), string());
        file.set_value (string ("error", 5), -file_size);
      }
    }
  }

  return i;
}

static bool parse_multipart (const char *post, int post_len, const string &boundary) {
  static const int MAX_BOUNDARY_LENGTH = 70;

  if (boundary.empty() || (int)boundary.size() > MAX_BOUNDARY_LENGTH) {
    return false;
  }
  php_assert (post_len >= 0);

  post_reader data (post, post_len, boundary);

  for (int i = 0; i < post_len; i++) {
//    fprintf (stderr, "!!!! %d\n", i);
    i = parse_multipart_one (data, i);

//    fprintf (stderr, "???? %d\n", i);
    while (!data.is_boundary (i)) {
      i++;
    }
    i += 2 + (int)boundary.size();

    while (i < post_len && data[i] != '\n') {
      i++;
    }
  }

  return true;
}

void f$parse_multipart (const string &post, const string &boundary) {
  parse_multipart (post.c_str(), (int)post.size(), boundary);
}


static char arg_vars_storage[sizeof (array <string>)];
static array <string> *arg_vars = NULL;

void arg_add (const char *value) {
  php_assert (dl::query_num == 0);

  if (arg_vars == NULL) {
    new (arg_vars_storage) array <string> ();
    arg_vars = reinterpret_cast <array <string> *> (arg_vars_storage);
  }

  arg_vars->push_back (string (value, (dl::size_type)strlen (value)));
}


extern char **environ;

static void init_superglobals (const char *uri, int uri_len, const char *get, int get_len, const char *headers, int headers_len, const char *post, int post_len,
                               const char *request_method, int request_method_len, int remote_ip, int remote_port, int keep_alive,
                               const int *serialized_data, int serialized_data_len, long long rpc_request_id, int rpc_remote_ip, int rpc_remote_port, int rpc_remote_pid, int rpc_remote_utime) {
  rpc_parse (serialized_data, serialized_data_len);

  INIT_VAR(var, v$_SERVER );
  INIT_VAR(var, v$_GET    );
  INIT_VAR(var, v$_POST   );
  INIT_VAR(var, v$_FILES  );
  INIT_VAR(var, v$_COOKIE );
  INIT_VAR(var, v$_REQUEST);
  INIT_VAR(var, v$_ENV    );

  v$_SERVER  = array <var>();
  v$_GET     = array <var>();
  v$_POST    = array <var>();
  v$_FILES   = array <var>();
  v$_COOKIE  = array <var>();
  v$_REQUEST = array <var>();
  v$_ENV     = array <var>();

  string uri_str;
  if (uri_len) {
    uri_str.assign (uri, uri_len);
    v$_SERVER.set_value (string ("PHP_SELF", 8), uri_str);
    v$_SERVER.set_value (string ("SCRIPT_URL", 10), uri_str);
    v$_SERVER.set_value (string ("SCRIPT_NAME", 11), uri_str);
  }

  string get_str;
  if (get_len) {
    get_str.assign (get, get_len);
    f$parse_str (get_str, v$_GET);

    v$_SERVER.set_value (string ("QUERY_STRING", 12), get_str);
  }

  if (uri) {
    if (get_len) {
      v$_SERVER.set_value (string ("REQUEST_URI", 11), (static_SB.clean() + uri_str + '?' + get_str).str());
    } else {
      v$_SERVER.set_value (string ("REQUEST_URI", 11), uri_str);
    }
  }

  http_need_gzip = 0;
  string content_type ("application/x-www-form-urlencoded", 33);
  string content_type_lower = content_type;
  if (headers_len) {
    int i = 0;
    while (i < headers_len && 33 <= headers[i] && headers[i] <= 126) {
      int j;
      for (j = i; j < headers_len && 33 <= headers[j] && headers[j] <= 126 && headers[j] != ':'; j++) {
      }
      if (headers[j] != ':') {
        break;
      }

      string header_name = f$strtolower (string (headers + i, j - i));
      i = j + 1;

      string header_value;
      do {
        while (i < headers_len && headers[i] != '\r' && headers[i] != '\n') {
          header_value.push_back (headers[i++]);
        }

        while (i < headers_len && (headers[i] == '\r' || headers[i] == '\n')) {
          i++;
        }

        if (i == headers_len || (33 <= headers[i] && headers[i] <= 126)) {
          break;
        }
      } while (1);
      header_value = f$trim (header_value);

      if (!strcmp (header_name.c_str(), "accept-encoding")) {
        if (strstr (header_value.c_str(), "gzip") != NULL) {
          http_need_gzip |= 1;
        }
        if (strstr (header_value.c_str(), "deflate") != NULL) {
          http_need_gzip |= 2;
        }
      } else if (!strcmp (header_name.c_str(), "cookie")) {
        array <string> cookie = explode (';', header_value);
        for (int t = 0; t < (int)cookie.count(); t++) {
          array <string> cur_cookie = explode ('=', f$trim (cookie[t]), 2);
          if ((int)cur_cookie.count() == 2) {
            parse_str_set_value (v$_COOKIE, cur_cookie[0], f$urldecode (cur_cookie[1]));
          }
        }
      } else if (!strcmp (header_name.c_str(), "host")) {
        v$_SERVER.set_value (string ("SERVER_NAME", 11), header_value);
      }

      if (!strcmp (header_name.c_str(), "content-type")) {
        content_type = header_value;
        content_type_lower = f$strtolower (header_value);
      } else if (!strcmp (header_name.c_str(), "content-length")) {
        //must be equal to post_len, ignored
      } else {
        string key (header_name.size() + 5, false);
        bool good_name = true;
        for (int i = 0; i < (int)header_name.size(); i++) {
          if ('a' <= header_name[i] && header_name[i] <= 'z') {
            key[i + 5] = (char)(header_name[i] + 'A' - 'a');
          } else if ('0' <= header_name[i] && header_name[i] <= '9') {
            key[i + 5] = header_name[i];
          } else if ('-' == header_name[i]) {
            key[i + 5] = '_';
          } else {
            good_name = false;
            break;
          }
        }
        if (good_name) {
          key[0] = 'H';
          key[1] = 'T';
          key[2] = 'T';
          key[3] = 'P';
          key[4] = '_';
          v$_SERVER.set_value (key, header_value);
        } else {
//          fprintf (stderr, "%s : %s\n", header_name.c_str(), header_value.c_str());
        }
      }
    }
  }

  string HTTP_X_REAL_SCHEME  ("HTTP_X_REAL_SCHEME", 18);
  string HTTP_X_REAL_HOST    ("HTTP_X_REAL_HOST", 16);
  string HTTP_X_REAL_REQUEST ("HTTP_X_REAL_REQUEST", 19);
  if (v$_SERVER.isset (HTTP_X_REAL_SCHEME) && v$_SERVER.isset (HTTP_X_REAL_HOST) && v$_SERVER.isset (HTTP_X_REAL_REQUEST)) {
    string script_uri (v$_SERVER.get_value (HTTP_X_REAL_SCHEME).to_string());
    script_uri.append ("://", 3);
    script_uri.append (v$_SERVER.get_value (HTTP_X_REAL_HOST).to_string());
    script_uri.append (v$_SERVER.get_value (HTTP_X_REAL_REQUEST).to_string());
    v$_SERVER.set_value (string ("SCRIPT_URI", 10), script_uri);
  }

  if (post_len > 0) {
    bool is_parsed = (post != NULL);
//    fprintf (stderr, "!!!%.*s!!!\n", post_len, post);
    if (strstr (content_type_lower.c_str(), "application/x-www-form-urlencoded")) {
      if (post != NULL) {
        dl::enter_critical_section();//OK
        raw_post_data.assign (post, post_len);
        dl::leave_critical_section();

        f$parse_str (raw_post_data, v$_POST);
      }
    } else if (strstr (content_type_lower.c_str(), "multipart/form-data")) {
      const char *p = strstr (content_type_lower.c_str(), "boundary");
      if (p) {
        p += 8;
        p = strchr (content_type.c_str() + (p - content_type_lower.c_str()), '=');
        if (p) {
//          fprintf (stderr, "!!%s!!\n", p);
          p++;
          const char *end_p = strchrnul (p, ';');
          if (*p == '"' && p + 1 < end_p && end_p[-1] == '"') {
            p++;
            end_p--;
          }
//          fprintf (stderr, "!%s!\n", p);
          is_parsed |= parse_multipart (post, post_len, string (p, (dl::size_type)(end_p - p)));
        }
      }
    } else {
      if (post != NULL) {
        dl::enter_critical_section();//OK
        raw_post_data.assign (post, post_len);
        dl::leave_critical_section();
      }
    }

    if (!is_parsed) {
      int loaded = 0;
      while (loaded < post_len) {
        int to_load = min (PHP_BUF_LEN, post_len - loaded);
        http_load_long_query (php_buf, to_load, to_load);
        loaded += to_load;
      }
    }

    v$_SERVER.set_value (string ("CONTENT_TYPE", 12), content_type);
  }

  double cur_time = microtime (true);
  v$_SERVER.set_value (string ("GATEWAY_INTERFACE", 17), string ("CGI/1.1", 7));
  if (remote_ip) {
    v$_SERVER.set_value (string ("REMOTE_ADDR", 11), f$long2ip (remote_ip));
  }
  if (remote_port) {
    v$_SERVER.set_value (string ("REMOTE_PORT", 11), remote_port);
  }
  if (rpc_request_id) {
    v$_SERVER.set_value (string ("RPC_REQUEST_ID", 14), f$strval (Long (rpc_request_id)));
    v$_SERVER.set_value (string ("RPC_REMOTE_IP", 13), rpc_remote_ip);
    v$_SERVER.set_value (string ("RPC_REMOTE_PORT", 15), rpc_remote_port);
    v$_SERVER.set_value (string ("RPC_REMOTE_PID", 14), rpc_remote_pid);
    v$_SERVER.set_value (string ("RPC_REMOTE_UTIME", 16), rpc_remote_utime);
  }
  if (request_method_len) {
    v$_SERVER.set_value (string ("REQUEST_METHOD", 14), string (request_method, request_method_len));
  }
  v$_SERVER.set_value (string ("REQUEST_TIME", 12), int (cur_time));
  v$_SERVER.set_value (string ("REQUEST_TIME_FLOAT", 18), cur_time);
  v$_SERVER.set_value (string ("SERVER_PORT", 11), string ("80", 2));
  v$_SERVER.set_value (string ("SERVER_PROTOCOL", 15), string ("HTTP/1.1", 8));
  v$_SERVER.set_value (string ("SERVER_SIGNATURE", 16), (static_SB.clean() + "Apache/2.2.9 (Debian) PHP/5.2.6-1+lenny10 with Suhosin-Patch Server at " + v$_SERVER[string ("SERVER_NAME", 11)] + " Port 80").str());
  v$_SERVER.set_value (string ("SERVER_SOFTWARE", 15), string ("Apache/2.2.9 (Debian) PHP/5.2.6-1+lenny10 with Suhosin-Patch", 60));

  if (environ != NULL) {
    for (int i = 0; environ[i] != NULL; i++) {
      const char *s = strchr (environ[i], '=');
      php_assert (s != NULL);
      v$_ENV.set_value (string (environ[i], (dl::size_type)(s - environ[i])), string (s + 1, (dl::size_type)strlen (s + 1)));
    }
  }

  v$_REQUEST.as_array ("", -1) += v$_GET.to_array();
  v$_REQUEST.as_array ("", -1) += v$_POST.to_array();
  v$_REQUEST.as_array ("", -1) += v$_COOKIE.to_array();

  if (uri != NULL) {
    if (keep_alive) {
      header ("Connection: keep-alive", 22);
    } else {
      header ("Connection: close", 17);
    }
  }

  if (arg_vars == NULL) {
    if (get_len > 0) {
      array <var> argv_array (array_size (1, 0, true));
      argv_array.push_back (get_str);

      v$argv = argv_array;
      v$argc = 1;
    } else {
      v$argv = array <var> ();
      v$argc = 0;
    }
  } else {
    v$argc = arg_vars->count();
    v$argv = *arg_vars;
  }

  v$_SERVER.set_value (string ("argv", 4), v$argv);
  v$_SERVER.set_value (string ("argc", 4), v$argc);

  v$d$PHP_SAPI = php_sapi_name();
  
  php_assert (dl::in_critical_section == 0);
}

static http_query_data empty_http_data;
static rpc_query_data empty_rpc_data;

void init_superglobals (php_query_data *data) {
  http_query_data *http_data;
  rpc_query_data *rpc_data;
  if (data != NULL) {
    if (data->http_data == NULL) {
      php_assert (data->rpc_data != NULL);
      query_type = QUERY_TYPE_RPC;

      http_data = &empty_http_data;
      rpc_data = data->rpc_data;
    } else {
      php_assert (data->rpc_data == NULL);
      query_type = QUERY_TYPE_HTTP;

      http_data = data->http_data;
      rpc_data = &empty_rpc_data;
    }
  } else {
    query_type = QUERY_TYPE_CONSOLE;

    http_data = &empty_http_data;
    rpc_data = &empty_rpc_data;
  }

  init_superglobals (http_data->uri, http_data->uri_len, http_data->get, http_data->get_len, http_data->headers, http_data->headers_len,
                     http_data->post, http_data->post_len, http_data->request_method, http_data->request_method_len,
                     http_data->ip, http_data->port, http_data->keep_alive,
                     rpc_data->data, rpc_data->len, rpc_data->req_id, rpc_data->ip, rpc_data->port, rpc_data->pid, rpc_data->utime);
}


bool f$set_server_status (const string &status) {
  set_server_status (status.c_str(), status.size());
  return true;
}


double f$get_net_time (void) {
  return get_net_time();
}

double f$get_script_time (void) {
  return get_script_time();
}

int f$get_net_queries_count (void) {
  return get_net_queries_count();
}


int f$get_engine_uptime (void) {
  return get_engine_uptime();
}

string f$get_engine_version (void) {
  const char *full_version_str = get_engine_version();
  return string (full_version_str, strlen (full_version_str));
}


static char ini_vars_storage[sizeof (array <string>)];
static array <string> *ini_vars = NULL;

void ini_set (const char *key, const char *value) {
  php_assert (dl::query_num == 0);

  if (ini_vars == NULL) {
    new (ini_vars_storage) array <string> ();
    ini_vars = reinterpret_cast <array <string> *> (ini_vars_storage);
  }

  ini_vars->set_value (string (key, (dl::size_type)strlen (key)), string (value, (dl::size_type)strlen (value)));
}

OrFalse <string> f$ini_get (const string &s) {
  if (ini_vars != NULL && ini_vars->has_key (s)) {
    return ini_vars->get_value (s);
  }

  if (!strcmp (s.c_str(), "sendmail_path")) {
    return string ("/usr/sbin/sendmail -ti", 22);
  }
  if (!strcmp (s.c_str(), "max_execution_time")) {
    return string ("30.0", 4);//TODO
  }
  if (!strcmp (s.c_str(), "memory_limit")) {
    return f$strval ((int)dl::memory_limit);//TODO
  }
  if (!strcmp (s.c_str(), "include_path")) {
    return string();//TODO
  }

  php_warning ("Unrecognized option %s in ini_get", s.c_str());
  //php_assert (0);
  return false;
}

bool f$ini_set (const string &s, const string &value) {
  if (!strcmp (s.c_str(), "soap.wsdl_cache_enabled") || !strcmp (s.c_str(), "include_path") || !strcmp (s.c_str(), "memory")) {
    php_warning ("Option %s not supported in ini_set", s.c_str());
    return true;
  }
  if (!strcmp (s.c_str(), "default_socket_timeout")) {
    return false;//TODO
  }
  if (!strcmp (s.c_str(), "error_reporting")) {
    return f$error_reporting (f$intval (value));
  }

  php_critical_error ("unrecognized option %s in ini_set", s.c_str());
  return false; //unreachable
}


const Stream INPUT ("php://input", 11);
const Stream STDOUT ("php://stdout", 12);
const Stream STDERR ("php://stderr", 12);


static OrFalse <int> php_fwrite (const Stream &stream, const string &text) {
  if (eq2 (stream, STDOUT)) {
    print (text);
    return (int)text.size();
  }

  if (eq2 (stream, STDERR)) {
    dbg_echo (text);
    return (int)text.size();
  }

  if (eq2 (stream, INPUT)) {
    php_warning ("Stream %s is not writeable", INPUT.to_string().c_str());
    return false;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return false;
}

static int php_fseek (const Stream &stream, int offset __attribute__((unused)), int whence __attribute__((unused))) {
  if (eq2 (stream, STDOUT) || eq2 (stream, STDERR)) {
    php_warning ("Can't use fseek with stream %s", stream.to_string().c_str());
    return -1;
  }

  if (eq2 (stream, INPUT)) {
    //TODO implement this
    php_warning ("Can't use fseek with stream %s", INPUT.to_string().c_str());
    return -1;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return -1;
}

static OrFalse <int> php_ftell (const Stream &stream) {
  if (eq2 (stream, STDOUT) || eq2 (stream, STDERR)) {
    php_warning ("Can't use ftell with stream %s", stream.to_string().c_str());
    return false;
  }

  if (eq2 (stream, INPUT)) {
    //TODO implement this
    php_warning ("Can't use ftell with stream %s", INPUT.to_string().c_str());
    return false;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return false;
}

static OrFalse <string> php_fread (const Stream &stream, int length __attribute__((unused))) {
  if (eq2 (stream, STDOUT) || eq2 (stream, STDERR)) {
    php_warning ("Can't use fread with stream %s", stream.to_string().c_str());
    return false;
  }

  if (eq2 (stream, INPUT)) {
    //TODO implement this
    php_warning ("Can't use fread with stream %s", INPUT.to_string().c_str());
    return false;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return false;
}

static OrFalse <int> php_fpassthru (const Stream &stream) {
  if (eq2 (stream, STDOUT) || eq2 (stream, STDERR)) {
    php_warning ("Can't use fpassthru with stream %s", stream.to_string().c_str());
    return false;
  }

  if (eq2 (stream, INPUT)) {
    //TODO implement this
    php_warning ("Can't use fpassthru with stream %s", INPUT.to_string().c_str());
    return false;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return false;
}

static bool php_fflush (const Stream &stream) {
  if (eq2 (stream, STDOUT)) {
    //TODO implement this
    return false;
  }

  if (eq2 (stream, STDERR)) {
    php_warning ("There is no reason to fflush %s", stream.to_string().c_str());
    return true;
  }

  if (eq2 (stream, INPUT)) {
    php_warning ("Stream %s is not writeable, so there is no reason to fflush it", INPUT.to_string().c_str());
    return false;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return false;
}

static bool php_feof (const Stream &stream) {
  if (eq2 (stream, STDOUT) || eq2 (stream, STDERR)) {
    php_warning ("Can't use feof with stream %s", stream.to_string().c_str());
    return true;
  }

  if (eq2 (stream, INPUT)) {
    //TODO implement this
    php_warning ("Can't use feof with stream %s", INPUT.to_string().c_str());
    return true;
  }

  php_warning ("Stream %s not found", stream.to_string().c_str());
  return true;
}

static OrFalse <string> php_file_get_contents (const string &url) {
  if (eq2 (url, STDOUT) || eq2 (url, STDERR)) {
    php_warning ("Can't use file_get_contents with stream %s", url.c_str());
    return false;
  }

  if (eq2 (url, INPUT)) {
    return raw_post_data;
  }

  php_warning ("Stream %s not found", url.c_str());
  return false;
}

static OrFalse <int> php_file_put_contents (const string &url, const string &content) {
  if (eq2 (url, STDOUT)) {
    print (content);
    return (int)content.size();
  }

  if (eq2 (url, STDERR)) {
    dbg_echo (content);
    return (int)content.size();
  }

  if (eq2 (url, INPUT)) {
    php_warning ("Stream %s is not writeable", url.c_str());
    return false;
  }

  php_warning ("Stream %s not found", url.c_str());
  return false;
}


static void interface_init_static_once (void) {
  static stream_functions php_stream_functions;

  php_stream_functions.name = string ("php", 3);
  php_stream_functions.fopen = NULL;
  php_stream_functions.fwrite = php_fwrite;
  php_stream_functions.fseek = php_fseek;
  php_stream_functions.ftell = php_ftell;
  php_stream_functions.fread = php_fread;
  php_stream_functions.fpassthru = php_fpassthru;
  php_stream_functions.fflush = php_fflush;
  php_stream_functions.feof = php_feof;
  php_stream_functions.fclose = NULL;

  php_stream_functions.file_get_contents = php_file_get_contents;
  php_stream_functions.file_put_contents = php_file_put_contents;

  php_stream_functions.stream_socket_client = NULL;
  php_stream_functions.context_set_option = NULL;
  php_stream_functions.stream_set_option = NULL;
  php_stream_functions.get_fd = NULL;

  register_stream_functions (&php_stream_functions, false);
}


void init_static_once (void) {
  files_init_static_once();
  interface_init_static_once();
  openssl_init_static_once();
  regexp::init_static();
  resumable_init_static_once();
  rpc_init_static_once();
}


#include <clocale>

void init_static (void) {
  bcmath_init_static();
  //curl_init_static();//lazy inited
  datetime_init_static();
  drivers_init_static();
  exception_init_static();
  files_init_static();
  net_events_init_static();
  openssl_init_static();
  resumable_init_static();
  rpc_init_static();
  streams_init_static();

  shutdown_function = NULL;
  finished = false;
  flushed = false;

  php_warning_level = 2;
  php_disable_warnings = 0;

  static char engine_pid_buf[20];
  dl::enter_critical_section();//OK
  sprintf (engine_pid_buf, "] [%d] ", (int)getpid());
  dl::leave_critical_section();
  engine_pid = engine_pid_buf;

  ob_cur_buffer = -1;
  f$ob_start();

  setlocale (LC_CTYPE, "ru_RU.CP1251");

  INIT_VAR(string, http_status_line);

  //TODO
  header ("HTTP/1.0 200 OK", 15);
  php_assert (http_return_code == 200);
  header ("Server: nginx/0.3.33", 20);
  string date = f$gmdate (HTTP_DATE);
  static_SB_spare.clean() + "Date: " + date;
  header (static_SB_spare.c_str(), (int)static_SB_spare.size());
  header ("Content-Type: text/html; charset=windows-1251", 45);

  INIT_VAR(bool, empty_bool);
  INIT_VAR(int, empty_int);
  INIT_VAR(double, empty_float);
  INIT_VAR(string, empty_string);
  INIT_VAR(array <var>, *empty_array_var_storage);
  INIT_VAR(var, empty_var);

  INIT_VAR(var, v$argc);
  INIT_VAR(var, v$argv);

  INIT_VAR(string, v$d$PHP_SAPI);

  dl::enter_critical_section();//OK
  INIT_VAR(string, raw_post_data);
  dl::leave_critical_section();

  php_assert (dl::in_critical_section == 0);
}

void free_static (void) {
  php_assert (dl::in_critical_section == 0);

  curl_free_static();
  drivers_free_static();
  files_free_static();
  openssl_free_static();
  rpc_free_static();
  streams_free_static();

  dl::enter_critical_section();//OK
  if (dl::query_num == uploaded_files_last_query_num) {
    const array <int> *const_uploaded_files = uploaded_files;
    for (array <int>::const_iterator p = const_uploaded_files->begin(); p != const_uploaded_files->end(); ++p) {
      unlink (p.get_key().to_string().c_str());
    }
    uploaded_files_last_query_num--;
  }
  
  CLEAR_VAR(string, http_status_line);

  CLEAR_VAR(string, empty_string);
  CLEAR_VAR(var, empty_var);

  CLEAR_VAR(string, raw_post_data);

  dl::script_runned = false;
  php_assert (dl::use_script_allocator == false);
  dl::leave_critical_section();
}


#include <cassert>

void read_engine_tag (const char *file_name) {
  assert (dl::query_num == 0);

  struct stat stat_buf;
  int file_fd = open (file_name, O_RDONLY);
  if (file_fd < 0) {
    assert ("Can't open file with engine tag" && 0);
  }
  if (fstat (file_fd, &stat_buf) < 0) {
    assert ("Can't do fstat on file with engine tag" && 0);
  }

  const size_t MAX_SIZE = 40;
  char buf[MAX_SIZE + 3];

  size_t size = stat_buf.st_size;
  if (size > MAX_SIZE) {
    size = MAX_SIZE;
  }
  if (read_safe (file_fd, buf, size) < (ssize_t)size) {
    assert ("Can't read file with engine tag" && 0);
  }
  close (file_fd);

  for (size_t i = 0; i < size; i++) {
    if (buf[i] < 32 || buf[i] > 126) {
      buf[i] = ' ';
    }
  }

  char prev = ' ';
  size_t j = 0;
  for (size_t i = 0; i < size; i++) {
    if (buf[i] != ' ' || prev != ' ') {
      buf[j++] = buf[i];
    }

    prev = buf[i];
  }
  if (prev == ' ' && j > 0) {
    j--;
  }

  buf[j] = 0;
  ini_set ("error_tag", buf);

  buf[j++] = ' ';
  buf[j++] = '[';
  buf[j] = 0;

  engine_tag = strdup (buf);
}

