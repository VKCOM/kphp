// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/streams.h"

#include <cstdlib>
#include <cstring>
#include <sys/select.h>

#include "runtime/array_functions.h"
#include "runtime/allocator.h"
#include "runtime/critical_section.h"

constexpr int PHP_CSV_NO_ESCAPE = EOF;

static string::size_type max_wrapper_name_size = 0;

static array<const stream_functions *> wrappers;

static const stream_functions *default_stream_functions;

void register_stream_functions(const stream_functions *functions, bool is_default) {
  string wrapper_name = functions->name;

  php_assert (dl::query_num == 0);
  php_assert (functions != nullptr);
  php_assert (!wrappers.isset(wrapper_name));
  php_assert (strlen(wrapper_name.c_str()) == wrapper_name.size());

  if (wrapper_name.size() > max_wrapper_name_size) {
    max_wrapper_name_size = wrapper_name.size();
  }

  wrappers.set_value(wrapper_name, functions);

  if (is_default) {
    php_assert (default_stream_functions == nullptr);
    default_stream_functions = functions;
  }
}

static const stream_functions *get_stream_functions(const string &name) {
  return wrappers.get_value(name);
}

static const stream_functions *get_stream_functions_from_url(const string &url) {
  if (url.empty()) {
    return nullptr;
  }

  void *res = memmem(static_cast<const void *> (url.c_str()), url.size(),
                     static_cast<const void *> ("://"), 3);
  if (res != nullptr) {
    const char *wrapper_end = static_cast<const char *> (res);
    return get_stream_functions(string(url.c_str(), static_cast<string::size_type>(wrapper_end - url.c_str())));
  }

  return default_stream_functions;
}


mixed f$stream_context_create(const mixed &options) {
  mixed result;
  f$stream_context_set_option(result, options);
  return result;
}

bool f$stream_context_set_option(mixed &context, const mixed &wrapper, const string &option, const mixed &value) {
  if (!context.is_array() && !context.is_null()) {
    php_warning("Wrong context specified");
    return false;
  }

  if (!wrapper.is_string()) {
    php_warning("Parameter wrapper must be a string");
    return false;
  }

  string wrapper_string = wrapper.to_string();
  const stream_functions *functions = get_stream_functions(wrapper_string);
  if (functions == nullptr) {
    php_warning("Wrapper \"%s\" is not supported", wrapper_string.c_str());
    return false;
  }
  if (functions->context_set_option == nullptr) {
    php_warning("Wrapper \"%s\" doesn't support function stream_context_set_option", wrapper_string.c_str());
    return false;
  }

  return functions->context_set_option(context[wrapper_string], option, value);
}

bool f$stream_context_set_option(mixed &context __attribute__((unused)), const mixed &, const string &) {
  php_warning("Function stream_context_set_option can't take 3 arguments");
  return false;
}

bool f$stream_context_set_option(mixed &context, const mixed &options_var) {
  if (!context.is_array() && !context.is_null()) {
    php_warning("Wrong context specified");
    return false;
  }

  if (!options_var.is_array()) {
    php_warning("Parameter options must be an array of arrays");
    return false;
  }

  bool was_error = false;
  const array<mixed> options_array = options_var.to_array();
  for (array<mixed>::const_iterator it = options_array.begin(); it != options_array.end(); ++it) {
    const mixed &wrapper = it.get_key();
    const mixed &values = it.get_value();

    if (!values.is_array()) {
      php_warning("Parameter options[%s] must be an array", wrapper.to_string().c_str());
      was_error = true;
      continue;
    }

    const array<mixed> values_array = values.to_array();
    for (array<mixed>::const_iterator values_it = values_array.begin(); values_it != values_array.end(); ++values_it) {
      if (!f$stream_context_set_option(context, wrapper, f$strval(values_it.get_key()), values_it.get_value())) {
        was_error = true;
      }
    }
  }

  return !was_error;
}


mixed error_number_dummy;
mixed error_description_dummy;

