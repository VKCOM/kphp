// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/mysql.h"

#include "server/php-queries.h"

static int mysql_callback_state;

constexpr int DB_TIMEOUT_MS = 120000;

static string *error_ptr;
static int *errno_ptr;
static int *affected_rows_ptr;
static int *insert_id_ptr;
static array<array<mixed>> *query_result_ptr;
static bool *query_id_ptr;

static int *field_cnt_ptr;
static array<string> *field_names_ptr;

static unsigned long long mysql_read_long_long(const unsigned char *&result, int &result_len, bool &is_null) {
  result_len--;
  if (result_len < 0) {
    return 0;
  }
  unsigned char c = *result;
  result++;
  if (c <= 250u) {
    return c;
  }
  unsigned long long value = 0;
  if (c == 251 || c == 255) {
    is_null = (c == 251);
    return value;
  }
  int lengths[3] = {2, 3, 8};
  int len = lengths[c - 252];
  result_len -= len;
  if (result_len < 0) {
    return 0;
  }

  int shift = 0;
  while (len--) {
    value += (unsigned long long)*result << shift;
    result++;
    shift += 8;
  }

  return value;
}

static string mysql_read_string(const unsigned char *&result, int &result_len, bool &is_null, bool need_value = false) {
  if (result_len < 0) {
    return {};
  }

  long long value_len = mysql_read_long_long(result, result_len, is_null);
  if (is_null || result_len < value_len || !need_value) {
    result_len -= (int)value_len;
    result += value_len;
    return {};
  }
  string value((const char *)result, (int)value_len);
  result_len -= (int)value_len;
  result += value_len;
  return value;
}

static void mysql_query_callback(const char *result_, int result_len) {
  //  fprintf (stderr, "%d %d\n", mysql_callback_state, result_len);
  if (!*query_id_ptr || !strcmp(result_, "ERROR\r\n")) {
    *query_id_ptr = false;
    return;
  }

  const auto *result = (const unsigned char *)result_;
  if (result_len < 4) {
    return;
  }
  int len = result[0] + (result[1] << 8) + (result[2] << 16);
  if (result_len < len + 4) {
    return;
  }
  if (len == 0) {
    *query_id_ptr = false;
    return;
  }

  bool is_null = false;
  result += 4;
  result_len -= 4;
  const unsigned char *result_end = result + len;
  switch (mysql_callback_state) {
    case 0:
      if (result[0] == 0) {
        mysql_callback_state = 5;

        ++result;
        result_len--;
        *affected_rows_ptr = (int)mysql_read_long_long(result, result_len, is_null);
        *insert_id_ptr = (int)mysql_read_long_long(result, result_len, is_null);
        if (result_len < 0 || is_null) {
          *query_id_ptr = false;
        }
        break;
      }
      if (result[0] == 255) {
        ++result;
        result_len--;
        *query_id_ptr = false;
        int message_len = len - 9;
        if (message_len < 0 || result[2] != '#') {
          return;
        }
        *errno_ptr = result[0] + (result[1] << 8);
        result += 8;
        result_len -= 8;
        error_ptr->assign((const char *)result, message_len);
        return;
      }
      if (result[0] == 254) {
        ++result;
        result_len--;
        *query_id_ptr = false;
        return;
      }

      *field_cnt_ptr = (int)mysql_read_long_long(result, result_len, is_null);
      if (result < result_end) {
        mysql_read_long_long(result, result_len, is_null);
      }
      if (result_len < 0 || is_null || result != result_end) {
        *query_id_ptr = false;
        return;
      }
      *field_names_ptr = array<string>(array_size(*field_cnt_ptr, true));

      mysql_callback_state = 1;
      break;
    case 1:
      if (result[0] == 254) {
        *query_id_ptr = false;
        return;
      }
      mysql_read_string(result, result_len, is_null);                                   // catalog
      mysql_read_string(result, result_len, is_null);                                   // db
      mysql_read_string(result, result_len, is_null);                                   // table
      mysql_read_string(result, result_len, is_null);                                   // org_table
      field_names_ptr->push_back(mysql_read_string(result, result_len, is_null, true)); // name
      mysql_read_string(result, result_len, is_null);                                   // org_name

      result_len -= 13;
      result += 13;

      if (result < result_end) {
        mysql_read_string(result, result_len, is_null); // default
      }

      if (result_len < 0 || result != result_end) {
        *query_id_ptr = false;
        return;
      }

      if (field_names_ptr->count() == *field_cnt_ptr) {
        mysql_callback_state = 2;
      }
      break;
    case 2:
      if (len != 5 || result[0] != 254) {
        *query_id_ptr = false;
        return;
      }
      result += 5;
      result_len -= 5;
      mysql_callback_state = 3;
      break;
    case 3:
      if (result[0] != 254) {
        array<mixed> row(array_size(*field_cnt_ptr, false));
        for (int i = 0; i < *field_cnt_ptr; i++) {
          is_null = false;
          mixed value = mysql_read_string(result, result_len, is_null, true);
          //          fprintf (stderr, "%p %p \"%s\" %d\n", result, result_end, value.to_string().c_str(), (int)is_null);
          if (is_null) {
            value = mixed();
          }
          if (result_len < 0 || result > result_end) {
            *query_id_ptr = false;
            return;
          }
          //          row[i] = value;
          row[field_names_ptr->get_value(i)] = value;
        }
        if (result != result_end) {
          *query_id_ptr = false;
          return;
        }
        query_result_ptr->push_back(row);

        break;
      }
      mysql_callback_state = 4;
      /* fallthrough */
    case 4:
      if (len != 5 || result[0] != 254) {
        *query_id_ptr = false;
        return;
      }
      result += 5;
      result_len -= 5;

      mysql_callback_state = 5;
      break;
    case 5:
      *query_id_ptr = false;
      break;
  }
}

