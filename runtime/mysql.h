#pragma once

#include "runtime/kphp_core.h"

class MyDB;

class db_driver {
private:
  int connection_id;
  int connected; // K.O.T.: 1 = connected, -1 = error, -2 = down

  int last_query_id;
  int biggest_query_id;
  string error;
  int errno_;
  int affected_rows;
  int insert_id;
  array<array<array<var>>>
    query_results;
  array<int> cur_pos;

  int field_cnt;
  array<string> field_names;

public:
  explicit db_driver();

  void do_connect_no_log();

  int get_affected_rows();

  int get_num_rows();

  int get_insert_id();

  bool mysql_query(const string &query);

  var mysql_query_update_last(const string &query_string);

  var mysql_fetch_array(int query_id);


  friend string f$mysqli_error(const MyDB &db);

  friend int f$mysqli_errno(const MyDB &db);

  friend int db_get_num_rows(const MyDB &db, int id);

  friend MyDB f$vk_mysqli_connect(const string &host __attribute__((unused)), int port __attribute__((unused)));
};

class MyDB {
private:
  bool bool_value;
  db_driver *db;

  explicit MyDB(db_driver *db);
public:
  MyDB();

  friend void db_do_connect_no_log(const MyDB &db);

  friend var db_mysql_query(const MyDB &db, const string &query);

  friend int db_get_affected_rows(const MyDB &db);

  friend int db_get_insert_id(const MyDB &db);

  friend var db_fetch_array(const MyDB &db, int query_id);

  friend bool f$boolval(const MyDB &my_db);
  friend bool eq2(const MyDB &my_db, bool value);
  friend bool eq2(bool value, const MyDB &my_db);
  friend bool equals(bool value, const MyDB &my_db);
  friend bool equals(const MyDB &my_db, bool value);

  MyDB &operator=(bool value);
  explicit MyDB(bool value);

  friend MyDB f$vk_mysqli_connect(const string &host, int port);

  friend int db_get_num_rows(const MyDB &db, int id);

  friend string f$mysqli_error(const MyDB &db);

  friend int f$mysqli_errno(const MyDB &db);
};

int db_get_affected_rows(const MyDB &db);

int db_get_num_rows(const MyDB &db, int id = -1);

int db_get_insert_id(const MyDB &db);

bool f$boolval(const MyDB &my_db);
bool eq2(const MyDB &my_db, bool value);
bool eq2(bool value, const MyDB &my_db);
bool equals(bool value, const MyDB &my_db);
bool equals(const MyDB &my_db, bool value);


string f$mysqli_error(const MyDB &db);

int f$mysqli_errno(const MyDB &db);


int f$mysqli_affected_rows(const MyDB &dn);

var f$mysqli_fetch_array(int query_id_var, int result_type);

int f$mysqli_insert_id(const MyDB &dn);

int f$mysqli_num_rows(int query_id);

var f$mysqli_query(const MyDB &dn, const string &query);

MyDB f$vk_mysqli_connect(const string &host, int port);

bool f$mysqli_select_db(const MyDB &dn, const string &name);

void init_mysql_lib();

void free_mysql_lib();
