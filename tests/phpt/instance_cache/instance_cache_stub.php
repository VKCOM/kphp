<?php

$always_equal_assertions = false;

#ifndef KittenPHP
  $always_equal_assertions = true;
  $cache = [];

  function instance_cache_store($key, $value) {
    global $cache;
    $cache[$key] = $value;
    return true;
  }

  function instance_cache_fetch($type, $key) {
    global $cache;
    if (isset($cache[$key])) {
      $instance = $cache[$key];
      if (get_class($instance) == $type) {
        return $instance;
      }
    }
    return false;
  }

  function instance_cache_fetch_immutable($type, $key) {
    return instance_cache_fetch($type, $key);
  }

  function instance_cache_delete($key) {
    global $cache;
    $deleted = isset($cache[$key]);
    unset($cache[$key]);
    return $deleted;
  }

  function instance_cache_clear() {
    global $cache;
    $cache = [];
    return true;
  }

  function tuple() {
    return func_get_args();
  }
#endif

function assert_equal($x, $y) {
  global $always_equal_assertions;
  var_dump($always_equal_assertions || $x == $y);
}
