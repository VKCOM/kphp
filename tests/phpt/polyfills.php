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

function array_keys_as_strings($a) {
  $keys = [];
  foreach ($a as $key => $value) {
    array_push($keys, (string)$key);
  }
  return $keys;
}

function array_keys_as_ints($a) {
  $keys = [];
  foreach ($a as $key => $value) {
    array_push($keys, (int)$key);
  }
  return $keys;
}

function instance_cast($instance, $unused) {
    return $instance;
}

$instance_cache_storage = [];

function instance_cache_store($key, $value) {
  global $instance_cache_storage;
  $instance_cache_storage[$key] = $value;
  return true;
}

function instance_cache_fetch($type, $key) {
  global $instance_cache_storage;
  if (isset($instance_cache_storage[$key])) {
    $instance = $instance_cache_storage[$key];
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
  global $instance_cache_storage;
  $deleted = isset($instance_cache_storage[$key]);
  unset($instance_cache_storage[$key]);
  return $deleted;
}

function instance_cache_clear() {
  global $instance_cache_storage;
  $instance_cache_storage = [];
  return true;
}

function cp1251($utf8_string) {
  return vk_utf8_to_win($utf8_string);
}

function register_kphp_on_warning_callback($callback) {
  $handler = function ($errno, $message) use($callback) {
    $stacktrace = array_map(function($o) { return $o['function']; }, debug_backtrace());
    $callback($message, $stacktrace);
  };
  set_error_handler($handler);
}

define('kphp', 0);
if (0)
#endif
define('kphp', 1);
