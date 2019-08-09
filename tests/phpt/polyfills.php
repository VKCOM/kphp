<?php
#ifndef KittenPHP

function tuple(...$args) {
    return $args;
}

function instance_to_array($instance) {
    if (!is_object($instance)) {
        return $instance;
    }

    $convert_array_of_instances = function (&$a) use (&$convert_array_of_instances) {
        foreach ($a as &$v) {
            if (is_object($v)) {
                $v = instance_to_array($v);
            } elseif (is_array($v)) {
                $convert_array_of_instances($v);
            }
        }
    };

    $a = (array) $instance;
    $convert_array_of_instances($a);

    return $a;
}

function array_first_key(&$a) {
  reset($a);
  return key($a);
}

function array_first_value(&$a) {
  reset($a);
  return current($a);
}

function array_last_key(&$a) {
  end($a);
  return key($a);
}

function array_last_value(&$a) {
  end($a);
  return current($a);
}

function instance_cast($instance, $unused) {
    return $instance;
}

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

#endif