mixed f$stream_socket_client(const string &url, mixed &error_number, mixed &error_description,
                           double timeout, int64_t flags, const mixed &context) {
  if (flags != STREAM_CLIENT_CONNECT) {
    php_warning("Wrong parameter flags = %" PRIi64 " in function stream_socket_client", flags);
    error_number = -1001;
    error_description = string("Wrong parameter flags", 21);
    return false;
  }

  const stream_functions *functions = get_stream_functions_from_url(url);
  if (functions == nullptr) {
    php_warning("Can't find appropriate wrapper for \"%s\"", url.c_str());
    error_number = -1002;
    error_description = string("Wrong wrapper", 13);
    return false;
  }
  if (functions->stream_socket_client == nullptr) {
    php_warning("Wrapper \"%s\" doesn't support function stream_socket_client", functions->name.c_str());
    error_number = -1003;
    error_description = string("Wrong wrapper", 13);
    return false;
  }

  int64_t error_number_int = 0;
  string error_description_string;
  mixed result = functions->stream_socket_client(url, error_number_int, error_description_string, timeout, flags, context.get_value(functions->name));
  error_number = error_number_int;
  error_description = error_description_string;
  return result;
}

bool f$stream_set_blocking(const Stream &stream, bool mode) {
  const string &url = stream.to_string();

  const stream_functions *functions = get_stream_functions_from_url(url);
  if (functions == nullptr) {
    php_warning("Can't find appropriate wrapper for \"%s\"", url.c_str());
    return false;
  }
  if (functions->stream_set_option == nullptr) {
    php_warning("Wrapper \"%s\" doesn't support function stream_set_blocking", functions->name.c_str());
    return false;
  }

  return functions->stream_set_option(stream, STREAM_SET_BLOCKING_OPTION, mode);
}

int64_t f$stream_set_write_buffer(const Stream &stream, int64_t size) {
  const string &url = stream.to_string();

  const stream_functions *functions = get_stream_functions_from_url(url);
  if (functions == nullptr) {
    php_warning("Can't find appropriate wrapper for \"%s\"", url.c_str());
    return -1;
  }
  if (functions->stream_set_option == nullptr) {
    php_warning("Wrapper \"%s\" doesn't support function stream_set_write_buffer", functions->name.c_str());
    return -1;
  }

  return functions->stream_set_option(stream, STREAM_SET_WRITE_BUFFER_OPTION, size);
}

int64_t f$stream_set_read_buffer(const Stream &stream, int64_t size) {
  const string &url = stream.to_string();

  const stream_functions *functions = get_stream_functions_from_url(url);
  if (functions == nullptr) {
    php_warning("Can't find appropriate wrapper for \"%s\"", url.c_str());
    return -1;
  }
  if (functions->stream_set_option == nullptr) {
    php_warning("Wrapper \"%s\" doesn't support function stream_set_read_buffer", functions->name.c_str());
    return -1;
  }

  return functions->stream_set_option(stream, STREAM_SET_READ_BUFFER_OPTION, size);
}


static void stream_array_to_fd_set(const mixed &streams_var, fd_set *fds, int32_t *nfds) {
  FD_ZERO(fds);

  if (!streams_var.is_array()) {
    if (!streams_var.is_null()) {
      php_warning("Not an array nor null passed to function stream_select");
    }

    return;
  }

  array<Stream> result;
  const array<Stream> &streams = streams_var.to_array();
  for (array<Stream>::const_iterator p = streams.begin(); p != streams.end(); ++p) {
    const Stream &stream = p.get_value();
    const string &url = stream.to_string();

    const stream_functions *functions = get_stream_functions_from_url(url);
    if (functions == nullptr) {
      php_warning("Can't find appropriate wrapper for \"%s\"", url.c_str());
      continue;
    }
    if (functions->get_fd == nullptr) {
      php_warning("Wrapper \"%s\" doesn't support stream_select", functions->name.c_str());
      continue;
    }

    int32_t fd = functions->get_fd(stream);
    if (fd == -1) {
      continue;
    }

    FD_SET(fd, fds);

    if (fd > *nfds) {
      *nfds = fd;
    }
  }
}

