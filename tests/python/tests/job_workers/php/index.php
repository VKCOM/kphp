<?php

function make_big_fake_array() {
  $arr = [];
  for ($i = 0; $i != 1000; ++$i) {
    $arr["$i key"] = ["hello" => "world"];
  }
  $arr2 = [$arr, $arr, $arr, $arr, $arr, $arr, $arr, $arr, $arr, $arr];
  return [$arr2, $arr2, $arr2, $arr2, $arr2, $arr2, $arr2, $arr2, $arr2];
}

if (isset($_SERVER["JOB_ID"])) {
  require_once "job_worker.php";
  do_job_worker();
} else {
  require_once "http_worker.php";
  do_http_worker();
}
