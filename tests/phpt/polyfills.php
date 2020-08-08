<?php
#ifndef KittenPHP

use MessagePack\BufferUnpacker;
use MessagePack\Exception\InvalidOptionException;
use MessagePack\Exception\PackingFailedException;
use MessagePack\Exception\UnpackingFailedException;
use MessagePack\MessagePack;
use MessagePack\Packer;
use MessagePack\PackOptions;
use VK\InstanceSerialization\ClassTransformer;
use VK\InstanceSerialization\InstanceParser;

function classAutoLoader($class) {
  $filename = $class.'.php';
  $filename = str_replace('\\', '/', $filename);
  if (strpos($filename, "MessagePack") !== false) {
    $filename = __DIR__."/vendor/".$filename;
  } else if (strpos($filename, "VK") === 0) {
    $filename = __DIR__."/".$filename;
  } else {
    [$script_path] = get_included_files();
    $filename = dirname($script_path)."/".$filename;
  }

  require_once $filename;
}

spl_autoload_register('classAutoLoader', true, true);

/**
 * 'tuple' implementation for plain php: it is just an array of input arguments: tuple(1,'s') === [1,'s']
 * @noinspection PhpDocSignatureInspection
 * @noinspection PhpUnusedParameterInspection
 */
function tuple(...$args) {
  // íå äàžì íàòèâíîìó èíôåððèíãó PhpStorm ïîíÿòü, ÷òî ïðîèñõîäèò
  return ${'args'};
}

/**
 * 'shape' implementation for plain php: it is just an associative array, the same as provided argument
 * @noinspection PhpDocSignatureInspection
 * @noinspection PhpUnusedParameterInspection
 */
function shape(array $associative_arr) {
  // íå äàžì íàòèâíîìó èíôåððèíãó PhpStorm ïîíÿòü, ÷òî ïðîèñõîäèò
  return ${'associative_arr'};
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

function array_merge_into(array &$a, array $another_array) {
  $a = array_merge($a, $another_array);
}

function array_filter_by_key(array $a, callable $pred) {
  return array_filter($a, $pred, ARRAY_FILTER_USE_KEY);
}

function instance_cast($instance, $unused) {
    return $instance;
}

$instance_cache_storage = [];

function instance_cache_store($key, $value, $ttl = 0) {
  global $instance_cache_storage;
  $instance_cache_storage[$key] = clone $value;
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

function instance_cache_update_ttl($key, $ttl = 0) {
  global $instance_cache_storage;
  return isset($instance_cache_storage[$key]);
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

$__forked = array();

function fork($x) {
  global $__forked;

  $__forked[] = $x;
  return count($__forked);
}

function wait($id, $timeout = -1.0) {
  global $__forked;
  return 0 < $id && $id <= count($__forked) && $__forked[$id - 1] !== '__already_gotten__';
}

function wait_result($id, $timeout = -1.0) {
  global $__forked;

  if (!wait($id, $timeout)) {
    return null;
  }

  $result = $__forked[$id - 1];

  $__forked[$id - 1] = '__already_gotten__';

  return $result;
}

function wait_synchronously($id) {
  return wait($id, -1);
}

function wait_multiple($id) {
  static $waiting = array();

  if (!array_key_exists($id, $waiting)) {
    $waiting[$id] = true;
    wait($id);
    unset($waiting[$id]);
  } else {
    while ($waiting[$id]) {
      sched_yield();
      if ($waiting[$id]) {
        usleep(10 * 1000);
      }
    }
  }
}

function wait_queue_create($request_ids) {
  if (is_array($request_ids)) {
    return $request_ids;
  }
  return array($request_ids);
}

function wait_queue_push(&$queue_id, $request_ids) {
  if ($queue_id == -1) {
    return wait_queue_create($request_ids);
  }

  if (is_array($request_ids)) {
    $queue_id += $request_ids;
  } else {
    $queue_id[] = $request_ids;
  }

  return $queue_id;
}

function wait_queue_empty($queue_id) {
  return $queue_id === -1 || empty($queue_id);
}

function wait_queue_next(&$queue_id, $timeout = -1.0) {
  if (wait_queue_empty($queue_id)) {
    return 0;
  }
  return array_pop($queue_id);
}

function wait_queue_next_synchronously($queue_id) {
  return wait_queue_next($queue_id, -1);
}

function sched_yield() {
  return;
}

function rpc_tl_query_result_synchronously($query_ids) {
  return rpc_tl_query_result($query_ids);
}

function get_running_fork_id() {
  return 0;
}

function array_find(array $ar, callable $clbk) {
  foreach ($ar as $k => $v) {
    if ($clbk($v)) {
      return [$k, $v];
    }
  }
  return [null, null];
}

function warning($str) {
  trigger_error($str, E_USER_WARNING);
}

function run_or_warning(callable $fun) {
  try {
    return $fun();
  } catch (Throwable $e) {
    warning($e->getMessage() . "\n" . $e->getTraceAsString());
    return null;
  }
}

function instance_serialize(object $instance): ?string {
  ClassTransformer::$depth = 0;
  return run_or_warning(static function() use ($instance) {
    $packer = (new Packer(PackOptions::FORCE_STR))->extendWith(new ClassTransformer());
    return $packer->pack($instance);
  });
}

function instance_deserialize(string $packed_str, string $type_of_instance): ?object {
  return run_or_warning(static function() use ($packed_str, $type_of_instance) {
    $unpacked_array = msgpack_deserialize_safe($packed_str);

    $instance_parser = new InstanceParser($type_of_instance);
    return $instance_parser->fromUnpackedArray($unpacked_array);
  });
}

/**
 * @param mixed $value
 * @return string
 */
function msgpack_serialize($value): string {
  return run_or_warning(static function() use ($value) {
    return msgpack_serialize_safe($value);
  });
}

/**
 * @param mixed $value
 * @return string
 * @throws InvalidOptionException
 * @throws PackingFailedException
 */
function msgpack_serialize_safe($value): string {
  $packer = new Packer(PackOptions::FORCE_STR);
  return $packer->pack($value);
}

/**
 * @param string $packed_str
 * @return mixed
 */
function msgpack_deserialize(string $packed_str) {
  return run_or_warning(static function() use ($packed_str) {
    return msgpack_deserialize_safe($packed_str);
  });
}

/**
 * @param string $packed_str
 * @throws InvalidOptionException
 * @throws UnpackingFailedException
 * @return mixed
 */
function msgpack_deserialize_safe(string $packed_str) {
  $unpacker = new BufferUnpacker($packed_str, null);
  $result = $unpacker->unpack();
  if (($remaining = $unpacker->getRemainingCount())) {
    $off = strlen($packed_str) - $remaining;
    throw new UnpackingFailedException("Consumed only first {$off} characters of " . strlen($packed_str) . " during deserialization");
  }
  return $result;
}

function not_false($x) {
  return $x;
}

function not_null($x) {
  return $x;
}

function kphp_backtrace(bool $pretty = true): array {
  $backtrace = array_column(debug_backtrace(), "function");
  array_shift($backtrace);
  return $backtrace;
}

if (false)
#endif
define('kphp', 1);
