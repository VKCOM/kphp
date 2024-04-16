// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "runtime/kphp_core.h"
#include "runtime/rpc.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_tl_query.h"
#include "runtime/tl/rpc_request.h"
#include "runtime/tl/rpc_response.h"

struct C$RpcConnection;

int64_t typed_rpc_tl_query_impl(const class_instance<C$RpcConnection> &connection,
                                const RpcRequest &req,
                                double timeout,
                                rpc_request_metrics_t &request_metrics,
                                bool collect_responses_metrics,
                                bool ignore_answer,
                                bool bytes_estimating,
                                size_t &bytes_sent,
                                bool flush);

class_instance<C$VK$TL$RpcResponse> typed_rpc_tl_query_result_one_impl(int64_t query_id,
                                                                       const RpcErrorFactory &error_factory);

array<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_impl(const array<int64_t> &query_ids,
                                                                          const RpcErrorFactory &error_factory);

array<class_instance<C$VK$TL$RpcResponse>> typed_rpc_tl_query_result_synchronously_impl(const array<int64_t> &query_ids,
                                                                                        const RpcErrorFactory &error_factory);

template<typename F, typename R = KphpRpcRequest>
int64_t f$typed_rpc_tl_query_one(const class_instance<C$RpcConnection> &connection,
                                 const class_instance<F> &query_function,
                                 double timeout = -1.0) {
  static_assert(std::is_base_of_v<C$VK$TL$RpcFunction, F>, "Unexpected type");
  static_assert(std::is_same_v<KphpRpcRequest, R>, "Unexpected type");

  size_t bytes_sent = 0;
  rpc_request_metrics_t _{};
  return typed_rpc_tl_query_impl(connection, R{query_function}, timeout, _, false, false, false, bytes_sent, true);
}

template<typename F, typename R = KphpRpcRequest>
array<int64_t> f$typed_rpc_tl_query(const class_instance<C$RpcConnection> &connection,
                                    const array<class_instance<F>> &query_functions,
                                    double timeout = -1.0,
                                    bool ignore_answer = false,
                                    class_instance<C$RpcRequestsMetrics> requests_metrics = {},
                                    bool need_responses_metrics = false) {
  static_assert(std::is_base_of_v<C$VK$TL$RpcFunction, F>, "Unexpected type");
  static_assert(std::is_same_v<KphpRpcRequest, R>, "Unexpected type");

  if (ignore_answer && need_responses_metrics) {
    php_warning(
            "Both $ignore_answer and $need_responses_metrics are 'true'. Can't collect metrics for ignored answers");
  }

  size_t bytes_sent = 0;
  array<int64_t> queries{query_functions.size()};
  array<rpc_request_metrics_t> requests_metrics_tmp{query_functions.size()};

  for (auto it = query_functions.begin(); it != query_functions.end(); ++it) {
    rpc_request_metrics_t req_metrics{};

    int64_t rpc_query = typed_rpc_tl_query_impl(connection, R{it.get_value()}, timeout, req_metrics,
                                                !ignore_answer && need_responses_metrics, ignore_answer, true,
                                                bytes_sent,
                                                false);

    queries.set_value(it.get_key(), rpc_query);
    requests_metrics_tmp.set_value(it.get_key(), std::move(req_metrics));
  }

  if (bytes_sent > 0) {
    f$rpc_flush();
  }

  if (!requests_metrics.is_null()) {
    requests_metrics->metrics_arr_ = std::move(requests_metrics_tmp);
  }

  return queries;
}

template<typename I, typename F = rpcResponseErrorFactory>
class_instance<C$VK$TL$RpcResponse> f$typed_rpc_tl_query_result_one(I query_id) {
  static_assert(std::is_same_v<int64_t, I>, "Unexpected type");
  static_assert(std::is_same_v<rpcResponseErrorFactory, F>, "Unexpected type");
  return typed_rpc_tl_query_result_one_impl(query_id, F::get());
}

template<typename I, typename F = rpcResponseErrorFactory>
array<class_instance<C$VK$TL$RpcResponse>> f$typed_rpc_tl_query_result(const array<I> &query_ids) {
  static_assert(std::is_same_v<int64_t, I>, "Unexpected type");
  static_assert(std::is_same_v<rpcResponseErrorFactory, F>, "Unexpected type");
  return typed_rpc_tl_query_result_impl(query_ids, F::get());
}

template<typename I, typename F = rpcResponseErrorFactory>
array<class_instance<C$VK$TL$RpcResponse>> f$typed_rpc_tl_query_result_synchronously(const array<I> &query_ids) {
  static_assert(std::is_same_v<int64_t, I>, "Unexpected type");
  static_assert(std::is_same_v<rpcResponseErrorFactory, F>, "Unexpected type");
  return typed_rpc_tl_query_result_synchronously_impl(query_ids, F::get());
}

void free_typed_rpc_lib();
