<?php

function memory_work() {
  $a = "a";

  $n = 1e6;
  for ($i = 1; $i < $n; ++$i) {
    $a .= "a";
  }

  return $a;
}

function test_memory_usage() {
  $base_usage = memory_get_usage(false);

  $a = memory_work();

  var_dump(memory_get_usage(false) - $base_usage);
  
  unset($a);

  var_dump(memory_get_usage(false) - $base_usage);
  var_dump(memory_get_peak_usage(false) - $base_usage);
}

function test_total_memory_usage() {
  $base_usage = memory_get_total_usage();

  $a = memory_work();

  var_dump(memory_get_total_usage() - $base_usage);
  
  unset($a);

  var_dump(memory_get_total_usage() - $base_usage);
  var_dump(memory_get_peak_usage(true) - $base_usage);
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
