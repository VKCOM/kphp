#pragma once

#ifndef INCLUDED_FROM_KPHP_CORE
  #error "this file must be included only from kphp_core.h"
#endif

const string::size_type string::max_size __attribute__ ((weak));

string::size_type string::string_inner::empty_string_storage[sizeof(string_inner) / sizeof(size_type) + 1] __attribute__ ((weak)) = {0 /* size */,
                                                                                                                                     0 /* capacity */,
                                                                                                                                     REF_CNT_FOR_CONST};

string::string_inner &string::string_inner::empty_string() {
  return *reinterpret_cast <string_inner *> (empty_string_storage);
}

bool string::string_inner::is_shared() const {
  return ref_count > 0;
}

void string::string_inner::set_length_and_sharable(size_type n) {
//  fprintf (stderr, "inc ref cnt %d %s\n", 0, ref_data());
  ref_count = 0;
  size = n;
  ref_data()[n] = '\0';
}

char *string::string_inner::ref_data() const {
  return (char *)(this + 1);
}

string::size_type string::string_inner::new_capacity(size_type requested_capacity, size_type old_capacity) {
  if (requested_capacity > max_size) {
    php_critical_error ("tried to allocate too big string of size %lld", (long long)requested_capacity);
  }

  if (requested_capacity > old_capacity && requested_capacity < 2 * old_capacity) {
    requested_capacity = 2 * old_capacity;
  }

  const size_type page_size = 4096;
  size_type new_size = (size_type)((requested_capacity + 1) + sizeof(string_inner));

  if (new_size > page_size && requested_capacity > old_capacity) {
    requested_capacity += (new_size + page_size - 1) / page_size * page_size - new_size;
  }

  return requested_capacity;
}


string::string_inner *string::string_inner::create(size_type requested_capacity, size_type old_capacity) {
/*
  if (requested_capacity == 0) {
    return &empty_string();
  }
*/

  size_type capacity = new_capacity(requested_capacity, old_capacity);
  size_type new_size = (size_type)(sizeof(string_inner) + (capacity + 1));
  string_inner *p = (string_inner *)dl::allocate(new_size);
  p->capacity = capacity;
  return p;
}

char *string::string_inner::reserve(size_type requested_capacity) {
  size_type new_cap = new_capacity(requested_capacity, capacity);
  size_type old_size = (size_type)(sizeof(string_inner) + (capacity + 1));
  size_type new_size = (size_type)(sizeof(string_inner) + (new_cap + 1));

  string_inner *p = (string_inner *)dl::reallocate((void *)this, new_size, old_size);
  p->capacity = new_cap;
  return p->ref_data();
}

void string::string_inner::dispose() {
//  fprintf (stderr, "dec ref cnt %d %s\n", ref_count - 1, ref_data());
  if (ref_count != REF_CNT_FOR_CONST) {
    ref_count--;
    if (ref_count <= -1) {
      destroy();
    }
  }
}

void string::string_inner::destroy() {
  dl::deallocate((void *)this, (size_type)(sizeof(string_inner) + (capacity + 1)));
}


char *string::string_inner::ref_copy() {
//  fprintf (stderr, "inc ref cnt %d, %s\n", ref_count + 1, ref_data());
  if (ref_count != REF_CNT_FOR_CONST) {
    ref_count++;
  }
  return ref_data();
}

char *string::string_inner::clone(size_type requested_cap) {
  string_inner *r = string_inner::create(requested_cap, capacity);
  if (size) {
    memcpy(r->ref_data(), ref_data(), size);
  }

  r->set_length_and_sharable(size);
  return r->ref_data();
}


string::string_inner *string::inner() const {
  return (string::string_inner *)p - 1;
}

bool string::disjunct(const char *s) const {
  return (s < p || p + size() < s);
}

void string::set_size(size_type new_size) {
  if (inner()->is_shared()) {
    string_inner *r = string_inner::create(new_size, capacity());

    inner()->dispose();
    p = r->ref_data();
  } else if (new_size > capacity()) {
    p = inner()->reserve(new_size);
  }
  inner()->set_length_and_sharable(new_size);
}

string::string_inner &string::empty_string() {
  return string_inner::empty_string();
}


char *string::create(const char *beg, const char *end) {
  if (beg == end) {
    return empty_string().ref_data();
  }

  const size_type dnew = (size_type)(end - beg);

  string_inner *r = string_inner::create(dnew, false);
  char *s = r->ref_data();
  memcpy(s, beg, dnew);

  r->ref_count = 0;
  r->size = dnew;
  s[dnew] = '\0';
  return s;
}

