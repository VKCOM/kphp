// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/tl/rpc_tl_query.h"

#include <cstdarg>

#include "runtime/exception.h"
#include "runtime/tl/rpc_request.h"
#include "runtime/tl/tl_func_base.h"

void RpcPendingQueries::save(const class_instance<RpcTlQuery>& query) {
  php_assert(!queries_.has_key(query.get()->query_id));
  queries_.set_value(query.get()->query_id, query);
}

class_instance<RpcTlQuery> RpcPendingQueries::withdraw(int64_t query_id) {
  auto query = queries_.get_value(query_id);
  queries_.unset(query_id);
  return query;
}

void RpcPendingQueries::hard_reset() {
  hard_reset_var(queries_);
}

void CurrentTlQuery::reset() {
  current_tl_function_name_ = string();
}

void CurrentTlQuery::set_current_tl_function(const string& tl_function_name) {
  // It can be not empty in the following case:
  // 1. Timeout is raised in the middle of serialization (when current TL function is still not reset).
  // 2. Then shutdown functions called from timeout.
  // 3. They use RPC which finally call set_current_tl_function.
  // It will be rewritten by another tl_function_name and work fine
  current_tl_function_name_ = tl_function_name;
}

void CurrentTlQuery::set_current_tl_function(const class_instance<RpcTlQuery>& current_query) {
  current_tl_function_name_ = current_query.get()->tl_function_name;
}

void CurrentTlQuery::raise_fetching_error(const char* format, ...) {
  php_assert(!current_tl_function_name_.empty());
  if (CurException.is_null()) {
    constexpr size_t BUFF_SZ = 1024;
    char buff[BUFF_SZ] = {0};
    va_list args;
    va_start(args, format);
    int32_t sz = vsnprintf(buff, BUFF_SZ, format, args);
    php_assert(sz > 0);
    va_end(args);
    string msg = string(buff, static_cast<size_t>(sz));
    php_warning("Fetching error:\n%s\nIn %s deserializing TL object", msg.c_str(), current_tl_function_name_.c_str());
    msg.append(string(" in result of ")).append(current_tl_function_name_);
    THROW_EXCEPTION(new_Exception(string{}, 0, msg, -1));
  }
}

void CurrentTlQuery::raise_storing_error(const char* format, ...) {
  const char* function_name = current_tl_function_name_.empty() ? "_unknown_" : current_tl_function_name_.c_str();
  if (CurException.is_null()) {
    constexpr size_t BUFF_SZ = 1024;
    char buff[BUFF_SZ] = {0};
    va_list args;
    va_start(args, format);
    int32_t sz = vsnprintf(buff, BUFF_SZ, format, args);
    php_assert(sz > 0);
    va_end(args);
    string msg = string(buff, static_cast<size_t>(sz));
    php_warning("Storing error:\n%s\nIn %s serializing TL object", msg.c_str(), function_name);
    THROW_EXCEPTION(new_Exception(string{}, 0, msg, -1));
  }
}

struct tl_func_base_simple_wrapper : public tl_func_base {
  explicit tl_func_base_simple_wrapper(class_instance<C$RpcFunctionFetcher> && wrapped):wrapped_(std::move(wrapped)) {}

  virtual mixed fetch() {
    // all functions annotated with @kphp will override this method with the generated code
    php_critical_error("hrissan TODO: This function should never be called. Should be overridden in every @kphp TL function");
    return mixed{};
  }

  virtual class_instance<C$VK$TL$RpcFunctionReturnResult> typed_fetch() {
    return call_custom_fetcher_wrapper_2_impl(wrapped_);
  }

  virtual void rpc_server_typed_store(const class_instance<C$VK$TL$RpcFunctionReturnResult>&) {
    // all functions annotated with @kphp will override this method with the generated code
    php_critical_error("hrissan TODO: This function should never be called. Should be overridden in every @kphp TL function");
  }

private:
  class_instance<C$RpcFunctionFetcher> wrapped_;
};

std::unique_ptr<tl_func_base> make_tl_func_base_simple_wrapper(class_instance<C$RpcFunctionFetcher> && wrapped) {
  return std::unique_ptr<tl_func_base>{new tl_func_base_simple_wrapper(std::move(wrapped))};
}
