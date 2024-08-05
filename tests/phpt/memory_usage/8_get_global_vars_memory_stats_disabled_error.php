@kphp_should_fail k2_skip
KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS=0
/use KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS to enable/
<?php


$arr = [1, 2, 3, 4, 5];
$arr[] = 6;

$str = "hello";
$str .= " world";

function function_with_static_var() {
  static $static_array = [9, 10, 11, 12, "13", 14.5];
  $static_array[] = [15];

  static $static_str = "hello";
  $static_str .= " world";
}

function_with_static_var();

function test() {
  var_dump(get_global_vars_memory_stats());
}

test();