char *string::create(size_type n, char c) {
  if (n == 0) {
    return empty_string().ref_data();
  }

  string_inner *r = string_inner::create(n, 0);
  char *s = r->ref_data();
  if (n == 1) {
    *s = c;
  } else {
    memset(s, c, n);
  }

  r->set_length_and_sharable(n);
  return s;
}

char *string::create(size_type n, bool b) {
  if (n == 0) {
    return empty_string().ref_data();
  }

  string_inner *r = string_inner::create(n, 0);

  if (b) {
    r->ref_count = 0;
    r->size = 0;
  } else {
    r->set_length_and_sharable(n);
  }
  return r->ref_data();
}

string::string() :
  p(empty_string().ref_data()) {
}

string::string(const string &str) :
  p(str.inner()->ref_copy()) {
}

string::string(string &&str) noexcept :
  p(str.p) {
  str.p = nullptr;
}

string::string(const char *s, size_type n) :
  p(create(s, s + n)) {
}

string::string(const char *s) :
  string(s, strlen(s)) {}

string::string(size_type n, char c) :
  p(create(n, c)) {
}

string::string(size_type n, bool b) :
  p(create(n, b)) {
}

string::string(int i) {
  static const unsigned int STRING_POOL_SIZE = 10000u;
  static void *str_pool[STRING_POOL_SIZE];

  static long long last_query_num = -1;
  if (dl::query_num != last_query_num) {
    memset(str_pool, 0, sizeof(str_pool[0]) * STRING_POOL_SIZE);
    last_query_num = dl::query_num;
  }

  if ((unsigned int)i < STRING_POOL_SIZE && str_pool[i] != NULL) {
    p = (*(string *)&str_pool[i]).inner()->ref_copy();
    return;
  }

  int neg = 0;

  int x = i;
  if (i < 0) {
    if (i == -2147483648) {
      const char *s = "-2147483648";
      p = create(s, s + strlen(s));
      return;
    }
    i = -i;
    neg = 1;
  }

  char buffer[STRLEN_INT];
  int cur_pos = STRLEN_INT;
  do {
    buffer[--cur_pos] = (char)(i % 10 + '0');
    i /= 10;
  } while (i > 0);

  if (neg) {
    buffer[--cur_pos] = '-';
  }

  if ((unsigned int)x < STRING_POOL_SIZE) {
    new(&str_pool[x]) string(buffer + cur_pos, STRLEN_INT - cur_pos);
    p = (*(string *)&str_pool[x]).inner()->ref_copy();
  } else {
    p = create(buffer + cur_pos, buffer + STRLEN_INT);
  }
}

string::string(double f) {
#define MAX_LEN 4096
  static char result[MAX_LEN + 2];
  char *begin = result + 2;
  int len = snprintf(begin, MAX_LEN, "%.14G", f);
  if (len < MAX_LEN) {
    if (len == 2 && begin[0] == '-' && begin[1] == '0') {
      begin++;
      len--;
    } else if ((unsigned int)(begin[len - 1] - '5') < 5 && begin[len - 2] == '0' && begin[len - 3] == '-') {
      --len;
      begin[len - 1] = begin[len];
    }
    if (begin[1] == 'E') {
      begin[-2] = begin[0];
      begin[-1] = '.';
      begin[0] = '0';
      begin -= 2;
      len += 2;
    } else if (begin[0] == '-' && begin[2] == 'E') {
      begin[-2] = begin[0];
      begin[-1] = begin[1];
      begin[0] = '.';
      begin[1] = '0';
      begin -= 2;
      len += 2;
    }
    p = create(begin, begin + len);
    php_assert (len <= STRLEN_FLOAT);
  } else {
    php_warning("Maximum length of float (%d) exceeded", MAX_LEN);
    p = empty_string().ref_data();
  }
#undef MAX_LEN
}


string::~string() {
  destroy();
}

string &string::operator=(const string &str) {
  return assign(str);
}

string &string::operator=(string &&str) noexcept {
  char *str_copy = str.p;
  destroy();
  p = str_copy;
  str.p = nullptr;

  return *this;
}

string::size_type string::size() const {
  return inner()->size;
}

void string::shrink(size_type n) {
  const size_type old_size = size();

  if (n < old_size) {
    if (inner()->is_shared()) {
      string_inner *r = string_inner::create(n, capacity());

      if (n) {
        memcpy(r->ref_data(), p, n);
      }

      inner()->dispose();
      p = r->ref_data();
    }
    inner()->set_length_and_sharable(n);
  }
}

