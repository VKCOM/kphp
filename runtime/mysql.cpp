#include "runtime/mysql.h"

#include "PHP/common-net-functions.h"

int mysql_callback_state;

const int DB_TIMEOUT_MS = 120000;

string *error_ptr;
int *errno_ptr;
int *affected_rows_ptr;
int *insert_id_ptr;
array<array<var>> *query_result_ptr;
bool *query_id_ptr;

int *field_cnt_ptr;
array<string> *field_names_ptr;

unsigned long long mysql_read_long_long(const unsigned char *&result, int &result_len, bool &is_null) {
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

string mysql_read_string(const unsigned char *&result, int &result_len, bool &is_null, bool need_value = false) {
  if (result_len < 0) {
    return string();
  }

  long long value_len = mysql_read_long_long(result, result_len, is_null);
  if (is_null || result_len < value_len || !need_value) {
    result_len -= (int)value_len;
    result += value_len;
    return string();
  }
  string value((const char *)result, (int)value_len);
  result_len -= (int)value_len;
  result += value_len;
  return value;
}

void mysql_query_callback(const char *result_, int result_len) {
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
      *field_names_ptr = array<string>(array_size(*field_cnt_ptr, 0, true));

      mysql_callback_state = 1;
      break;
    case 1:
      if (result[0] == 254) {
        *query_id_ptr = false;
        return;
      }
      mysql_read_string(result, result_len, is_null);//catalog
      mysql_read_string(result, result_len, is_null);//db
      mysql_read_string(result, result_len, is_null);//table
      mysql_read_string(result, result_len, is_null);//org_table
      field_names_ptr->push_back(mysql_read_string(result, result_len, is_null, true));//name
      mysql_read_string(result, result_len, is_null);//org_name

      result_len -= 13;
      result += 13;

      if (result < result_end) {
        mysql_read_string(result, result_len, is_null);//default
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
        array<var>
        row(array_size(*field_cnt_ptr,
        *field_cnt_ptr, false));
        for (int i = 0; i < *field_cnt_ptr; i++) {
          is_null = false;
          var value = mysql_read_string(result, result_len, is_null, true);
//          fprintf (stderr, "%p %p \"%s\" %d\n", result, result_end, value.to_string().c_str(), (int)is_null);
          if (is_null) {
            value = var();
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


static MyDB DB_Proxy;

db_driver::db_driver() :
  connection_id(-1),
  connected(0),
  last_query_id(0),
  biggest_query_id(0),
  error(),
  errno_(0),
  affected_rows(0),
  insert_id(0),
  field_cnt(0) {
  cur_pos.push_back(0);
  query_results.push_back(array<array<var >>());
}

void db_driver::do_connect_no_log() {
  if (connection_id >= 0 || connected) {
    return;
  }

  connection_id = db_proxy_connect();

  if (connection_id < 0) {
    connected = -1;
    return;
  }

  connected = 1;
}

var db_driver::mysql_query_update_last(const string &query_string) {
  bool query_id = mysql_query(query_string);
  if (!query_id) {
    return false;
  }
  return last_query_id = biggest_query_id;
}


int db_driver::get_affected_rows() {
  if (connected < 0) {
    return 0;
  }
  return affected_rows;
}

int db_driver::get_num_rows() {
  if (connected < 0) {
    return 0;
  }
  return query_results[last_query_id].count();
}

int db_driver::get_insert_id() {
  if (connected < 0) {
    return -1;
  }
  return insert_id;
}

bool db_driver::mysql_query(const string &query) {
  if (query.size() > (1 << 24) - 10) {
    return false;
  }

  error = string();
  errno_ = 0;
  affected_rows = 0;

  insert_id = 0;
  array<array<var>> query_result;
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

  error_ptr = &error;
  errno_ptr = &errno_;
  affected_rows_ptr = &affected_rows;
  insert_id_ptr = &insert_id;
  query_result_ptr = &query_result;
  query_id_ptr = &query_id;

  field_cnt_ptr = &field_cnt;
  field_names_ptr = &field_names;

  mysql_callback_state = 0;
  db_run_query(connection_id, real_query.c_str(), len, DB_TIMEOUT_MS, mysql_query_callback);
  if (mysql_callback_state != 5 || !query_id) {
    return false;
  }

  php_assert(biggest_query_id < 2000000000);
  query_results[++biggest_query_id] = query_result;
  cur_pos[biggest_query_id] = 0;

  return true;
}

var db_driver::mysql_fetch_array(int query_id) {
  if (query_id <= 0 || query_id > biggest_query_id) {
    return var();
  }

  array<array<var>> &query_result = query_results[query_id];
  int &cur = cur_pos[query_id];
  if (cur >= (int)query_result.count()) {
    return var();
  }
  array<var> result = query_result[cur++];
  if (cur >= (int)query_result.count()) {
    query_result = query_results[0];
  }
  return result;
}


void db_do_connect_no_log(const MyDB &db) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in DB->do_connect");
    return;
  }
  return db.db->do_connect_no_log();
}

var db_mysql_query(const MyDB &db, const string &query) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in DB->mysql_query");
    return false;
  }
  return db.db->mysql_query_update_last(query);
}

int db_get_affected_rows(const MyDB &db) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in DB->get_affected_rows");
    return 0;
  }
  return db.db->get_affected_rows();
}

