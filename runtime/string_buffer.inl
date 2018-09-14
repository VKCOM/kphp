#pragma once

inline void string_buffer::resize(dl::size_type new_buffer_len) {
  if (new_buffer_len < MIN_BUFFER_LEN) {
    new_buffer_len = MIN_BUFFER_LEN;
  }
  if (new_buffer_len >= MAX_BUFFER_LEN) {
    if (buffer_len + 1 < MAX_BUFFER_LEN) {
      new_buffer_len = MAX_BUFFER_LEN - 1;
    } else {
      if (string_buffer_error_flag != STRING_BUFFER_ERROR_FLAG_OFF) {
        clean();
        string_buffer_error_flag = STRING_BUFFER_ERROR_FLAG_FAILED;
        return;
      } else {
        php_critical_error ("maximum buffer size exceeded. buffer_len = %d, new_buffer_len = %d", buffer_len, new_buffer_len);
      }
    }
  }

  dl::size_type current_len = size();
  dl::static_reallocate((void **)&buffer_begin, new_buffer_len, &buffer_len);
  buffer_end = buffer_begin + current_len;
}

inline void string_buffer::reserve_at_least(dl::size_type need) {
  dl::size_type new_buffer_len = need + size();
  while (unlikely (buffer_len < new_buffer_len && string_buffer_error_flag != STRING_BUFFER_ERROR_FLAG_FAILED)) {
    resize(((new_buffer_len * 2 + 1 + 64) | 4095) - 64);
  }
}

string_buffer::string_buffer(dl::size_type buffer_len) :
  buffer_end((char *)dl::static_allocate(buffer_len)),
  buffer_begin(buffer_end),
  buffer_len(buffer_len) {
}

string_buffer &string_buffer::clean() {
  buffer_end = buffer_begin;
  return *this;
}

string_buffer &operator<<(string_buffer &sb, char c) {
  sb.reserve_at_least(1);

  *sb.buffer_end++ = c;

  return sb;
}


string_buffer &operator<<(string_buffer &sb, const char *s) {
  while (*s != 0) {
    sb.reserve_at_least(1);

    *sb.buffer_end++ = *s++;
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, double f) {
  return sb << string(f);
}

string_buffer &operator<<(string_buffer &sb, const string &s) {
  dl::size_type l = s.size();
  sb.reserve_at_least(l);

  if (unlikely (sb.string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    return sb;
  }

  memcpy(sb.buffer_end, s.c_str(), l);
  sb.buffer_end += l;

  return sb;
}

string_buffer &operator<<(string_buffer &sb, bool x) {
  if (x) {
    sb.reserve_at_least(1);
    *sb.buffer_end++ = '1';
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, int x) {
  sb.reserve_at_least(11);

  if (x < 0) {
    if (x == INT_MIN) {
      sb.append("-2147483648", 11);
      return sb;
    }
    x = -x;
    *sb.buffer_end++ = '-';
  }

  char *left = sb.buffer_end;
  do {
    *sb.buffer_end++ = (char)(x % 10 + '0');
    x /= 10;
  } while (x > 0);

  char *right = sb.buffer_end - 1;
  while (left < right) {
    char t = *left;
    *left++ = *right;
    *right-- = t;
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, unsigned int x) {
  sb.reserve_at_least(10);

  char *left = sb.buffer_end;
  do {
    *sb.buffer_end++ = (char)(x % 10 + '0');
    x /= 10;
  } while (x > 0);

  char *right = sb.buffer_end - 1;
  while (left < right) {
    char t = *left;
    *left++ = *right;
    *right-- = t;
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, long long x) {
  sb.reserve_at_least(20);

  if (x < 0) {
    if (x == (long long)9223372036854775808ull) {
      sb.append("-9223372036854775808", 20);
      return sb;
    }
    x = -x;
    *sb.buffer_end++ = '-';
  }

  char *left = sb.buffer_end;
  do {
    *sb.buffer_end++ = (char)(x % 10 + '0');
    x /= 10;
  } while (x > 0);

  char *right = sb.buffer_end - 1;
  while (left < right) {
    char t = *left;
    *left++ = *right;
    *right-- = t;
  }

  return sb;
}

string_buffer &operator<<(string_buffer &sb, unsigned long long x) {
  sb.reserve_at_least(20);

  char *left = sb.buffer_end;
  do {
    *sb.buffer_end++ = (char)(x % 10 + '0');
    x /= 10;
  } while (x > 0);

  char *right = sb.buffer_end - 1;
  while (left < right) {
    char t = *left;
    *left++ = *right;
    *right-- = t;
  }

  return sb;
}

dl::size_type string_buffer::size() const {
  return (dl::size_type)(buffer_end - buffer_begin);
}

char *string_buffer::buffer() {
  return buffer_begin;
}

const char *string_buffer::buffer() const {
  return buffer_begin;
}

const char *string_buffer::c_str() {
  reserve_at_least(1);
  *buffer_end = 0;

  return buffer_begin;
}

string string_buffer::str() const {
  php_assert (size() <= buffer_len);
  return string(buffer_begin, size());
}

bool string_buffer::set_pos(int pos) {
  php_assert ((dl::size_type)pos <= buffer_len);
  buffer_end = buffer_begin + pos;
  return true;
}

string_buffer::~string_buffer() {
  dl::static_deallocate((void **)&buffer_begin, &buffer_len);
}

string_buffer &string_buffer::append(const char *str, int len) {
  reserve_at_least(len);

  if (unlikely (string_buffer_error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    return *this;
  }
  memcpy(buffer_end, str, len);
  buffer_end += len;

  return *this;
}

void string_buffer::append_unsafe(const char *str, int len) {
  memcpy(buffer_end, str, len);
  buffer_end += len;
}

void string_buffer::append_char(char c) {
  *buffer_end++ = c;
}


void string_buffer::reserve(int len) {
  reserve_at_least(len + 1);
}

inline void string_buffer_init_static(int max_length) {
  string_buffer::MIN_BUFFER_LEN = 266175;
  string_buffer::MAX_BUFFER_LEN = (1 << 24);
  if (max_length > 0) {
    string_buffer::MAX_BUFFER_LEN = max_length;
  }
}