static class_instance<C$mysqli> DB_Proxy;

static bool mysql_query(const class_instance<C$mysqli> &db, const string &query) {
  if (query.size() > (1 << 24) - 10) {
    return false;
  }

  db->error = string();
  db->errno_ = 0;
  db->affected_rows = 0;

  db->insert_id = 0;
  array<array<mixed>> query_result;
  bool query_id = true;

  int packet_len = query.size() + 1;
  int len = query.size() + 5;

  string real_query(len, false);
  real_query[0] = (char)(packet_len & 255);
  real_query[1] = (char)((packet_len >> 8) & 255);
  real_query[2] = (char)((packet_len >> 16) & 255);
  real_query[3] = 0;
  real_query[4] = 3;
  memcpy(&real_query[5], query.c_str(), query.size());

  error_ptr = &db->error;
  errno_ptr = &db->errno_;
  affected_rows_ptr = &db->affected_rows;
  insert_id_ptr = &db->insert_id;
  query_result_ptr = &query_result;
  query_id_ptr = &query_id;

  field_cnt_ptr = &db->field_cnt;
  field_names_ptr = &db->field_names;

  mysql_callback_state = 0;
  db_run_query(db->connection_id, real_query.c_str(), len, DB_TIMEOUT_MS, mysql_query_callback);
  if (mysql_callback_state != 5 || !query_id) {
    return false;
  }

  php_assert(db->biggest_query_id < 2000000000);
  db->query_results[++db->biggest_query_id] = query_result;
  db->cur_pos[db->biggest_query_id] = 0;

  return true;
}

string f$mysqli_error(const class_instance<C$mysqli> &db) {
  return db->error;
}

int64_t f$mysqli_errno(const class_instance<C$mysqli> &db) {
  return db->errno_;
}

int64_t f$mysqli_affected_rows(const class_instance<C$mysqli> &db) {
  if (db->connected < 0) {
    return 0;
  }
  return db->affected_rows;
}

Optional<array<mixed>> f$mysqli_fetch_array(int64_t query_id, int64_t result_type) {
  if (result_type != 1) {
    php_warning("Only MYSQL_ASSOC result_type supported in mysqli_fetch_array");
  }
  if (query_id <= 0 || query_id > DB_Proxy->biggest_query_id) {
    return Optional<array<mixed>>{};
  }

  array<array<mixed>> &query_result = DB_Proxy->query_results[query_id];
  int &cur = DB_Proxy->cur_pos[query_id];
  if (cur >= (int)query_result.count()) {
    return Optional<array<mixed>>{};
  }
  array<mixed> result = query_result[cur++];
  if (cur >= (int)query_result.count()) {
    query_result = DB_Proxy->query_results[0];
  }
  return Optional<array<mixed>>{std::move(result)};
}

int64_t f$mysqli_insert_id(const class_instance<C$mysqli> &db) {
  if (db->connected < 0) {
    return -1;
  }
  return db->insert_id;
}

int64_t f$mysqli_num_rows(int64_t query_id) {
  if (DB_Proxy.is_null()) {
    php_warning("DB object is NULL in get_num_rows");
    return 0;
  }
  if (query_id != -1 && query_id != DB_Proxy->last_query_id) {
    php_warning("mysql_num_rows is supported only for last request");
    return 0;
  }

  if (DB_Proxy->connected < 0) {
    return 0;
  }
  return DB_Proxy->query_results[static_cast<int64_t>(DB_Proxy->last_query_id)].count();
}

mixed f$mysqli_query(const class_instance<C$mysqli> &db, const string &query) {
  if (db.is_null()) {
    php_warning("DB object is NULL in mysql_query");
    return false;
  }
  bool query_id = mysql_query(db, query);
  if (!query_id) {
    return false;
  }
  return db->last_query_id = db->biggest_query_id;
}

class_instance<C$mysqli> f$mysqli_connect(const string &host __attribute__((unused)), const string &username __attribute__((unused)),
                                          const string &password __attribute__((unused)), const string &db_name __attribute__((unused)),
                                          int64_t port __attribute__((unused))) {
  // though this function is named like PHP's mysqli_connect(), it doesn't use provided credentials for connection
  // instead, they are embedded to and managed by db proxy
  // even db name is skipped: it is set as an option on server start

  if (DB_Proxy.is_null()) {
    DB_Proxy.alloc();

    DB_Proxy->cur_pos.push_back(0);
    DB_Proxy->query_results.push_back(array<array<mixed>>{});
  }

  if (DB_Proxy->connection_id < 0 && !DB_Proxy->connected) {
    DB_Proxy->connection_id = db_proxy_connect();

    if (DB_Proxy->connection_id < 0) {
      DB_Proxy->connected = -1;
    } else {
      DB_Proxy->connected = 1;
    }
  }

  if (DB_Proxy->connection_id >= 0 || DB_Proxy->connected) {
    return DB_Proxy;
  } else {
    return {};
  }
}

bool f$mysqli_select_db(const class_instance<C$mysqli> &db __attribute__((unused)), const string &name __attribute__((unused))) {
  return true;
}

static void reset_mysql_global_vars() {
  hard_reset_var(DB_Proxy);
}

void init_mysql_lib() {
  reset_mysql_global_vars();
}

void free_mysql_lib() {
  reset_mysql_global_vars();
}
