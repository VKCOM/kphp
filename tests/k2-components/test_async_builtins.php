<?php

$async_func_str = "async function result";
$sync_func_str = "sync function result";

/** @kphp-required */
function async_func(string $str): string {
  sched_yield_sleep(0.1);
  global $async_func_str;
  return $async_func_str;
}

/** @kphp-required */
function sync_func(string $str): string {
  global $sync_func_str;
  return $sync_func_str;
}

$arr = ["1", "2", "3"];

foreach (array_map('async_func', $arr) as $elem) {
  if ($elem != $async_func_str) {
    critical_error("error");
  }
}

foreach (array_map('sync_func', $arr) as $elem) {
  if ($elem != $sync_func_str) {
    critical_error("error");
  }
}
