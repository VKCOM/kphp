<?php

$res = [];

$a = "";

function memory_work($n) {
  global $a;

  for ($i = 1; $i <= $n; ++$i) {
    $a .= "a";
  }
}

function test_memory_usage() {
  global $res, $a;

  $base_usage = memory_get_usage(false);

  memory_work(1e6);  # 1 MB allocation expected

  $usage = memory_get_usage(false) - $base_usage;

  $a = "";

  $usage_after_cleaning = memory_get_usage(false) - $base_usage;
  $peak_usage = memory_get_peak_usage(false) - $base_usage;

  $res["usage"] = $usage;
  $res["usage_after_cleaning"] = $usage_after_cleaning;
  $res["peak_usage"] = $peak_usage;
}

function test_total_memory_usage() {
  global $res, $a;

  memory_work(2048);  # memory usage for small pieces depends on runtime

  $base_usage = memory_get_total_usage();

  memory_work(1e6);  # 1 MB allocation expected

  $usage = memory_get_total_usage() - $base_usage;

  $a = "";

  $usage_after_cleaning = memory_get_total_usage() - $base_usage;
  $peak_usage = memory_get_peak_usage(true) - $base_usage;

  $res["usage"] = $usage;
  $res["usage_after_cleaning"] = $usage_after_cleaning;
  $res["peak_usage"] = $peak_usage;
}

switch($_SERVER["PHP_SELF"]) {
  case "/test_memory_usage":
    test_memory_usage();
    break;
  case "/test_total_memory_usage":
    test_total_memory_usage();
    break;
  default:
    critical_error('unexpected "' . $_SERVER["PHP_SELF"] . '"');
}

echo json_encode($res);