string::size_type string::capacity() const {
  return inner()->capacity;
}

void string::make_not_shared() {
  if (inner()->is_shared()) {
    force_reserve(size());
  }
}

void string::force_reserve(size_type res) {
  char *new_p = inner()->clone(res);
  inner()->dispose();
  p = new_p;
}

string &string::reserve_at_least(size_type res) {
  if (inner()->is_shared()) {
    force_reserve(res);
  } else if (res > capacity()) {
    p = inner()->reserve(res);
  }
  return *this;
}

bool string::empty() const {
  return size() == 0;
}

const char &string::operator[](size_type pos) const {
  return p[pos];
}

char &string::operator[](size_type pos) {
  return p[pos];
}


string &string::append(const string &str) {
  const size_type n2 = str.size();
  if (n2) {
    const size_type len = n2 + size();
    reserve_at_least(len);

    memcpy(p + size(), str.p, n2);
    inner()->set_length_and_sharable(len);
  }
  return *this;
}

string &string::append(const string &str, size_type pos2, size_type n2) {
  if (pos2 > str.size()) {
    pos2 = str.size();
  }
  if (n2 > str.size() - pos2) {
    n2 = str.size() - pos2;
  }
  if (n2) {
    const size_type len = n2 + size();
    reserve_at_least(len);

    memcpy(p + size(), str.p + pos2, n2);
    inner()->set_length_and_sharable(len);
  }
  return *this;
}

string &string::append(const char *s, size_type n) {
  if (n) {
    if (max_size - size() < n) {
      php_critical_error ("tried to allocate too big string of size %lld", (long long)size() + n);
    }
    const size_type len = n + size();
    if (len > capacity() || inner()->is_shared()) {
      if (disjunct(s)) {
        force_reserve(len);
      } else {
        const size_type off = (size_type)(s - p);
        force_reserve(len);
        s = p + off;
      }
    }
    memcpy(p + size(), s, n);
    inner()->set_length_and_sharable(len);
  }

  return *this;
}

string &string::append(size_type n, char c) {
  if (n) {
    if (max_size - size() < n) {
      php_critical_error ("tried to allocate too big string of size %lld", (long long)size() + n);
    }
    const size_type len = n + size();
    reserve_at_least(len);
    memset(p + size(), c, n);
    inner()->set_length_and_sharable(len);
  }

  return *this;
}

string &string::append(bool b) {
  if (b) {
    push_back('1');
  }

  return *this;
}

string &string::append(int i) {
  dl::size_type cur_pos = size();
  reserve_at_least(cur_pos + STRLEN_INT);

  if (i < 0) {
    if (i == INT_MIN) {
      append("-2147483648", 11);
      return *this;
    }
    i = -i;
    p[cur_pos++] = '-';
  }

  dl::size_type left = cur_pos;
  do {
    p[cur_pos++] = (char)(i % 10 + '0');
    i /= 10;
  } while (i > 0);

  dl::size_type right = cur_pos - 1;
  while (left < right) {
    char t = p[left];
    p[left++] = p[right];
    p[right--] = t;
  }

  inner()->set_length_and_sharable(cur_pos);
  return *this;
}

string &string::append(double d) {
  return append(string(d));
}

