<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

class Example {
  /** @var ffi_cdata<test, struct Object**> */
  public static $obj_ptr2 = null;
  /** @var ffi_cdata<test, struct Object*> */
  public static $obj_ptr = null;
  /** @var ffi_cdata<test, struct Object> */
  public static $obj = null;
}

function bool_string(bool $b) {
  return $b ? 'true' : 'false';
}

function test() {
  $lib = FFI::scope('pointers');
  echo __LINE__ . ': ' . bool_string($lib->intptr_addr_value(null) === 0) . "\n";
  echo __LINE__ . ': ' . bool_string($lib->voidptr_addr_value(null) === 0) . "\n";
}

/** @param ffi_cdata<pointers, int*> $ptr */
function test2($ptr) {
  $lib = FFI::scope('pointers');
  echo __LINE__ . ': ' . bool_string($lib->intptr_addr_value($ptr) === 0) . "\n";
  echo __LINE__ . ': ' . bool_string($lib->voidptr_addr_value($ptr) === 0) . "\n";
}

function test3() {
  $lib = FFI::scope('pointers');
  echo __LINE__ . ': ' . ($lib->strlen_safe(null)) . "\n";
  $ptr1 = FFI::new('char *');
  $ptr2 = FFI::new('const char*');
  echo __LINE__ . ': ' . ($lib->strlen_safe($ptr1)) . "\n";
  echo __LINE__ . ': ' . ($lib->strlen_safe($ptr2)) . "\n";
}

/** @param ffi_scope<test> $cdef */
function test4($cdef) {
  $lib = FFI::scope('pointers');

  echo __LINE__ . ': ' . bool_string(Example::$obj_ptr2 === null) . "\n";
  echo __LINE__ . ': ' . bool_string(Example::$obj_ptr === null) . "\n";
  echo __LINE__ . ': ' . bool_string(Example::$obj === null) . "\n";

  echo __LINE__ . ': ' . bool_string($lib->voidptr_addr_value(Example::$obj_ptr2) === 0) . "\n";
  Example::$obj_ptr2 = $cdef->new('struct Object**');
  echo __LINE__ . ': ' . bool_string(Example::$obj_ptr2 === null) . "\n";
  echo __LINE__ . ': ' . bool_string(FFI::isNull(Example::$obj_ptr2)) . "\n";
  echo __LINE__ . ': ' . bool_string(FFI::isNull(FFI::addr(Example::$obj_ptr2))) . "\n";
  echo __LINE__ . ': ' . bool_string($lib->voidptr_addr_value(Example::$obj_ptr2) !== 0) . "\n";

  echo __LINE__ . ': ' . bool_string($lib->voidptr_addr_value(Example::$obj_ptr) === 0) . "\n";
  Example::$obj_ptr = $cdef->new('struct Object*');
  echo __LINE__ . ': ' . bool_string(Example::$obj_ptr === null) . "\n";
  echo __LINE__ . ': ' . bool_string(FFI::isNull(Example::$obj_ptr)) . "\n";
  echo __LINE__ . ': ' . bool_string(FFI::isNull(FFI::addr(Example::$obj_ptr))) . "\n";
  echo __LINE__ . ': ' . bool_string($lib->voidptr_addr_value(Example::$obj_ptr) !== 0) . "\n";

  Example::$obj = $cdef->new('struct Object');
  echo __LINE__ . ': ' . bool_string(Example::$obj === null) . "\n";
  echo __LINE__ . ': ' . bool_string(FFI::isNull(FFI::addr(Example::$obj))) . "\n";
}

$cdef = FFI::cdef('
#define FFI_SCOPE "test"
struct Object {
  int x;
};
');

test();
test2(null);
$null_var = null;
test2($null_var);
test3();
test4($cdef);
