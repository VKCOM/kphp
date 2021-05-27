<?php

function main() {
  if (strpos($_SERVER["PHP_SELF"], "/echo") === 0) {
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
    case "/test_curl":
      test_curl();
      return;
  }

  critical_error("unknown test");
}

function test_curl() {
  $params = json_decode(file_get_contents('php://input'));

  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, (string)$params["url"]);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, (int)$params["return_transfer"]);

  if ($post = $params["post"]) {
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($post));
  }

  if ($headers = $params["headers"]) {
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
  }

  ob_start();

  $output = curl_exec($ch);
  curl_close($ch);

  $resp = ["exec_result" => is_string($output) ? json_decode($output) : $output];
  if ($ob_json = ob_get_clean()) {
    $resp["output_buffer"] = json_decode($ob_json);
  }
  echo json_encode($resp);
}

main();
