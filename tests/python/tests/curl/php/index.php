<?php

function simple_function() {
    $stderr = fopen("php://stderr", "w");
    fwrite($stderr, "start_resumable_function\n");
    sched_yield_sleep(0.3);
    fwrite($stderr, "end_resumable_function\n");
    return true;
}

function main() {
  if (strpos($_SERVER["PHP_SELF"], "/echo/multi/") === 0) {
    usleep(500000);
    echo substr($_SERVER["REQUEST_URI"], strlen("/echo/multi/"));
    return;
  }

  if (strpos($_SERVER["PHP_SELF"], "/echo") === 0) {
    usleep(500000);
    $resp = array_filter_by_key($_SERVER, function ($key): bool {
      return strpos($key, "HTTP_") === 0 ||
        in_array($key, ["REQUEST_URI", "REQUEST_METHOD", "SERVER_PROTOCOL"]);
    });
    if ($body = json_decode(file_get_contents('php://input'))) {
      $resp["POST_BODY"] = $body;
    }
    echo json_encode($resp);
    return;
  }

  switch ($_SERVER["PHP_SELF"]) {
    case "/test_curl": {
        test_curl();
        return;
      }
      case "/test_curl_resumable": {
        $future = fork(simple_function());
        test_curl(true);
        wait($future);
        return;
      }
      case "/test_curl_reuse_handle": {
        test_curl_reuse_handle();
        return;
      }
      case "/test_curl_resumable_reuse_handle": {
        test_curl_reuse_handle(true);
        return;
      }
      case "/test_curl_reset_handle": {
        test_curl_reset_handle();
        return;
      }
      case "/test_curl_multi": {
        test_curl_multi();
        return;
      }
  }

  critical_error("unknown test");
}

function to_json_safe($str) {
  return is_string($str) && $str != '' ? json_decode($str) : $str;
}

function test_curl($curl_resumable = false) {
  $params = json_decode(file_get_contents('php://input'));

  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, (string)$params["url"]);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, (int)$params["return_transfer"]);

  $headers = ["Content-Type: application/json"];
#ifndef K2
  $headers = [];
#endif
  curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);

  if ($post = $params["post"]) {
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($post));
  }

  if ($headers = $params["headers"]) {
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
  }

  $timeout_s = $params["timeout"];
  if (!$curl_resumable) {
    // CURLOPT_TIMEOUT_MS for resumable case isn't working, instead timeout may be set as curl_exec_concurrently() param
    curl_setopt($ch, CURLOPT_TIMEOUT_MS, $timeout_s * 1000);
  }

  if ($connection_only = $params["connect_only"]) {
    curl_setopt($ch, CURLOPT_CONNECT_ONLY, 1);
  }

  ob_start();

  $stderr = fopen("php://stderr", "w");
  fwrite($stderr, "start_curl_query\n");
  $output = $curl_resumable ? curl_exec_concurrently($ch, $timeout_s ?? -1) : curl_exec($ch);
  fwrite($stderr, "end_curl_query\n");
  curl_close($ch);

  $resp = ["exec_result" => to_json_safe($output)];
  if ($ob_json = ob_get_clean()) {
    $resp["output_buffer"] = json_decode($ob_json);
  }
  echo json_encode($resp);
}

function test_curl_reuse_handle($curl_resumable = false) {
  $params = json_decode(file_get_contents('php://input'));

  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, (string)$params["url"]);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  $timeout_s = $params["timeout"];

  // set timeout only on first request
  if (!$curl_resumable) {
    curl_setopt($ch, CURLOPT_TIMEOUT_MS, $timeout_s * 1000);
  }
  $output1 = $curl_resumable ? curl_exec_concurrently($ch, $timeout_s ?? -1) : curl_exec($ch);
  if (!$curl_resumable) {
    curl_setopt($ch, CURLOPT_TIMEOUT_MS, 0);
  }

  $output2 = $curl_resumable ? curl_exec_concurrently($ch, 30) : curl_exec($ch);
  curl_close($ch);

  $resp = ["exec_result1" => to_json_safe($output1), "exec_result2" => to_json_safe($output2)];
  echo json_encode($resp);
}

function test_curl_reset_handle() {
  $params = json_decode(file_get_contents('php://input'));

  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, (string)$params["url"] . "/before");
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

  $output1 = curl_exec($ch);

  curl_reset($ch);

  $output2 = curl_exec($ch);

  curl_setopt($ch, CURLOPT_URL, (string)$params["url"] . "/after");
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

  $output3 = curl_exec($ch);

  curl_close($ch);

  $resp = ["exec_result1" => to_json_safe($output1), "exec_result2" => to_json_safe($output2), "exec_result3" => to_json_safe($output3)];
  echo json_encode($resp);
}

function test_curl_multi() {
  $params = json_decode(file_get_contents('php://input'));

  $url1 = (string)$params["url1"];
  $url2 = (string)$params["url2"];


  $ch1 = curl_init($url1);
  $ch2 = curl_init($url2);

  curl_setopt($ch1, CURLOPT_RETURNTRANSFER, true);
  curl_setopt($ch2, CURLOPT_RETURNTRANSFER, true);

  $mh = curl_multi_init();

  curl_multi_add_handle($mh, $ch1);
  curl_multi_add_handle($mh, $ch2);

  $running = null;
  $status = 0;
  // Execute the multi handle
  do {
      $msgs_in_queue = 0;
      curl_multi_info_read($mh, $msgs_in_queue);

      $status = curl_multi_exec($mh, $running);
      // Wait for activity on any curl-connection
      if ($running) {
          curl_multi_select($mh);
      }
  } while ($running && $status == CURLM_OK);

  if ($status != CURLM_OK) {
      $resp = ["mutli_ret_code" => $status, "easy_result1" => "", "easy_result2" => ""];
      echo json_encode($resp);
      return;
  }

  $response1 = (string)curl_multi_getcontent($ch1);
  $response2 = (string)curl_multi_getcontent($ch2);

  curl_multi_remove_handle($mh, $ch1);
  curl_multi_remove_handle($mh, $ch2);

  curl_multi_close($mh);
  curl_close($ch1);
  curl_close($ch2);

  $resp = ["mutli_ret_code" => $status, "easy_result1" => $response1, "easy_result2" => $response2];
  echo json_encode($resp);
}

main();
