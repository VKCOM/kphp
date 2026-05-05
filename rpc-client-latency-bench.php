<?php


if (false) {
  new VK\TL\_common\Types\rpcResponseOk;
  new VK\TL\_common\Types\rpcResponseHeader;
  new VK\TL\_common\Types\rpcResponseError;
  new VK\TL\memcache\Functions\memcache_get;
}

function proxy_queries($queries) {
//   $start_time = microtime(true);
//   $requests_time_start = $start_time;

//   if ($is_k2 === 1) {
    $query_ids = rpc_send_typed_query_requests("rpc", $queries, null, false, null, false);
//   } else {
//     $connection = new_rpc_connection("127.0.0.1", 9999);
//     $query_ids = typed_rpc_tl_query($connection, $queries, null, false, null, false);
//   }

//   $requests_time_end = microtime(true);
//   $responses_time_start = $requests_time_end;

//   if ($is_k2 === 1) {
    $responses = rpc_fetch_typed_responses($query_ids);
//   } else {
//     $responses = typed_rpc_tl_query_result($query_ids);
//   }

//   $responses_time_end = microtime(true);

//   $end_time = $responses_time_end;

  foreach ($responses as $response) {
    if (is_null($response)) {
      critical_error("null response");
    }
  }

//   echo "Wall time -> " . number_format(($end_time - $start_time) * 1000, 2) . ", requests time -> " . number_format(($requests_time_end - $requests_time_start) * 1000, 2) .
//       ", responses time -> " . number_format(($responses_time_end - $responses_time_start) * 1000, 2) . "\n";

  return $responses[0];
}

function fibonacciRecursive($n) {
    if ($n <= 1) {
        return $n;
    } else {
        return fibonacciRecursive($n - 1) + fibonacciRecursive($n - 2);
    }
}

$fibonacci_num = (int) $_ENV['FIBONACCI_NUM'];
// $fibonacci_num = 30;
// echo $fibonacci_num;

// CPU WORK
$a = fibonacciRecursive($fibonacci_num);

$sum_num = (int) $_ENV['SUM_NUM'];
$b = 0;
for ($i = 1; $i <= $sum_num; ++$i) {
  $b += $i;
}

$requests_num = (int) $_ENV['REQUESTS_NUM'];
// $requests_num = 0;
// echo $requests_num;

// $key_len = (int) $_ENV['KEY_LEN'];

// NET WORK
$query = rpc_server_fetch_request();
$queries = array_fill(0, $requests_num, $query);

if ($requests_num >= 0) {
    $response = proxy_queries($queries);
}

// $start = microtime(true);
// $queries = array_map(function ($i) { return $query; }, range(0, $requests_num));
// $end = microtime(true);
// echo "It took " . number_format(($end - $start) * 1000, 2) . " to create an array\n";


// rpc_server_store_response($response->getResult());

$responsee = VK\TL\memcache\Functions\memcache_get::createRpcServerResponse(new VK\TL\memcache\Types\memcache_not_found());
rpc_server_store_response($responsee);

// wait(fork(benchmark($queries)), 0.15);
// exit(0);
