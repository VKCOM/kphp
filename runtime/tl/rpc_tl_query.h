// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"

class RpcRequestResult;

struct RpcTlQuery : refcountable_php_classes<RpcTlQuery> {
  std::unique_ptr<RpcRequestResult> result_fetcher;
  string tl_function_name;
  int64_t query_id{0};
};

class RpcPendingQueries {
public:
  void save(const class_instance<RpcTlQuery> &query);
  class_instance<RpcTlQuery> withdraw(int64_t query_id);

  void hard_reset();

  static RpcPendingQueries &get() {
    static RpcPendingQueries queries;
    return queries;
  }

  int64_t count() const { return queries_.count(); }

private:
  RpcPendingQueries() = default;

  array<class_instance<RpcTlQuery>> queries_;
};

class CurrentProcessingQuery {
public:
  static CurrentProcessingQuery &get() {
    static CurrentProcessingQuery context;
    return context;
  }

  void reset();
  void set_current_tl_function(const string &tl_function_name);
  void set_current_tl_function(const class_instance<RpcTlQuery> &current_query);
  void raise_fetching_error(const char *format, ...) __attribute__ ((format (printf, 2, 3)));
  void raise_storing_error(const char *format, ...) __attribute__ ((format (printf, 2, 3)));

  // called from generated TL serializers (from autogen)
  void set_last_stored_tl_function_magic(uint32_t tl_magic) { last_stored_tl_function_magic_ = tl_magic; }
  uint32_t get_last_stored_tl_function_magic() const { return last_stored_tl_function_magic_; }

  const string &get_current_tl_function_name() const {
    return current_tl_function_name_;
  }

private:
  string current_tl_function_name_;
  uint32_t last_stored_tl_function_magic_{0};
};
