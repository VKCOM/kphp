<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

function test() {
  $lib = FFI::scope('pointers');
  var_dump($lib->intptr_addr_value(null) === 0);
  var_dump($lib->voidptr_addr_value(null) === 0);
}

/** @param ffi_cdata<pointers, int*> $ptr */
function test2($ptr) {
  $lib = FFI::scope('pointers');
  var_dump($lib->intptr_addr_value($ptr) === 0);
  var_dump($lib->voidptr_addr_value($ptr) === 0);
}

test();
test2(null);
$null_var = null;
test2($null_var);