static void stream_array_from_fd_set(mixed &streams_var, fd_set *fds) {
  if (!streams_var.is_array()) {
    return;
  }

  const array<Stream> &streams = streams_var.to_array();
  if (streams.empty()) {
    return;
  }

  array<Stream> result;
  for (array<Stream>::const_iterator p = streams.begin(); p != streams.end(); ++p) {
    const Stream &stream = p.get_value();
    const string &url = stream.to_string();

    const stream_functions *functions = get_stream_functions_from_url(url);
    if (functions == nullptr) {
      continue;
    }
    if (functions->get_fd == nullptr) {
      continue;
    }

    int32_t fd = functions->get_fd(stream);
    if (fd == -1) {
      continue;
    }

    if (FD_ISSET(fd, fds)) {
      result.set_value(p.get_key(), stream);
    }
  }

  streams_var = result;
}

Optional<int64_t> f$stream_select(mixed &read, mixed &write, mixed &except, const mixed &tv_sec_var, int64_t tv_usec) {
  struct timeval tv, *timeout = nullptr;
  if (!tv_sec_var.is_null()) {
    int64_t tv_sec = tv_sec_var.to_int();
    if (tv_sec < 0) {
      php_warning("Wrong parameter tv_sec = %" PRIi64 "\n", tv_sec);
      return false;
    }
    if (tv_usec < 0 || tv_usec >= 1000000) {
      php_warning("Wrong parameter tv_usec = %" PRIi64 "\n", tv_usec);
      return false;
    }

    tv.tv_sec = tv_sec;
    tv.tv_usec = tv_usec;

    timeout = &tv;
  }

  fd_set rfds, wfds, efds;
  int32_t nfds = 0;

  stream_array_to_fd_set(read, &rfds, &nfds);
  stream_array_to_fd_set(write, &wfds, &nfds);
  stream_array_to_fd_set(except, &efds, &nfds);

  if (nfds == 0) {
    php_warning("No valid streams was passed to function stream_select");
    return false;
  }

//TODO use pselect
  dl::enter_critical_section();//OK
  int32_t select_result = select(nfds + 1, &rfds, &wfds, &efds, timeout);
  dl::leave_critical_section();

  if (select_result == -1) {
    php_warning("Call to select has failed: %m");
    return false;
  }

  stream_array_from_fd_set(read, &rfds);
  stream_array_from_fd_set(write, &wfds);
  stream_array_from_fd_set(except, &efds);

  return select_result;
}


