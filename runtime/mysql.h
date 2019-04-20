#pragma once

#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"

class C$mysqli : public refcountable_php_classes<C$mysqli> {
public:
  int connection_id = -1;
  int connected = 0; // K.O.T.: 1 = connected, -1 = error, -2 = down
  int last_query_id = 0;
  int biggest_query_id = 0;
  string error{};
  int errno_ = 0;
  int affected_rows = 0;
  int insert_id = 0;
  array<array<array<var>>> query_results;
  array<int> cur_pos;
  int field_cnt = 0;
  array<string> field_names;
};

string f$mysqli_error(const class_instance<C$mysqli> &db);

int f$mysqli_errno(const class_instance<C$mysqli> &db);

int f$mysqli_affected_rows(const class_instance<C$mysqli> &db);

var f$mysqli_fetch_array(int query_id_var, int result_type);

int f$mysqli_insert_id(const class_instance<C$mysqli> &db);

int f$mysqli_num_rows(int query_id);

var f$mysqli_query(const class_instance<C$mysqli> &dn, const string &query);

class_instance<C$mysqli> f$vk_mysqli_connect(const string &host, int port);

bool f$mysqli_select_db(const class_instance<C$mysqli> &db, const string &name);

void init_mysql_lib();

void free_mysql_lib();
