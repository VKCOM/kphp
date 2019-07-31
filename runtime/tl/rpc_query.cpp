#include "runtime/tl/rpc_query.h"

#include <cstdarg>

#include "runtime/exception.h"
#include "runtime/tl/rpc_request.h"

void RpcPendingQueries::save(const class_instance<RpcQuery> &query) {
  php_assert(!queries_.has_key(query.get()->query_id));
  queries_.set_value(query.get()->query_id, query);
}

class_instance<RpcQuery> RpcPendingQueries::withdraw(int query_id) {
  auto query = queries_.get_value(query_id);
  queries_.unset(query_id);
  return query;
}

void RpcPendingQueries::hard_reset() {
  hard_reset_var(queries_);
}

void CurrentProcessingQuery::reset() {
  current_tl_function_name_ = string();
}

void CurrentProcessingQuery::set_current_tl_function(const string &tl_function_name) {
  php_assert(current_tl_function_name_.empty());
  current_tl_function_name_ = tl_function_name;
}

void CurrentProcessingQuery::set_current_tl_function(const class_instance<RpcQuery> &current_query) {
  php_assert(current_tl_function_name_.empty());
  current_tl_function_name_ = current_query.get()->tl_function_name;
}

void CurrentProcessingQuery::raise_fetching_error(const char *format, ...) {
  php_assert(!current_tl_function_name_.empty());
  if (CurException.is_null()) {
    constexpr size_t BUFF_SZ = 1024;
    char buff[BUFF_SZ] = {0};
    va_list args;
    va_start(args, format);
    int sz = vsnprintf(buff, BUFF_SZ, format, args);
    php_assert(sz > 0);
    va_end(args);
    string msg = string(buff, static_cast<size_t>(sz));
    php_warning("Fetching error:\n%s\nDeserializing result of function: %s", msg.c_str(), current_tl_function_name_.c_str());
    msg.append(string(" in result of ")).append(current_tl_function_name_);
    THROW_EXCEPTION(new_Exception(string{}, 0, msg, -1));
  }
}

void CurrentProcessingQuery::raise_storing_error(const char *format, ...) {
  const char *function_name = current_tl_function_name_.empty() ? "_unknown_" : current_tl_function_name_.c_str();
  if (CurException.is_null()) {
    constexpr size_t BUFF_SZ = 1024;
    char buff[BUFF_SZ] = {0};
    va_list args;
    va_start (args, format);
    int sz = vsnprintf(buff, BUFF_SZ, format, args);
    php_assert(sz > 0);
    va_end(args);
    string msg = string(buff, static_cast<size_t>(sz));
    php_warning("Storing error:\n%s\nIn %s serializing TL object", msg.c_str(), function_name);
    THROW_EXCEPTION(new_Exception(string{}, 0, msg, -1));
  }
}