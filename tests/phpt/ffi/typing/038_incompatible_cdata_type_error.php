@kphp_should_fail k2_skip
/assign FFI\\CData_int64 to Foo::\$int32/
/declared as @var FFI\\CData_int32/
<?php

class Foo {
  /** @var \FFI\CData_int32 $x */
  public $int32;
}

$int64 = FFI::new('int64_t');
$foo = new Foo();
$foo->int32 = $int64;