int db_get_num_rows(const MyDB &db, int id) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in DB->get_num_rows");
    return 0;
  }
  if (id != -1 && id != db.db->last_query_id) {
    php_warning("mysql_num_rows is supported only for last request");
    return 0;
  }
  return db.db->get_num_rows();
}

int db_get_insert_id(const MyDB &db) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in DB->get_insert_id");
    return -1;
  }
  return db.db->get_insert_id();
}

var db_fetch_array(const MyDB &db, int query_id) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in DB->get_insert_id");
    return var();
  }
  return db.db->mysql_fetch_array(query_id);
}


bool f$boolval(const MyDB &my_db) {
  return f$boolval(my_db.bool_value);
}

bool eq2(const MyDB &my_db, bool value) {
  return my_db.bool_value == value;
}

bool eq2(bool value, const MyDB &my_db) {
  return value == my_db.bool_value;
}

bool equals(bool value, const MyDB &my_db) {
  return equals(value, my_db.bool_value);
}

bool equals(const MyDB &my_db, bool value) {
  return equals(my_db.bool_value, value);
}


MyDB &MyDB::operator=(bool value) {
  bool_value = value;
  db = nullptr;
  return *this;
}

MyDB::MyDB(bool value) :
  bool_value(value),
  db(nullptr) {}


MyDB::MyDB(db_driver *db) :
  bool_value(true),
  db(db) {
}

MyDB::MyDB() :
  bool_value(),
  db(nullptr) {
}

string f$mysqli_error(const MyDB &db) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in f$mysqli_error");
    return string();
  }
  return db.db->error;
}

int f$mysqli_errno(const MyDB &db) {
  if (db.db == nullptr) {
    php_warning("DB object is NULL in f$mysqli_errno");
    return 0;
  }
  return db.db->errno_;
}

int f$mysqli_affected_rows(const MyDB &dn) {
  return db_get_affected_rows(dn);
}

var f$mysqli_fetch_array(int query_id_var, int result_type) {
  if (result_type != 1) {
    php_warning("Only MYSQL_ASSOC result_type supported in mysqli_fetch_array");
  }
  return db_fetch_array(DB_Proxy, query_id_var);
}

int f$mysqli_insert_id(const MyDB &dn) {
  return db_get_insert_id(dn);
}

int f$mysqli_num_rows(int query_id) {
  return db_get_num_rows(DB_Proxy, query_id);
}

var f$mysqli_query(const MyDB &dn, const string &query) {
  return db_mysql_query(dn, query);
}

MyDB f$vk_mysqli_connect(const string &host __attribute__((unused)), int port __attribute__((unused))) {
  if (!f$boolval(DB_Proxy)) {
    void *buf = dl::allocate(sizeof(db_driver));
    DB_Proxy = MyDB(new(buf) db_driver());
  }
  db_do_connect_no_log(DB_Proxy);
  if (DB_Proxy.db->connection_id >= 0 || DB_Proxy.db->connected) {
    return DB_Proxy;
  } else {
    return MyDB(false);
  }
}

bool f$mysqli_select_db(const MyDB &dn __attribute__((unused)), const string &name __attribute__((unused))) {
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
