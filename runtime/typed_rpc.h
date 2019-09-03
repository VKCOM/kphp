#pragma once
#include "runtime/kphp_core.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_query.h"
#include "runtime/tl/rpc_request.h"
#include "runtime/tl/rpc_response.h"

struct rpc_connection;

int typed_rpc_tl_query_impl(const rpc_connection &connection,
                            const RpcRequest &req,
                            double timeout,
                            bool ignore_answer,
                            bool bytes_estimating,
                            int &bytes_sent,
                            bool flush);

class_instance<C$VK$TL$RpcResponse> typed_rpc_tl_query_result_one_impl(int query_id,
                                                                 const RpcErrorFactory &error_factory);

array<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_impl(const array<int> &query_ids,
                                                                    const RpcErrorFactory &error_factory);

array<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_synchronously_impl(const array<int> &query_ids,
                                                                                  const RpcErrorFactory &error_factory);

template<typename F, typename R = KphpRpcRequest>
int f$typed_rpc_tl_query_one(const rpc_connection &connection,
                             const class_instance<F> &query_function,
                             double timeout = -1.0) {
  static_assert(std::is_base_of<C$VK$TL$RpcFunction, F>::value, "Unexpected type");
  static_assert(std::is_same<KphpRpcRequest, R>::value, "Unexpected type");

  int bytes_sent = 0;
  return typed_rpc_tl_query_impl(connection, R{query_function}, timeout, false, false, bytes_sent, true);
}

template<typename F, typename R = KphpRpcRequest>
array<int> f$typed_rpc_tl_query(const rpc_connection &connection,
                                const array<class_instance<F>> &query_functions,
                                double timeout = -1.0,
                                bool ignore_answer = false) {
  static_assert(std::is_base_of<C$VK$TL$RpcFunction, F>::value, "Unexpected type");
  static_assert(std::is_same<KphpRpcRequest, R>::value, "Unexpected type");

  array<int> queries(query_functions.size());
  int bytes_sent = 0;
  for (auto it = query_functions.begin(); it != query_functions.end(); ++it) {
    auto rpc_query = typed_rpc_tl_query_impl(connection, R{it.get_value()}, timeout, ignore_answer, true, bytes_sent, false);
    queries.set_value(it.get_key(), rpc_query);
  }
  if (bytes_sent > 0) {
    f$rpc_flush();
  }

  return queries;
}

template<typename I, typename F = rpcResponseErrorFactory>
class_instance<C$VK$TL$RpcResponse> f$typed_rpc_tl_query_result_one(I query_id) {
  static_assert(std::is_same<int, I>::value, "Unexpected type");
  static_assert(std::is_same<rpcResponseErrorFactory, F>::value, "Unexpected type");
  return typed_rpc_tl_query_result_one_impl(query_id, F::get());
}

template<typename I, typename F = rpcResponseErrorFactory>
array<class_instance<C$VK$TL$RpcResponse>> f$typed_rpc_tl_query_result(const array<I> &query_ids) {
  static_assert(std::is_same<int, I>::value, "Unexpected type");
  static_assert(std::is_same<rpcResponseErrorFactory, F>::value, "Unexpected type");
  return typed_rpc_tl_query_result_impl(query_ids, F::get());
}

template<typename I, typename F = rpcResponseErrorFactory>
array<class_instance<C$VK$TL$RpcResponse>> f$typed_rpc_tl_query_result_synchronously(const array<I> &query_ids) {
  static_assert(std::is_same<int, I>::value, "Unexpected type");
  static_assert(std::is_same<rpcResponseErrorFactory, F>::value, "Unexpected type");
  return typed_rpc_tl_query_result_synchronously_impl(query_ids, F::get());
}

void free_typed_rpc_lib();