string &string::append(const var &v) {
  switch (v.type) {
    case var::NULL_TYPE:
      return *this;
    case var::BOOLEAN_TYPE:
      if (v.as_bool()) {
        push_back('1');
      }
      return *this;
    case var::INTEGER_TYPE:
      return append(v.as_int());
    case var::FLOAT_TYPE:
      return append(string(v.as_double()));
    case var::STRING_TYPE:
      return append(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Convertion from array to string");
      return append("Array", 5);
    default:
      php_assert (0);
      exit(1);
  }
}

void string::push_back(char c) {
  const size_type len = 1 + size();
  reserve_at_least(len);
  p[len - 1] = c;
  inner()->set_length_and_sharable(len);
}


string &string::append_unsafe(bool b) {
  if (b) {
    p[inner()->size++] = '1';
  }
  return *this;
}

string &string::append_unsafe(int i) {
  dl::size_type cur_pos = size();
  if (i < 0) {
    if (i == INT_MIN) {
      append_unsafe("-2147483648", 11);
      return *this;
    }
    i = -i;
    p[cur_pos++] = '-';
  }

  dl::size_type left = cur_pos;
  do {
    p[cur_pos++] = (char)(i % 10 + '0');
    i /= 10;
  } while (i > 0);

  dl::size_type right = cur_pos - 1;
  while (left < right) {
    char t = p[left];
    p[left++] = p[right];
    p[right--] = t;
  }
  inner()->size = cur_pos;

  return *this;
}

string &string::append_unsafe(double d) {
  return append_unsafe(string(d));//TODO can be optimized
}

string &string::append_unsafe(const string &str) {
  const size_type n2 = str.size();
  memcpy(p + size(), str.p, n2);
  inner()->size += n2;

  return *this;
}

string &string::append_unsafe(const char *s, size_type n) {
  memcpy(p + size(), s, n);
  inner()->size += n;

  return *this;
}

template<class T>
string &string::append_unsafe(const array<T> &a __attribute__((unused))) {
  php_warning("Convertion from array to string");
  return append_unsafe("Array", 5);
}

string &string::append_unsafe(const var &v) {
  switch (v.type) {
    case var::NULL_TYPE:
      return *this;
    case var::BOOLEAN_TYPE:
      return append_unsafe(v.as_bool());
    case var::INTEGER_TYPE:
      return append_unsafe(v.as_int());
    case var::FLOAT_TYPE:
      return append_unsafe(v.as_double());
    case var::STRING_TYPE:
      return append_unsafe(v.as_string());
    case var::ARRAY_TYPE:
      return append_unsafe(v.as_array());
    default:
      php_assert (0);
      exit(1);
  }
}

string &string::finish_append() {
  php_assert (inner()->size <= inner()->capacity);
  p[inner()->size] = '\0';
  return *this;
}

template<class T>
string &string::append_unsafe(const OrFalse<T> &v) {
  if (v.bool_value) {
    return append_unsafe(v.value);
  }
  return *this;
}


string &string::assign(const string &str) {
  char *str_copy = str.inner()->ref_copy();
  destroy();
  p = str_copy;
  return *this;
}

string &string::assign(const string &str, size_type pos, size_type n) {
  if (pos > str.size()) {
    pos = str.size();
  }
  if (n > str.size() - pos) {
    n = str.size() - pos;
  }
  return assign(str.p + pos, n);
}

string &string::assign(const char *s, size_type n) {
  if (max_size < n) {
    php_critical_error ("tried to allocate too big string of size %lld", (long long)n);
  }

  if (disjunct(s) || inner()->is_shared()) {
    set_size(n);
    memcpy(p, s, n);
  } else {
    const size_type pos = (size_type)(s - p);
    if (pos >= n) {
      memcpy(p, s, n);
    } else if (pos) {
      memmove(p, s, n);
    }
    inner()->set_length_and_sharable(n);
  }
  return *this;
}

string &string::assign(size_type n, char c) {
  if (max_size < n) {
    php_critical_error ("tried to allocate too big string of size %lld", (long long)n);
  }

  set_size(n);
  memset(p, c, n);

  return *this;
}

string &string::assign(size_type n, bool b __attribute__((unused))) {
  if (max_size < n) {
    php_critical_error ("tried to allocate too big string of size %lld", (long long)n);
  }

  set_size(n);

  return *this;
}


void string::assign_raw(const char *s) {
  php_assert (sizeof(string_inner) == 12u);
  p = const_cast <char *> (s + sizeof(string_inner));
}


void string::swap(string &s) {
  char *tmp = p;
  p = s.p;
  s.p = tmp;
}

char *string::buffer() {
  return p;
}

const char *string::c_str() const {
  return p;
}


inline void string::warn_on_float_conversion() const {
  const char *s = c_str();
  while (isspace(*s)) {
    s++;
  }

  if (s[0] == '-' || s[0] == '+') {
    s++;
  }

  int i = 0;
  while (i < 15) {
    if (s[i] == 0) {
      return;
    }
    if ('0' <= s[i] && s[i] <= '9') {
      i++;
      continue;
    }

    if (s[i] == '.') {
      do {
        i++;
      } while (s[i] == '0');

      if (s[i] == 0) {
        return;
      }

      while (i < 16) {
        if (s[i] == 0) {
          return;
        }

        if ('0' <= s[i] && s[i] <= '9') {
          i++;
          continue;
        }
        break;
      }
    }
    break;
  }

  php_warning("Possible loss of precision on converting string \"%s\" to float", c_str());
}


inline bool string::try_to_int(int *val) const {
  return php_try_to_int(p, size(), val);
}

bool string::try_to_float(double *val) const {
  if (empty()) {
    return false;
  }
  char *end_ptr = NULL;
  *val = strtod(p, &end_ptr);
  return (end_ptr == p + size());
}


var string::to_numeric() const {
  char *end_ptr;
  double res = strtod(p, &end_ptr);
  if (end_ptr == p) {
    return 0;
  }
  int int_res = (int)res;
  if (int_res == res) {
    return int_res;
  }
  return res;
}

bool string::to_bool() const {
  int l = size();
  return l >= 2 || (l == 1 && p[0] != '0');
}

int string::to_int(const char *s, int l) {
  int mul = 1, cur = 0;
  if (l > 0 && (s[0] == '-' || s[0] == '+')) {
    if (s[0] == '-') {
      mul = -1;
    }
    cur++;
  }

  int val = 0;
  while (cur < l && '0' <= s[cur] && s[cur] <= '9') {
    val = val * 10 + s[cur++] - '0';
  }

//  if (cur < l) {
//    php_warning ("Part of the string \"%s\" was ignored while converting to int", c_str());
//  }
  return val * mul;
}


int string::to_int() const {
  return to_int(p, size());
}

double string::to_float() const {
  char *end_ptr = NULL;
  double res = strtod(p, &end_ptr);
//  if (*end_ptr) {
//    php_warning ("Part of the string \"%s\" was ignored while converting to float", c_str());
//  }
  if (end_ptr == p) {
    return 0.0;
  }
  return res;
}

const string &string::to_string() const {
  return *this;
}


int string::safe_to_int() const {
  int mul = 1, l = size(), cur = 0;
  if (l > 0 && (p[0] == '-' || p[0] == '+')) {
    if (p[0] == '-') {
      mul = -1;
    }
    cur++;
  }

  int val = 0;
  double dval = 0;
  while (cur < l && '0' <= p[cur] && p[cur] <= '9') {
    val = val * 10 + p[cur] - '0';
    dval = dval * 10 + (p[cur++] - '0');
  }

  if (val != dval) {
    php_warning("Integer overflow on converting string \"%s\" to int", c_str());
  }
  return val * mul;
}


bool string::is_int() const {
  return php_is_int(p, size());
}


bool string::is_numeric() const {
  const char *s = c_str();
  while (isspace(*s)) {
    s++;
  }

  if (*s == '0' && s[1] == 'x') {
    s += 2;
    while (*s) {
      if (!dl::is_hex_digit(*s++)) {
        return false;
      }
    }
    return true;
  }

  if (*s == '+' || *s == '-') {
    s++;
  }

  int l = 0;
  while (dl::is_decimal_digit(*s)) {
    l++;
    s++;
  }

  if (*s == '.') {
    s++;
    while (dl::is_decimal_digit(*s)) {
      l++;
      s++;
    }
  }

  if (l == 0) {
    return false;
  }

  if (*s == 'e' || *s == 'E') {
    s++;
    if (*s == '+' || *s == '-') {
      s++;
    }

    if (*s == '\0') {
      return false;
    }

    while (dl::is_decimal_digit(*s)) {
      s++;
    }
  }

  return *s == '\0';
}

int string::hash() const {
  return string_hash(p, size());
}


string string::substr(size_type pos, size_type n) const {
  return string(p + pos, n);
}

string::size_type string::find(const string &s, size_type pos) const {
  for (size_type i = pos; i + s.size() <= size(); i++) {
    bool equal = true;
    for (size_type j = 0; j < s.size(); j++) {
      if (p[i + j] != s.p[j]) {
        equal = false;
        break;
      }
    }
    if (equal) {
      return i;
    }
  }
  return string::npos;
}

string::size_type string::find_first_of(const string &s, size_type pos) const {
  for (size_type i = pos; i < size(); i++) {
    for (size_type j = 0; j < s.size(); j++) {
      if (p[i] == s.p[j]) {
        return i;
      }
    }
  }
  return string::npos;
}

int string::compare(const string &str) const {
  dl::size_type my_size = size();
  dl::size_type str_size = str.size();
  int diff = (int)my_size - (int)str_size;
  int res = memcmp(p, str.p, (diff < 0 ? my_size + 1 : str_size + 1));
  return res ? res : diff;
}


const string string::get_value(int int_key) const {
  if ((dl::size_type)int_key >= size()) {
    return string();
  }
  return string(1, p[int_key]);
}

const string string::get_value(const string &string_key) const {
  int int_val;
  if (!string_key.try_to_int(&int_val)) {
    php_warning("\"%s\" is illegal offset for string", string_key.c_str());
    int_val = string_key.to_int();
  }
  if ((dl::size_type)int_val >= size()) {
    return string();
  }
  return string(1, p[int_val]);
}

const string string::get_value(const var &v) const {
  switch (v.type) {
    case var::NULL_TYPE:
      return get_value(0);
    case var::BOOLEAN_TYPE:
      return get_value(v.as_bool());
    case var::INTEGER_TYPE:
      return get_value(v.as_int());
    case var::FLOAT_TYPE:
      return get_value((int)v.as_double());
    case var::STRING_TYPE:
      return get_value(v.as_string());
    case var::ARRAY_TYPE:
      php_warning("Illegal offset type %s", v.get_type_c_str());
      return string();
    default:
      php_assert (0);
      exit(1);
  }
}


int string::get_reference_counter() const {
  return inner()->ref_count + 1;
}

inline void string::set_reference_counter_to_const() {
  inner()->ref_count = REF_CNT_FOR_CONST;
}

inline void string::destroy() {
  if (p) {
    inner()->dispose();
  }
}


bool operator==(const string &lhs, const string &rhs) {
  return lhs.compare(rhs) == 0;
}

bool operator!=(const string &lhs, const string &rhs) {
  return lhs.compare(rhs) != 0;
}

bool is_ok_float(double v) {
  return v == 0.0 || (__fpclassify(v) == FP_NORMAL && v < 1e100 && v > -1e100);
}

inline bool is_all_digits(const string &s) {
  for (size_t i = 0; i < s.size(); ++i) {
    if (!('0' <= s[i] && s[i] <= '9')) {
      return false;
    }
  }

  return true;
}

int compare_strings_php_order(const string &lhs, const string &rhs) {
  if (lhs.size() == rhs.size() && is_all_digits(lhs) && is_all_digits(rhs)) {
    return lhs.compare(rhs);
  }

  // possible numbers: -.1e2; 0.12; 123; .12; +123; +.123
  bool lhs_maybe_num = lhs[0] <= '9';
  bool rhs_maybe_num = rhs[0] <= '9';

  if (lhs_maybe_num && rhs_maybe_num) {
    int lhs_int_val, rhs_int_val;
    if (lhs.try_to_int(&lhs_int_val) && rhs.try_to_int(&rhs_int_val)) {
      return three_way_comparison(lhs_int_val, rhs_int_val);
    }

    double lhs_float_val, rhs_float_val;
    if (lhs.try_to_float(&lhs_float_val) && rhs.try_to_float(&rhs_float_val)) {
      lhs.warn_on_float_conversion();
      rhs.warn_on_float_conversion();
      if (is_ok_float(lhs_float_val) && is_ok_float(rhs_float_val)) {
        return three_way_comparison(lhs_float_val, rhs_float_val);
      }
    }
  }

  return lhs.compare(rhs);
}

inline bool eq2(const string &lhs, const string &rhs) {
  return compare_strings_php_order(lhs, rhs) == 0;
}

bool neq2(const string &lhs, const string &rhs) {
  return !eq2(lhs, rhs);
}


void swap(string &lhs, string &rhs) {
  lhs.swap(rhs);
}


dl::size_type max_string_size(bool) {
  return STRLEN_BOOL;
}

dl::size_type max_string_size(int) {
  return STRLEN_INT;
}

dl::size_type max_string_size(double) {
  return STRLEN_FLOAT;
}

dl::size_type max_string_size(const string &s) {
  return s.size();
}

dl::size_type max_string_size(const var &v) {
  switch (v.type) {
    case var::NULL_TYPE:
      return 0;
    case var::BOOLEAN_TYPE:
      return max_string_size(v.as_bool());
    case var::INTEGER_TYPE:
      return max_string_size(v.as_int());
    case var::FLOAT_TYPE:
      return max_string_size(v.as_double());
    case var::STRING_TYPE:
      return max_string_size(v.as_string());
    case var::ARRAY_TYPE:
      return max_string_size(v.as_array());
    default:
      php_assert (0);
      exit(1);
  }
}

template<class T>
dl::size_type max_string_size(const array<T> &) {
  return STRLEN_ARRAY;
}

template<class T>
dl::size_type max_string_size(const OrFalse<T> &v) {
  return v.bool_value ? max_string_size(v.value) : 0;
}