#define STREAM_FUNCTION_BODY(function_name, error_result)                                             \
  const string &url = stream.to_string();                                                             \
                                                                                                      \
  const stream_functions *functions = get_stream_functions_from_url (url);                            \
  if (functions == nullptr) {                                                                            \
    php_warning ("Can't find appropriate wrapper for \"%s\"", url.c_str());                           \
    return error_result;                                                                              \
  }                                                                                                   \
  if (functions->function_name == nullptr) {                                                             \
    php_warning ("Wrapper \"%s\" doesn't support function " #function_name, functions->name.c_str()); \
    return error_result;                                                                              \
  }                                                                                                   \
                                                                                                      \
  return functions->function_name


Stream f$fopen(const string &stream, const string &mode) {
  STREAM_FUNCTION_BODY(fopen, false)(url, mode);
}

Optional<int64_t> f$fwrite(const Stream &stream, const string &text) {
  STREAM_FUNCTION_BODY(fwrite, false)(stream, text);
}

int64_t f$fseek(const Stream &stream, int64_t offset, int64_t whence) {
  STREAM_FUNCTION_BODY(fseek, -1)(stream, offset, whence);
}

bool f$rewind(const Stream &stream) {
  return f$fseek(stream, 0, 0) == 0;
}

Optional<int64_t> f$ftell(const Stream &stream) {
  STREAM_FUNCTION_BODY(ftell, false)(stream);
}

Optional<string> f$fread(const Stream &stream, int64_t length) {
  STREAM_FUNCTION_BODY(fread, false)(stream, length);
}

Optional<string> f$fgetc(const Stream &stream) {
  STREAM_FUNCTION_BODY(fgetc, false)(stream);
}

Optional<string> f$fgets(const Stream &stream, int64_t length) {
  STREAM_FUNCTION_BODY(fgets, false)(stream, length);
}

Optional<int64_t> f$fpassthru(const Stream &stream) {
  STREAM_FUNCTION_BODY(fpassthru, false)(stream);
}

bool f$fflush(const Stream &stream) {
  STREAM_FUNCTION_BODY(fflush, false)(stream);
}

bool f$feof(const Stream &stream) {
  STREAM_FUNCTION_BODY(feof, true)(stream);
}

bool f$fclose(const Stream &stream) {
  STREAM_FUNCTION_BODY(fclose, false)(stream);
}

Optional<int64_t> f$fprintf(const Stream &stream, const string &format, const array<mixed> &args) {
  return f$vfprintf(stream, format, args);
}

Optional<int64_t> f$vfprintf(const Stream &stream, const string &format, const array<mixed> &args) {
  string text = f$vsprintf(format, args);
  return f$fwrite(stream, text);
}

Optional<int64_t> f$fputcsv(const Stream &stream, const array<mixed> &fields, string delimiter,
                            string enclosure, string escape) {
  if (delimiter.empty()) {
    php_warning("delimiter must be a character");
    return false;
  } else if (delimiter.size() > 1) {
    php_warning("delimiter must be a single character");
  }
  if (enclosure.empty()) {
    php_warning("enclosure must be a character");
    return false;
  } else if (enclosure.size() > 1) {
    php_warning("enclosure must be a single character");
  }
  int escape_char = PHP_CSV_NO_ESCAPE;
  if (!escape.empty()) {
    escape_char = static_cast<int>(escape[0]);
  } else if (escape.size() > 1) {
    php_warning("escape_char must be a single character");
  }
  char delimiter_char = delimiter[0];
  char enclosure_char = enclosure[0];
  string_buffer csvline;
  string to_enclose = string(" \t\r\n", 4).append(string(1, delimiter_char))
                                          .append(string(1, enclosure_char));
  if (escape_char != PHP_CSV_NO_ESCAPE) {
    to_enclose.append(string(1, static_cast<char>(escape_char)));
  }
  for (array<mixed>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
    if (it != fields.begin()) {
      csvline.append_char(delimiter_char);
    }
    const string &value = it.get_value().to_string();
    if (value.find_first_of(to_enclose) != string::npos) {
      bool escaped = false;
      csvline.append_char(enclosure_char);
      for (string::size_type i = 0; i < value.size(); i++) {
        char current = value[i];
        if (escape_char != PHP_CSV_NO_ESCAPE && current == escape_char) {
          escaped = true;
        } else if (!escaped && current == enclosure_char) {
          csvline.append_char(enclosure_char);
        } else {
          escaped = false;
        }
        csvline.append_char(current);
      }
      csvline.append_char(enclosure_char);
    } else {
      csvline.append(value.c_str(), value.size());
    }
  }
  csvline.append_char('\n');
  return f$fwrite(stream, csvline.str());
}

// this function is imported from https://github.com/php/php-src/blob/master/ext/standard/file.c,
// function php_fgetcsv_lookup_trailing_spaces
static const char *fgetcsv_lookup_trailing_spaces(const char *ptr, size_t len) {
  int inc_len;
  unsigned char last_chars[2] = {0, 0};

  while (len > 0) {
    inc_len = (*ptr == '\0' ? 1 : mblen(ptr, len));
    switch (inc_len) {
      case -2:
      case -1:
        inc_len = 1;
        break;
      case 0:
        goto quit_loop;
      case 1:
      default:
        last_chars[0] = last_chars[1];
        last_chars[1] = *ptr;
        break;
    }
    ptr += inc_len;
    len -= inc_len;
  }
  quit_loop:
  switch (last_chars[1]) {
    case '\n':
      if (last_chars[0] == '\r') {
        return ptr - 2;
      }
      /* fallthrough */
    case '\r':
      return ptr - 1;
  }
  return ptr;
}


Optional<array<mixed>> f$fgetcsv(const Stream &stream, int64_t length, string delimiter, string enclosure, string escape) {
  if (delimiter.empty()) {
    php_warning("delimiter must be a character");
    return false;
  } else if (delimiter.size() > 1) {
    php_warning("delimiter must be a single character");
  }
  if (enclosure.empty()) {
    php_warning("enclosure must be a character");
    return false;
  } else if (enclosure.size() > 1) {
    php_warning("enclosure must be a single character");
  }
  int escape_char = PHP_CSV_NO_ESCAPE;
  if (!escape.empty()) {
    escape_char = static_cast<int>(escape[0]);
  } else if (escape.size() > 1) {
    php_warning("escape_char must be a single character");
  }
  char delimiter_char = delimiter[0];
  char enclosure_char = enclosure[0];
  if (length < 0) {
    php_warning("Length parameter may not be negative");
    return false;
  } else if (length == 0) {
    length = -1;
  }
  Optional<string> buf_optional = length < 0 ? f$fgets(stream) : f$fgets(stream, length + 1);
  if (!buf_optional.has_value()) {
    return false;
  }
  string buffer = buf_optional.val();
  array<mixed> answer;
  int current_id = 0;
  string_buffer tmp_buffer;
  // this part is imported from https://github.com/php/php-src/blob/master/ext/standard/file.c, function php_fgetcsv
  char const *buf = buffer.c_str();
  char const *bptr = buf;
  size_t buf_len = buffer.size();
  char const *tptr = fgetcsv_lookup_trailing_spaces(buf, buf_len);
  size_t line_end_len = buf_len - (tptr - buf);
  char const *line_end = tptr, *limit = tptr;
  bool first_field = true;
  size_t temp_len = buf_len;
  int inc_len;
  do {
    char const *hunk_begin;

    inc_len = (bptr < limit ? (*bptr == '\0' ? 1 : mblen(bptr, limit - bptr)) : 0);
    if (inc_len == 1) {
      char const *tmp = bptr;
      while ((*tmp != delimiter_char) && isspace((int)*(unsigned char *)tmp)) {
        tmp++;
      }
      if (*tmp == enclosure_char) {
        bptr = tmp;
      }
    }

    if (first_field && bptr == line_end) {
      answer.set_value(current_id++, mixed());
      break;
    }
    first_field = false;
    /* 2. Read field, leaving bptr pointing at start of next field */
    if (inc_len != 0 && *bptr == enclosure_char) {
      int state = 0;

      bptr++;        /* move on to first character in field */
      hunk_begin = bptr;

      /* 2A. handle enclosure delimited field */
      for (;;) {
        switch (inc_len) {
          case 0:
            switch (state) {
              case 2:
                tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
                hunk_begin = bptr;
                goto quit_loop_2;

              case 1:
                tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
                hunk_begin = bptr;
                /* fallthrough */
              case 0: {

                if (hunk_begin != line_end) {
                  tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
                  hunk_begin = bptr;
                }

                /* add the embedded line end to the field */
                tmp_buffer.append(line_end, line_end_len);
                string new_buffer;

                if (stream.is_null()) {
                  goto quit_loop_2;
                } else {
                  Optional<string> new_buffer_optional = f$fgets(stream);
                  if (!new_buffer_optional.has_value()) {
                    if ((size_t)temp_len > (size_t)(limit - buf)) {
                      goto quit_loop_2;
                    }
                    return answer;
                  }
                  new_buffer = new_buffer_optional.val();
                }
                temp_len += new_buffer.size();
                buf_len = new_buffer.size();
                buffer = new_buffer;
                buf = bptr = buffer.c_str();
                hunk_begin = buf;

                line_end = limit = fgetcsv_lookup_trailing_spaces(buf, buf_len);
                line_end_len = buf_len - (size_t)(limit - buf);

                state = 0;
              }
                break;
            }
            break;

          case -2:
          case -1:
            /* break is omitted intentionally */
          case 1:
            /* we need to determine if the enclosure is
             * 'real' or is it escaped */
            switch (state) {
              case 1: /* escaped */
                bptr++;
                state = 0;
                break;
              case 2: /* embedded enclosure ? let's check it */
                if (*bptr != enclosure_char) {
                  /* real enclosure */
                  tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
                  hunk_begin = bptr;
                  goto quit_loop_2;
                }
                tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
                bptr++;
                hunk_begin = bptr;
                state = 0;
                break;
              default:
                if (*bptr == enclosure_char) {
                  state = 2;
                } else if (escape_char != PHP_CSV_NO_ESCAPE && *bptr == escape_char) {
                  state = 1;
                }
                bptr++;
                break;
            }
            break;

          default:
            switch (state) {
              case 2:
                /* real enclosure */
                tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin - 1));
                hunk_begin = bptr;
                goto quit_loop_2;
              case 1:
                bptr += inc_len;
                tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
                hunk_begin = bptr;
                state = 0;
                break;
              default:
                bptr += inc_len;
                break;
            }
            break;
        }
        inc_len = (bptr < limit ? (*bptr == '\0' ? 1 : mblen(bptr, limit - bptr)) : 0);
      }

      quit_loop_2:
      /* look up for a delimiter */
      for (;;) {
        switch (inc_len) {
          case 0:
            goto quit_loop_3;

          case -2:
          case -1:
            inc_len = 1;
            /* fallthrough */
          case 1:
            if (*bptr == delimiter_char) {
              goto quit_loop_3;
            }
            break;
          default:
            break;
        }
        bptr += inc_len;
        inc_len = (bptr < limit ? (*bptr == '\0' ? 1 : mblen(bptr, limit - bptr)) : 0);
      }

      quit_loop_3:
      tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));
      bptr += inc_len;
    } else {
      /* 2B. Handle non-enclosure field */

      hunk_begin = bptr;

      for (;;) {
        switch (inc_len) {
          case 0:
            goto quit_loop_4;
          case -2:
          case -1:
            inc_len = 1;
            /* fallthrough */
          case 1:
            if (*bptr == delimiter_char) {
              goto quit_loop_4;
            }
            break;
          default:
            break;
        }
        bptr += inc_len;
        inc_len = (bptr < limit ? (*bptr == '\0' ? 1 : mblen(bptr, limit - bptr)) : 0);
      }
      quit_loop_4:
      tmp_buffer.append(hunk_begin, static_cast<size_t>(bptr - hunk_begin));

      char const *comp_end = (char *)fgetcsv_lookup_trailing_spaces(tmp_buffer.c_str(), tmp_buffer.size());
      tmp_buffer.set_pos(comp_end - tmp_buffer.c_str());
      if (*bptr == delimiter_char) {
        bptr++;
      }
    }

    /* 3. Now pass our field back to php */
    answer.set_value(current_id++, tmp_buffer.str());
    tmp_buffer.clean();
  } while (inc_len > 0);

  return answer;
}

Optional<string> f$file_get_contents(const string &stream) {
  STREAM_FUNCTION_BODY(file_get_contents, false)(url);
}

Optional<int64_t> f$file_put_contents(const string &stream, const mixed &content_var, int64_t flags) {
  string content;
  if (content_var.is_array()) {
    content = f$implode(string(), content_var.to_array());
  } else {
    content = content_var.to_string();
  }

  if (flags & ~FILE_APPEND) {
    php_warning("Flags other, than FILE_APPEND are not supported in file_put_contents");
    flags &= FILE_APPEND;
  }

  STREAM_FUNCTION_BODY(file_put_contents, false)(url, content, flags);
}

static void reset_streams_global_vars() {
  hard_reset_var(error_number_dummy);
  hard_reset_var(error_description_dummy);
}

void init_streams_lib() {
  reset_streams_global_vars();
}

void free_streams_lib() {
  reset_streams_global_vars();
}
