<?php

$res = [];

function memory_work() {
  $a = "a";

  $n = 1e6;
  for ($i = 1; $i < $n; ++$i) {
    $a .= "a";
  }

  return $a;
}

function test_memory_usage() {
  global $res;

  $base_usage = memory_get_usage(false);

  $a = memory_work();

  $usage = memory_get_usage(false) - $base_usage;
  
  unset($a);

  $usage_after_unset = memory_get_usage(false) - $base_usage;
  $peak_usage = memory_get_peak_usage(false) - $base_usage;

  $res["usage"] = $usage;
  $res["usage_after_unset"] = $usage_after_unset;
  $res["peak_usage"] = $peak_usage;
}

function test_total_memory_usage() {
  global $res;

  $base_usage = memory_get_total_usage();

  $a = memory_work();

  $usage = memory_get_total_usage() - $base_usage;
  
  unset($a);

  $usage_after_unset = memory_get_total_usage() - $base_usage;
  $peak_usage = memory_get_peak_usage(true) - $base_usage;

  $res["usage"] = $usage;
  $res["usage_after_unset"] = $usage_after_unset;
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
