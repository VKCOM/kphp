// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/class-instance/refcountable-php-classes.h"
#include "kphp-core/kphp_core.h"
#include "runtime/dummy-visitor-methods.h"
#include "runtime/memory_usage.h"

class C$mysqli : public refcountable_php_classes<C$mysqli>, private DummyVisitorMethods {
public:
  int32_t connection_id = -1;
  int32_t connected = 0; // K.O.T.: 1 = connected, -1 = error, -2 = down
  int32_t last_query_id = 0;
  int32_t biggest_query_id = 0;
  string error{};
  int32_t errno_ = 0;
  int32_t affected_rows = 0;
  int32_t insert_id = 0;
  array<array<array<mixed>>> query_results;
  array<int32_t> cur_pos;
  int32_t field_cnt = 0;
  array<string> field_names;

  using DummyVisitorMethods::accept;

  void accept(CommonMemoryEstimateVisitor &visitor) {
    visitor("", error);
    visitor("", query_results);
    visitor("", last_query_id);
    visitor("", cur_pos);
    visitor("", field_names);
  }
};

string f$mysqli_error(const class_instance<C$mysqli> &db);

int64_t f$mysqli_errno(const class_instance<C$mysqli> &db);

int64_t f$mysqli_affected_rows(const class_instance<C$mysqli> &db);

Optional<array<mixed>> f$mysqli_fetch_array(int64_t query_id_var, int64_t result_type);

int64_t f$mysqli_insert_id(const class_instance<C$mysqli> &db);

int64_t f$mysqli_num_rows(int64_t query_id);

mixed f$mysqli_query(const class_instance<C$mysqli> &dn, const string &query);

class_instance<C$mysqli> f$mysqli_connect(const string &host, const string &username, const string &password, const string &db_name, int64_t port);

bool f$mysqli_select_db(const class_instance<C$mysqli> &db, const string &name);

void init_mysql_lib();

void free_mysql_lib();
