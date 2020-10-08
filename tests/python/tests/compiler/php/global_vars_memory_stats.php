<?php

$result = ["initial" => get_global_vars_memory_stats()];

$arr = [1, 2, 3, 4, 5];
$arr[] = 6;

$str = "hello";
$str .= " world";

$result["globals"] = get_global_vars_memory_stats();

function function_with_static_var() {
  static $static_array = [9, 10, 11, 12, "13", 14.5];
  $static_array[] = [15];

  static $static_str = "hello";
  $static_str .= " world";
}

function_with_static_var();

$result["static"] = get_global_vars_memory_stats();

echo json_encode($result);
