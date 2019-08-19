<?php
require_once "init_typed_rpc_tests.php";
set_tl_mode(SAFE_NEW_TL_MODE);

function _memcache_set() {
  $q_set = new VK\TL\Functions\memcache\set;
  $q_set->key = '1111';
  $q_set->value = "foo";
  $q_set->flags = 0;
  $q_set->delay = 10000;

  $value = VK\TL\Functions\memcache\set::result(rpc_query_sync($q_set, MEMCACHE_PORT));
  echo "memcache.set: ", $value, "\n";
}

function _memcache_get($key) {
  $q_set = new VK\TL\Functions\memcache\get;
  $q_set->key = $key;

  $value = VK\TL\Functions\memcache\get::result(rpc_query_sync($q_set, MEMCACHE_PORT));
  echo "memcache.get: ";
  if ($value instanceof VK\TL\Types\memcache\strvalue) {
    echo "value = ", $value->value, "; flags = ", $value->flags, "\n";
  } else if ($value instanceof VK\TL\Types\memcache\not_found) {
    echo "memcache.not_found\n";
  }
}

_memcache_set();
_memcache_get("123123");
_memcache_get("1111");